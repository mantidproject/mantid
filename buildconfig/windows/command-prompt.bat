@echo off
::
:: Sets up the environment configured for Mantid by CMake and
:: starts the appropriate version of Visual Studio

:: Assume the buildenv.bat script exists and is in the same directory
call %~dp0buildenv.bat

:: Start command line
%COMSPEC% /k "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
