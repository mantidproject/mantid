@echo off

SETLOCAL
if "%1" == "" set ARCH=x86

if /i %1 == x86 SET ARCH=x86 && SET QMAKE_ARGS=""
if /i %1 == amd64 SET ARCH=amd64 && SET QMAKE_ARGS="CONFIG+=build64"

if /i not %1 == x86 (
    if /i not %1 == amd64 (
        goto usage
    )
)

python release_date.py

CALL "%VCINSTALLDIR%"\vcvarsall.bat %ARCH%

:: The top-level directory 
SET ROOTDIR=%CD%

:: First, build dialog library
cd "%ROOTDIR%\MantidQt"
qmake %QMAKE_ARGS%
nmake clean
nmake
IF %ERRORLEVEL% NEQ 0 goto mantidqterr

:: Now build QtPropertyBrowser
cd "%ROOTDIR%\QtPropertyBrowser"
qmake %QMAKE_ARGS%
nmake clean
nmake
IF %ERRORLEVEL% NEQ 0 goto qtpropertyerr

:: Now build qtiplot
cd "%ROOTDIR%\qtiplot"
nmake clean
qmake %QMAKE_ARGS%
nmake
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
