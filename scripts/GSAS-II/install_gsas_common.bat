setlocal enableextensions enabledelayedexpansion

@echo off
@echo Running install_gsas_latest.bat
@echo Run with -b flag for non-interactive (quiet) mode
@echo (don't pause after installation and install in Python user site package directory)

:: find python
@set DEV_PYTHON_EXE=%~dp0..\..\external\src\ThirdParty\lib\python2.7\python.exe
@set RELEASE_PYTHON_EXE=%~dp0..\..\bin\python.exe
@set PATH_PYTHON_EXE=
python --version 2>nul
if %ERRORLEVEL%==0 @set PATH_PYTHON_EXE=python

if not "!PATH_PYTHON_EXE!" == "" (
  set PYTHON_EXE=!PATH_PYTHON_EXE!
) else (
  if EXIST "%DEV_PYTHON_EXE%" (
    @set PYTHON_EXE=%DEV_PYTHON_EXE%
  ) else if EXIST %RELEASE_PYTHON_EXE% (
    @set PYTHON_EXE=%RELEASE_PYTHON_EXE%
  ) else (
    @echo Cannot find python executable
    exit /b 1
  )
) 

@echo Using '!PYTHON_EXE!' to install GSAS
!PYTHON_EXE! %~dp0install_gsas_proxy.py %*

if not "%1"=="-b" pause
