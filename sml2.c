/**
 * sml2.c - Convert XML to SML and back
 *
 * Test libxml2 SML parsing and saving ability.
 * Run (sml2 -?) to display a help screen.
 *
 * See Copyright for the status of this software.
 *
 * jf.larvoire@free.fr
 *
 * History
 *  2019-05-13 JFL Added options -pH and -ph.
 *  2020-03-09 JFL Pass head spaces through unchanged.
 *		   Fixed the parsing of the - special arguments.
 *  2020-03-10 JFL Minor update to yesterday's head spaces change.
 *  2020-05-07 JFL Renamed option -w as -ow.
 *                 Added options -oh and -oxh.
 *  2020-10-07 JFL Report parser errors on stderr.
 *                 Added option -ih to parse HTML.
 *                 Added options -pr and -pR to control the relaxed parsing.
 *  2021-01-18 JFL Merged in significant library changes. No change in sml2.c.
 */

#define VERSION "2021-01-18"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
// #include <stdarg.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/encoding.h>
#include <libxml/xmlsave.h>
#include <libxml/xmlversion.h>
#include <libxml/HTMLparser.h>

#include <debugm.h> /* SysToolsLib debug macros */

/* Global variables */
char *program;	/* This program basename, with extension in Windows */
char *progcmd;	/* This program invokation name, without extension in Windows */
int GetProgramNames(char *argv0);	/* Initialize the above two */

/* Forward references */
int usage(void);
int xmlRemoveBlankNodes(xmlNodePtr n);
int xmlTrimTextNodes(xmlNodePtr n);

// void myXmlGenericErrorFunc(void *pUserData, const char *pszFormat, ...) {
//   va_list vl;
//   fprintf(stderr, "Error: ");
//   va_start(vl, pszFormat);
//   vfprintf(stderr, pszFormat, vl);
//   va_end(vl);
// }

/* Report parser errors on stderr */
void myXmlStructuredErrorFunc(void *pUserData, xmlErrorPtr e) {
  int *pnParserErrors = pUserData;
  if (pnParserErrors) *pnParserErrors += 1;
  fprintf(stderr, "Error at line %d column %d: %s", e->line, e->int2, e->message);
}

int usage() {
  printf("\n\
%s version " VERSION " " EXE_OS_NAME " - XML <--> SML converter based on libxml2\n\
\n\
Usage: %s [OPTIONS] [INPUT_FILENAME [OUTPUT_FILENAME]]\n\
\n\
Options:\n\
  -?        Display this help screen. Alias -h or --help.\n\
  -B        Delete blank nodes\n\
"
DEBUG_CODE("\
  -d        Debug mode. Repeat to get more debugging output\n\
")
, program, progcmd);
  printf("\
  -D        Output no ?xml declaration. Default: Same as in input\n\
  -E        Output no empty tags\n\
  -f        Format and indent the output. Default: Same as the input\n\
  -ih       Input is HTML. (Not autodetected) (Reports invalid XHTML)\n\
  -is       Input is SML. Default: Autodetect if XML or SML\n\
  -ix       Input is XML. Default: Autodetect if XML or SML\n\
  -os       Output SML. Default if the input is XML or HTML\n\
  -ow       Output non-significant white spaces\n\
  -ox       Output XML. Default if the input is SML\n\
  -pb       Parse keeping blank nodes (Default)\n\
  -pB       Parse removing blank nodes\n\
  -pe       Parse reporting errors\n\
  -pE       Parse ignoring errors (Default)\n\
  -ph       Parse using the huge file option\n\
  -pH       Parse without using the huge file option (Default)\n\
  -pn       Parse keeping entity nodes (i.e. not expanding entities) (Default)\n\
  -pN       Parse removing entity nodes (i.e. expanding entities)\n\
  -pr       Relaxed parsing, recovering from minor errors\n\
  -pR       Standard parsing, not recovering from errors (Default)\n\
  -pw       Parse reporting warnings\n\
  -pW       Parse ignoring warnings (Default)\n\
  -s        Input & output SML. Default: Input one kind & output the other\n\
  -t        Trim text nodes\n\
  -x        Input & output XML. Default: Input one kind & output the other\n\
\n\
Filenames: Default or \"-\": Use stdin and stdout respectively\n\
"
#ifdef __unix__
"\n"
#endif
);
  return 0;
}

int main(int argc, char *argv[]) {
  int i;
  xmlDocPtr doc; /* the resulting document tree */
  const char * infilename = NULL;
  const char * outfilename = NULL;
  /* const char * encoding = "ISO-8859-1"; */
  int iSaveOpts = 0;
  int iParseOpts = XML_PARSE_DETECT_ML | XML_PARSE_NOERROR | XML_PARSE_NOWARNING | HTML_PARSE_NONET;
  int iOutMLTypeSet = 0; /* 1 = Output markup type specified */
  int iXmlDeclSet = 0; /* 1 = Whether to output the ?xml declaration specified */
  xmlSaveCtxtPtr ctxt;
  int iDeleteBlankNodes = 0;
  int iTrimSpaces = 0;
  FILE *hIn, *hOut;
  char *pszHeadSpaces = NULL;
  int nHeadSpaces = 0;
  int nParserErrors = 0;
  int iParseHtml = 0;

  /* Extract the program names from argv[0] */
  GetProgramNames(argv[0]);

  /* Process arguments */
  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (   ((arg[0] == '-') && arg[1])
#if defined(_WIN32)
	|| (arg[0] == '/')
#endif
	) {
      char *opt = arg + 1;
      if (!strcmp(opt, "?") || !strcmp(opt, "h") || !strcmp(opt, "-help")) {
      	return usage();
      }
      if (!strcmp(opt, "B")) {
      	iDeleteBlankNodes = 1;
      	continue;
      }
      DEBUG_CODE(
      if (!strcmp(opt, "d")) {
	DEBUG_MORE();
      	continue;
      }
      )
      if (!strcmp(opt, "D")) {
      	iXmlDeclSet = 1;
      	iSaveOpts |= XML_SAVE_NO_DECL;
      	continue;
      }
      if (!strcmp(opt, "E")) {
      	iSaveOpts |= XML_SAVE_NO_EMPTY;
      	continue;
      }
      if (!strcmp(opt, "f")) {
      	iSaveOpts |= XML_SAVE_FORMAT;
      	/* iParseOpts |= XML_PARSE_NOBLANKS; /* This does not seem to have any effect */
      	iDeleteBlankNodes = 1; /* Do it manually instead */
      	continue;
      }
      if (!strcmp(opt, "ih")) {
      	int iParseHtml = 1;
      	iParseOpts &= ~XML_PARSE_DETECT_ML;
      	iParseOpts &= ~XML_PARSE_SML;
      	iParseOpts |= HTML_PARSE_RECOVER; /* Accept HTML that's not valid XHTML */
      	iParseOpts |= HTML_PARSE_NOIMPLIED;
      	continue;
      }
      if (!strcmp(opt, "is")) {
      	int iParseHtml = 0;
      	iParseOpts &= ~XML_PARSE_DETECT_ML;
      	iParseOpts |= XML_PARSE_SML;
      	continue;
      }
      if (!strcmp(opt, "ix")) {
      	int iParseHtml = 0;
      	iParseOpts &= ~XML_PARSE_DETECT_ML;
      	iParseOpts &= ~XML_PARSE_SML;
      	continue;
      }
      if (!strcmp(opt, "oh")) {
      	iSaveOpts |= XML_SAVE_AS_HTML;
      	continue;
      }
      if (!strcmp(opt, "os")) {
      	iSaveOpts |= XML_SAVE_AS_SML;
	iOutMLTypeSet = 1;
      	continue;
      }
      if (!strcmp(opt, "ow")) {
      	iSaveOpts |= XML_SAVE_WSNONSIG;
      	continue;
      }
      if (!strcmp(opt, "ox")) {
      	iSaveOpts &= ~XML_SAVE_AS_SML;
	iOutMLTypeSet = 1;
      	continue;
      }
      if (!strcmp(opt, "oxh")) {
      	iSaveOpts |= XML_SAVE_XHTML;
      	continue;
      }
      if (!strcmp(opt, "pb")) {
      	iParseOpts &= ~XML_PARSE_NOBLANKS;
      	continue;
      }
      if (!strcmp(opt, "pB")) {
      	iParseOpts |= XML_PARSE_NOBLANKS;
      	continue;
      }
      if (!strcmp(opt, "pe")) {
      	iParseOpts &= ~XML_PARSE_NOERROR;
      	continue;
      }
      if (!strcmp(opt, "pE")) {
      	iParseOpts |= XML_PARSE_NOERROR;
      	continue;
      }
      if (!strcmp(opt, "pH")) {
      	iParseOpts &= ~XML_PARSE_HUGE;
      	continue;
      }
      if (!strcmp(opt, "ph")) {
      	iParseOpts |= XML_PARSE_HUGE;
      	continue;
      }
      if (!strcmp(opt, "pn")) {
      	iParseOpts &= ~XML_PARSE_NOENT;
      	continue;
      }
      if (!strcmp(opt, "pN")) {
      	// Note: This does expand user-defined entities, but the 5 standard XML entities
      	//       (&lt; &gt; &amp; &quot; &apos;) will be changed back in the output.
      	iParseOpts |= XML_PARSE_NOENT;
      	continue;
      }
      if (!strcmp(opt, "pR")) {
      	iParseOpts &= ~XML_PARSE_RECOVER;
      	continue;
      }
      if (!strcmp(opt, "pr")) {
      	iParseOpts |= XML_PARSE_RECOVER;
      	continue;
      }
      if (!strcmp(opt, "pw")) {
      	iParseOpts &= ~XML_PARSE_NOWARNING;
      	continue;
      }
      if (!strcmp(opt, "pW")) {
      	iParseOpts |= XML_PARSE_NOWARNING;
      	continue;
      }
      if (!strcmp(opt, "s")) {
      	iParseOpts &= ~XML_PARSE_DETECT_ML;
      	iParseOpts |= XML_PARSE_SML;
      	iSaveOpts |= XML_SAVE_AS_SML;
	iOutMLTypeSet = 1;
      	continue;
      }
      if (!strcmp(opt, "t")) {
      	iTrimSpaces = 1;
      	continue;
      }
      if ((!strcmp(opt, "V")) || (!strcmp(opt, "-version"))) {
      	printf("%s version " VERSION " " EXE_OS_NAME, program);
      	DEBUG_CODE(
      	printf(" Debug");
      	)
      	printf(" ; libxml2 version " LIBXML_DOTTED_VERSION);
        printf("\n");
      	exit(0);
      }
      if (!strcmp(opt, "x")) {
      	iParseOpts &= ~XML_PARSE_DETECT_ML;
      	iParseOpts &= ~XML_PARSE_SML;
      	iSaveOpts &= ~XML_SAVE_AS_SML;
	iOutMLTypeSet = 1;
      	continue;
      }
      printf("Unexpected option: %s\n", arg);
      exit(1);
    }
    if (!infilename) {
      infilename = arg;
      continue;
    }
    if (!outfilename) {
      outfilename = arg;
      continue;
    }
    printf("Unexpected argument: %s\n", arg);
    exit(1);
  }
  DEBUG_PRINTF(("# Debug level %d\n", iDebug));

  if (!infilename) infilename = "-"; /* Input from stdin by default */
  if (!outfilename) outfilename = "-"; /* Output to stdout by default */

#if 0  /* This does not seem to have any effect */
  if (iSaveOpts & XML_SAVE_FORMAT) { /* If we want to reformat the output */
    i = xmlKeepBlanksDefault(0); /* Then ignore blank nodes in input */
    DEBUG_PRINTF(("xmlKeepBlanksDefault was %d\n", i)); // Prints a 1 */
  }
#endif

  /* Get the head spaces, that libxml2 skips */
  if (!strcmp(infilename, "-")) {
    hIn = stdin;
  } else {
    hIn = fopen(infilename, "rb");
    if (!hIn) {
      fprintf(stderr, "Error: Can't open input file \"%s\"\n", infilename);
      return 1;
    }
  }
  while ((i = fgetc(hIn)) != EOF) {
    if (!isspace(i)) {
      if (hIn == stdin) {
      	ungetc(i, stdin);
      } else {
      	fclose(hIn);
      }
      break;
    }
    pszHeadSpaces = realloc(pszHeadSpaces, nHeadSpaces + 1);
    if (!pszHeadSpaces) {
      fprintf(stderr, "Error: Not enough memory\n");
      return 1;
    }
    pszHeadSpaces[nHeadSpaces++] = (char)i;
  }

  /* Parse the ML input */
  // xmlSetGenericErrorFunc(NULL, myXmlGenericErrorFunc); // Commented-out as this does not seem to report anything
  xmlSetStructuredErrorFunc(&nParserErrors, myXmlStructuredErrorFunc); // Report every parsing error
  if (iParseHtml) {
    doc = htmlReadFile(infilename, NULL, iParseOpts); // This still reports invalid XHTML
  } else {
    doc = xmlReadFile(infilename, NULL, iParseOpts);
  }
  if (doc == NULL) {
    // xmlErrorPtr error = xmlGetLastError();
    // fprintf(stderr, "Failed to parse \"%s\" at line %d column %d: %s\n",
    //         infilename, error->line, error->int2, error->message);
    if (nParserErrors) {
      fprintf(stderr, "%d errors found parsing \"%s\"\n", nParserErrors, infilename);
    } else { /* Not sure this can happen, but just in case */
      fprintf(stderr, "Failed to parse \"%s\"\n", infilename);
    }
    return 1;
  }
  DEBUG_PRINTF(("# Parsed ML successfully\n"));

  /* Output the other ML type if not specified on the command line */
  if (doc->properties & XML_DOC_SML) {		/* The input doc was SML */
    DEBUG_PRINTF(("# The input was SML\n"));
    if (!iOutMLTypeSet) iSaveOpts &= ~XML_SAVE_AS_SML; /* So output XML */
  } else {					/* Else the input doc was XML */
    DEBUG_PRINTF(("# The input was XML\n"));
    if (!iOutMLTypeSet) iSaveOpts |= XML_SAVE_AS_SML;  /* So output SML */
  }

  /* Output an ?xml declaration only if there was one already */
  if (doc->properties & XML_DOC_XMLDECL) {	/* There was one */
    DEBUG_PRINTF(("# The input had an ?xml declaration\n"));
    if (!iXmlDeclSet) iSaveOpts &= ~XML_SAVE_NO_DECL;
  } else {					/* Else there was none */
    DEBUG_PRINTF(("# The input did not have an ?xml declaration\n"));
    if (!iXmlDeclSet) iSaveOpts |= XML_SAVE_NO_DECL;
  }

  if (iDeleteBlankNodes) { /* If we want to reformat the output */
    xmlRemoveBlankNodes(xmlDocGetRootElement(doc)); // Remove blank text nodes
  }

  if (iTrimSpaces) { /* If we want to reformat the output */
    xmlTrimTextNodes(xmlDocGetRootElement(doc)); // Trim spaces around text nodes
  }

  /* Output the head spaces extracted above */
  if (!strcmp(outfilename, "-")) {
    hOut = stdout;
  } else {
    hOut = fopen(outfilename, "wb");
    if (!hOut) {
      fprintf(stderr, "Error: Can't open output file \"%s\"\n", outfilename);
      return 1;
    }
  }
  if (nHeadSpaces) {
    fwrite(pszHeadSpaces, nHeadSpaces, 1, hOut);
    fflush(hOut); /* Without this, the spaces are output _after_ the ML! */
  }

  /* Generate the ML output */
  ctxt = xmlSaveToFd(fileno(hOut), NULL, iSaveOpts);
  xmlSaveDoc(ctxt, doc);
  xmlSaveClose(ctxt);
  if (hOut != stdout) fclose(hOut);
  xmlFreeDoc(doc);
  xmlCleanupParser(); /* Cleanup function for the XML library. */

  return 0;
}

/**
 * Extract the program names from argv[0]
 * Sets global variables program and progcmd.
 */

int GetProgramNames(char *argv0) {
#if defined(_MSDOS) || defined(_WIN32)
#if defined(_MSC_VER) /* Building with Microsoft tools */
#define strlwr _strlwr
#endif
  int lBase;
  char *pBase;
  char *p;
  pBase = strrchr(argv0, '\\');
  if ((p = strrchr(argv0, '/')) > pBase) pBase = p;
  if ((p = strrchr(argv0, ':')) > pBase) pBase = p;
  if (!(pBase++)) pBase = argv0;
  lBase = (int)strlen(pBase);
  program = strdup(pBase);
  strlwr(program);
  progcmd = strdup(program);
  if ((lBase > 4) && !strcmp(program+lBase-4, ".exe")) {
    progcmd[lBase-4] = '\0';
  } else {
    program = realloc(strdup(program), lBase+4+1);
    strcpy(program+lBase, ".exe");
  }
#else /* Build for Unix */
#include <libgen.h>	/* For basename() */
  program = basename(strdup(argv0)); /* basename() modifies its argument */
  progcmd = program;
#endif
  return 0;
}

/**
 * Remove blank text nodes in a DOM node tree
 */

int xmlRemoveBlankNodes(xmlNodePtr node) {
  xmlNodePtr child;

  if (node) {
    if (xmlIsBlankNode(node)) {
      DEBUG_CODE(
      xmlChar *pszText = xmlNodeGetContent(node);
      DEBUG_PRINTF(("xmlUnlinkNode(%s)\n", dbgStr(pszText)));
      xmlFree(pszText);
      )
      xmlUnlinkNode(node);
      xmlFreeNode(node);
    } else {
      xmlNodePtr next;
      for (child = node->children; child; child = next) {
      	next = child->next; // Do that before deleting the node!
	xmlRemoveBlankNodes(child);
      }
    }
  }

  return 0;
}

/**
 * Trim spaces around text in a DOM node tree
 *
 * Danger: This may remove significant spaces, in strings that really do have
 *         head or tail spaces
 */

int xmlTrimTextNodes(xmlNodePtr node) {
  xmlNodePtr child;

  if (node) {
    if (node->type == XML_TEXT_NODE) {
      xmlChar *pData = xmlNodeGetContent(node);
      xmlChar *pData0 = pData;
      xmlChar *pc;
      int changed = 0;
      /* Trim the left side */
      for (; *pData && isspace(*pData); pData++) changed = 1;
      /* Trim the right side */
      for (pc = pData+strlen(pData); (pc > pData) && isspace(*(pc-1)); ) {
      	*(--pc) = '\0';
      	changed = 1;
      }
      if (changed) xmlNodeSetContent(node, pData);
      xmlFree(pData0);
    } else {
      for (child = node->children; child; child = child->next) {
	xmlTrimTextNodes(child);
      }
    }
  }

  return 0;
}
