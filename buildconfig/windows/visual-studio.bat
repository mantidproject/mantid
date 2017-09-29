@echo off
::
:: Sets up the environment configured for Mantid by CMake and
:: starts the appropriate version of Visual Studio

:: Assume the buildenv.bat script exists and is in the same directory
call %~dp0buildenv.bat
set MSVC_IDE=@MSVC_IDE_LOCATION@
:: Start IDE
start "" "%MSVC_IDE%/devenv.exe" Mantid.sln
