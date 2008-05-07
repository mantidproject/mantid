@echo off
cd qtiplot
IF "%VCINSTALLDIR%"=="" CALL "%VS80COMNTOOLS%"vsvars32 
qmake
nmake
