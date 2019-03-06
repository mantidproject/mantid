@echo off
setlocal enableextensions
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Launch script for MantidPlot
::
:: Sets the required environment variables for MantidPlot to run correctly.
:: All variables that are passed to this script are passed directly to
:: MantidPlot.exe
::
:: It is not advised to start MantidPlot.exe directly as this is unlikely
:: to work.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: Set base paths
set _BIN_DIR=%~dp0
:: Strip the trailing slash so that the ~dp expansion works as expected
set _BIN_DIR=%_BIN_DIR:~,-1%
for /f "delims=" %%I in ("%_BIN_DIR%") do (
    set _INSTALL_DIR=%%~dpI
)
set _INSTALL_DIR=%_INSTALL_DIR:~,-1%
set _EXTRA_PATH_DIRS=%_INSTALL_DIR%\bin;%_INSTALL_DIR%\plugins\qt4

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Required environment variables for Mantid
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set PATH=%_EXTRA_PATH_DIRS%;%PATH%
set PV_PLUGIN_PATH=%_INSTALL_DIR%\plugins\paraview\qt4
:: Matplotlib backend should default to Qt if not set (requires matplotlib >= 1.5)
if "%MPLBACKEND%"=="" (
  set MPLBACKEND=qt4agg
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Start MantidPlot
:: The working directory is whatever is set by the caller
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
start "MantidPlot" /B /WAIT %_BIN_DIR%\MantidPlot.exe %*

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check if Mantidplot exited correctly
:: If not launch the error reporter
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if %errorlevel% NEQ 0 (
set QT_API=pyqt4
python %_INSTALL_DIR%\scripts\ErrorReporter\error_dialog_app.py --exitcode=%errorlevel% --directory=%_BIN_DIR%
)
