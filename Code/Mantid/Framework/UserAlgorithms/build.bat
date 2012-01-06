@echo off
setlocal

REM %1 can be an optional --quiet argument that means the script does not 
REM pause at the end. This ensures that if the argument is not --quiet
REM it is passed on to scons
if "%1" == "--quiet" (
  set ONE=""
) else (
  set ONE=%1
)

REM Save where we started
set OLDPWD=%CD%
REM Go to directory of this script
cd %~dp0

REM Build the code
python ../scons-local/scons.py %ONE% %2 %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto end

REM Install libraries
python ../scons-local/scons.py install %ONE% %2 %3 %4 %5 %6 %7 %8 %9


:end
REM Save error level from being overwritten
set ERR=%errorlevel%
REM Make sure we are back where we started
cd %OLDPWD%
REM quiet allows scripts to avoid the asking for input to close the window
if "%1" == "--quiet" (
  exit /b %ERR%
) else (
  pause
)

