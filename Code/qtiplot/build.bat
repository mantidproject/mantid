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
if errorlevel 1 goto mantidqterr

:: Now build qtiplot
cd "%ROOTDIR%\qtiplot"
nmake clean
qmake
nmake
if errorlevel 1 goto qtiploterr
exit(0)

:mantidqterr
echo "MantidQt build failed"
exit(1)

:qtiploterr
echo "MantidPlot build failed"
exit(1)