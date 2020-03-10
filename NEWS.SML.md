# SML-Specific Change Log

Major changes for the sml2 tool, and SML-specific changes in libxml2.

## [Unreleased] 2020-03-10
### Changed
- sml2.c: Pass head spaces through unchanged.
- parser.c: Improved the markup type detection heuristic, to better detect short SML snippets.

### Fixed
- sml2.c: Fixed the parsing of the - special arguments.

## [Unreleased] 2020-03-03
### Changed
- Merged in changes from libxml2 master branch head (version 2.9.10 + subsequent changes until today).

## [v2.9.8+SML-u1] 2018-12-04
- sml2.c: Ignore parser errors by default.

## [v2.9.8+SML] 2018-04-08
- Initial release: Libxml2 2.9.8 with support for SML

