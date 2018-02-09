@echo off
@echo Running install_gsas_latest.bat
@echo Run with -b flag for non-interactive (quiet) mode
@echo (don't pause after installation and install in Python user site package directory)

@set DEV_PYTHON_EXE=%~dp0..\..\external\src\ThirdParty\lib\python2.7\python.exe
@set RELEASE_PYTHON_EXE=%~dp0..\..\bin\python.exe

if exist %DEV_PYTHON_EXE% goto runDev

if exist %RELEASE_PYTHON_EXE% goto runRelease

@echo Could not find Mantid Python executable
goto commonExit

:runDev
%DEV_PYTHON_EXE% install_gsas_proxy.py %*
goto commonExit

:runRelease
%RELEASE_PYTHON_EXE% install_gsas_proxy.py %*
goto commonExit

:commonExit
if not "%1"=="-b" pause
