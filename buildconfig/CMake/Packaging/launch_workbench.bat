@echo off
setlocal enableextensions
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Launch script for Mantid Workbench
::
:: This is provided as a backup way to start the Workbench - the default
:: way being through the provided executable or the PowerShell script.
::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: Set base paths
set _BIN_DIR=%~dp0
:: Strip the trailing slash so that the ~dp expansion works as expected
set _BIN_DIR=%_BIN_DIR:~,-1%
for /f "delims=" %%I in ("%_BIN_DIR%") do (
    set _INSTALL_DIR=%%~dpI
)
set _INSTALL_DIR=%_INSTALL_DIR:~,-1%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Start MantidPlot
:: The working directory is whatever is set by the caller
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
start "Workbench" /B /WAIT %_BIN_DIR%\pythonw.exe %_BIN_DIR%\launch_workbench.pyw

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check if Mantidplot exited correctly
:: If not launch the error reporter
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if %errorlevel% NEQ 0 (
python %_INSTALL_DIR%\scripts\ErrorReporter\error_dialog_app.py --exitcode=%1 --directory=%_BIN_DIR%
)