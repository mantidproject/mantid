@echo off
setlocal enableextensions
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Launch script for command line python
:: 
:: Sets the required environment variables for the Python to run correctly.
:: All variables that are passed to this script are passed directly to
:: python.exe
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: Set base paths
set _BIN_DIR=%~dp0
:: Strip the trailing slash so that the ~dp expansion works as expected
set _BIN_DIR=%_BIN_DIR:~,-1%
for /f "delims=" %%I in ("%_BIN_DIR%") do (
    set _INSTALL_DIR=%%~dpI
)
set _INSTALL_DIR=%_INSTALL_DIR:~,-1%
set _EXTRA_PATH_DIRS=%_INSTALL_DIR%\bin;%_INSTALL_DIR%\PVPlugins;%_INSTALL_DIR%\plugins

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Required environment variables for Mantid
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set MANTIDPATH=%_BIN_DIR%
set PYTHONPATH=%MANTIDPATH%;%PYTHONPATH%
set PATH=%_EXTRA_PATH_DIRS%;%PATH%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Start python
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
start "Mantid Python" /B /WAIT %_BIN_DIR%\python.exe %*

