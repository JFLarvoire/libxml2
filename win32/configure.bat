@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename        configure.bat                                             *
:#                                                                            *
:#  Description     Front end to configure.js                                 *
:#                                                                            *
:#  Notes                                                                     *
:#                                                                            *
:#  Authors     JFL "Jean-François Larvoire" <jf.larvoire@free.fr>            *
:#                                                                            *
:#  History                                                                   *
:#   2017-11-25 JFL Created this script.                                      *
:#   2021-01-19 JFL Added the optional configure.default.bat.                 *
:#                                                                            *
:#*****************************************************************************

setlocal EnableExtensions DisableDelayedExpansion
set CONFIGURE_OPTS=%*
if not defined CONFIGURE_OPTS if exist configure.default.bat call configure.default.bat
cscript configure.js %CONFIGURE_OPTS%
