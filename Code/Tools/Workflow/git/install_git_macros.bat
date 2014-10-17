@echo off
:: Batch script to install the git macros in to the appropriate location.

setlocal

:: Get the install location of Git from the registry
:REG_QUERY
for /f "skip=2 delims=: tokens=1*" %%a in ('reg query "HKLM\SOFTWARE%WOW%\Microsoft\Windows\CurrentVersion\Uninstall\Git_is1" /v InstallLocation 2^> nul') do (
    for /f "tokens=3" %%z in ("%%a") do (
        set GIT_BIN_DIR=%%z:%%b
    )
)

if "%GIT_BIN_DIR%"=="" (
    if "%WOW%"=="" (
        :: Assume we are on 64-bit Windows, so explicitly read the 32-bit Registry.
        set WOW=\Wow6432Node
        goto REG_QUERY
    )
)

echo Found Git at %GIT_BIN_DIR%

:: Define files to be copied
set FILES=git-new git-checkbuild gitworkflow-helpers git-finish git-test git-publish

:: Do copy
FOR %%f IN (%FILES%) DO echo Copying %%f to "%GIT_BIN_DIR%bin\" && xcopy /Y %~dp0%%f "%GIT_BIN_DIR%bin\"

:: Leave the window open just in case any error messages
pause
