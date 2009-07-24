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
nmake /Q
IF %ERRORLEVEL% NEQ 0 goto qtiploterr

:: Now build qtiplot
cd "%ROOTDIR%\qtiplot"
nmake clean
qmake
nmake
nmake /Q
IF %ERRORLEVEL% NEQ 0 goto qtiploterr
EXIT /B 0

:mantidqterr
echo "MantidQt build failed"
EXIT /B 1

:qtiploterr
echo "MantidPlot build failed"
EXIT /B 1