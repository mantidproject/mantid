@echo off
python release_date.py

IF "%VCINSTALLDIR%"=="" CALL "%VS80COMNTOOLS%"vsvars32

:: The top-level directory 
SET ROOTDIR=%CD%

:: First, build dialog library
cd "%ROOTDIR%\MantidQt"
qmake
nmake clean
nmake
IF %ERRORLEVEL% NEQ 0 goto mantidqterr

:: Now build qtiplot
cd "%ROOTDIR%\qtiplot"
nmake clean
qmake
nmake
IF %ERRORLEVEL% NEQ 0 goto qtiploterr
echo "MantidQt and MantidPlot build succeeded."
EXIT 0

:mantidqterr
echo "MantidQt build failed."
EXIT 1

:qtiploterr
echo "MantidPlot build failed."
EXIT 1
