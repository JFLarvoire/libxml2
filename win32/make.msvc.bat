@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename        make.msvc.bat                                             *
:#                                                                            *
:#  Description     Front end to nmake, to build the libxml2 lib and tools    *
:#                                                                            *
:#  Notes           Saves the output files, so that they're not overwritten   *
:#                  by builds for other platforms.                            *
:#                                                                            *
:#  History                                                                   *
:#   2021-01-17 JFL Added command-line options parsing.                       *
:#                  Save the last build in a bin\WIN32 or bin\WIN64 subdir.   *
:#   2021-01-19 JFL Factored-out routine :GetLastFileTime.                    *
:#                  Optionally use MAKE_OPTS in configure.default.bat         *
:#                                                                            *
:#*****************************************************************************

setlocal EnableExtensions DisableDelayedExpansion
set "VERSION=2021-01-19"
set "SCRIPT=%~nx0"              &:# Script name
goto :Main

:#----------------------------------------------------------------------------#

:# Get the characteristics of the most recent file in a given directory
:GetLastFileTime %1=[in]DIRNAME %2=[out]VARNAME
set "%~2="
for /f "delims=" %%f in ('dir /b /od %1 2^>NUL') do (
  for /f "delims=" %%l in ('dir "%~1\%%~f" ^| findstr /C:" %%~f"') do set "%~2=%%l"
)
exit /b

:#----------------------------------------------------------------------------#

:Help
echo.
echo %SCRIPT% - Front end to nmake, to build the libxml2 lib and tools
echo.
echo Usage: %SCRIPT% [OPTIONS]
echo.
echo Options:
echo   -?    Display this help
echo   -V    Display this script version
echo   -X    Display the commands generated, but don't run them
echo.
exit /b

:#----------------------------------------------------------------------------#

:Main
set "ARGS="
set "EXEC="

goto :get_arg
:next_arg
shift
:get_arg
if [%1]==[] goto :Start
if "%~1"=="/?" goto :Help
if "%~1"=="-?" goto :Help
if "%~1"=="-V" (echo.%VERSION%) & exit /b
if "%~1"=="-tg" call :GetLastFileTime %2 LASTFILE & set LASTFILE & exit /b
if "%~1"=="-X" set "EXEC=echo" & goto :next_arg
set ARGS=%ARGS% %1
goto :next_arg

:Start

:# Detect a common error that would cause a build failure
if not defined VCINSTALLDIR (
  >&2 echo Error: No build environment defined. Please run vcvarsall.bat in your MS VC installation directory.
  exit /b 1
)

:# Remember what the newest output file was in bin.msvc
call :GetLastFileTime bin.msvc PREVIOUS_LAST

:# Build the nmake options
if defined ARGS set ARGS=%ARGS:~1%
set MAKE_OPTS=%ARGS%
if not defined MAKE_OPTS if exist configure.default.bat call configure.default.bat

:# Run nmake
%EXEC% nmake %MAKE_OPTS%
if errorlevel 1 exit /b                         &:# Exit if nmake failed

:# Check if anything has been updated in bin.msvc
call :GetLastFileTime bin.msvc NEW_LAST
if not defined NEW_LAST exit /b 0               &:# Exit if make clean
if "%PREVIOUS_LAST%"=="%NEW_LAST%" exit /b 0    &:# Exit if nothing changed

:# Save the output, so that it's not overwritten by builds for other platforms

:# Visual Studio 8 does not define Platform. It supports only the x86 platform.
if not defined Platform set "Platform=x86"

:# Choose where to save the output
set "SAVEDIR="
if /i "%Platform%"=="x86"   set "SAVEDIR=bin\WIN32"
if /i "%Platform%"=="x64"   set "SAVEDIR=bin\WIN64"
if /i "%Platform%"=="arm"   set "SAVEDIR=bin\ARM"
if /i "%Platform%"=="arm64" set "SAVEDIR=bin\ARM64"
if not defined SAVEDIR (
  echo Warning: Unexpected platform %Platform%. Please update the make.msvc.bat script.
  set "SAVEDIR=bin\%Platform%"
)

:# Save the output
if not exist %SAVEDIR% (
  %EXEC% md %SAVEDIR%
) else ( :# If we're coming from a clean state, make sure %SAVEDIR% is clean too
  if not defined PREVIOUS_LAST if exist %SAVEDIR%\* (
    echo # Cleaning up %SAVEDIR%...
    %EXEC% del /q %SAVEDIR%\*
  )
)
echo # Copying bin.msvc files to %SAVEDIR%...
%EXEC% xcopy /d /y bin.msvc\* %SAVEDIR%\
