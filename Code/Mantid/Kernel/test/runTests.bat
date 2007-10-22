@echo off
REM IF "%VCINSTALLDIR%"=="" SET PATH=%path%;C:\Mantid\Code\Third_Party\lib\win32
IF "%VCINSTALLDIR%"=="" CALL "C:\Program Files\Microsoft Visual Studio 8\VC\vcvarsall.bat"
REM Simple script to build and run the tests.
REM Have kept separate from the makefile since that's automatically generated
REM   by Eclipse.
REM
REM Author: Nick Draper, 19/10/07
REM
echo "Generating the source from the test header files..."
python ..\..\..\Third_Party\src\cxxtest\cxxtestgen.py --error-printer -o runner.cpp *.h

echo "Compiling the test executable..."
devenv CxxTest_2_Build.vcproj /BUILD "Debug|Win32"

copy ..\..\..\Third_Party\lib\win32\*.dll .
 
REM echo "Running the tests..."
runner.exe
REM Remove the generated files to ensure that they're not inadvertently run
REM   when something in the chain has failed.
devenv CxxTest_2_Build.vcproj /CLEAN
del runner.cpp
del BuildLog.htm
del *.dll