@echo off
svn update
cd qtiplot 
IF "%VCINSTALLDIR%"=="" CALL "%VS80COMNTOOLS%"vsvars32
qmake
nmake
if errorlevel 1 echo nmake failed
