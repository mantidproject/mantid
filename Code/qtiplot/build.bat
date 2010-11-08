echo off
setlocal

if "%1" == "" SET ARCH=x86

if /i "%1" == "x86" SET ARCH=x86 && SET QMAKE_ARGS=""
if /i "%1" == "amd64" SET ARCH=amd64 && SET QMAKE_ARGS="CONFIG+=build64"

if /i not %ARCH% == x86 (
    if /i not %ARCH% == amd64 (
        goto usage
    )
)

python release_date.py

rem CALL "%VCINSTALLDIR%"\vcvarsall.bat %ARCH%

:: The top-level directory
SET ROOTDIR=%CD%
pause
:: Now build QtPropertyBrowser
cd "%ROOTDIR%\QtPropertyBrowser"
qmake %QMAKE_ARGS%
pause
nmake clean
nmake
pause

IF %ERRORLEVEL% NEQ 0 goto qtpropertyerr


:: First, build dialog library
cd "%ROOTDIR%\MantidQt"
qmake %QMAKE_ARGS%
nmake clean
nmake
pause
IF %ERRORLEVEL% NEQ 0 goto mantidqterr

:: Now build qtiplot
cd "%ROOTDIR%\qtiplot"
nmake clean
qmake %QMAKE_ARGS%
nmake
pause
IF %ERRORLEVEL% NEQ 0 goto qtiploterr
echo "MantidQt and MantidPlot build succeeded."
EXIT 0

:mantidqterr
echo "MantidQt build failed."
EXIT 1

:qtpropertyerr
echo "QtPropertyBrowser build failed."
EXIT 1

:qtiploterr
echo "MantidPlot build failed."
EXIT 1

:usage
echo "Use either build x86 or build amd64"
