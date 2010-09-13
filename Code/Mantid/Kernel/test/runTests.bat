@echo off

IF "%VCINSTALLDIR%"=="" CALL "C:\Program Files\Microsoft Visual Studio 8\VC\vcvarsall.bat"
IF "%MANTIDTESTSETUP%"=="" SET PATH=%CD%\..\..\..\Third_Party\lib\win32;%CD%\..\..\Debug;%PATH%
IF "%MANTIDTESTSETUP%"=="" SET MANTIDTESTSETUP=1
REM Simple script to build and run the tests.
REM Have kept separate from the makefile since that's automatically generated
REM   by Eclipse.
REM
REM Author: Nick Draper, 19/10/07
REM
echo "Generating the source from the test header files..."
IF "%1" == "" GOTO BUILD_ALL ELSE GOTO BUILD_ONE
:BUILD_ONE 
ECHO Building only %1
python ..\..\..\Third_Party\src\cxxtest\cxxtestgen.py  --runner=MantidPrinter -o runner.cpp %1
GOTO COMPILE

:BUILD_ALL 
ECHO Building all .h files
python ..\..\..\Third_Party\src\cxxtest\cxxtestgen.py --runner=MantidPrinter -o runner.cpp *.h
GOTO COMPILE

:COMPILE
echo "Compiling the test executable..."
cl runner.cpp /I "..\inc" /I "..\..\..\Third_Party\include" /I "..\..\..\Third_Party\src\cxxtest" /I "..\.." /I "../../API/inc" /I "..\inc" /D_CRT_SECURE_NO_DEPRECATE /DBOOST_ALL_DYN_LINK /DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG /EHsc /MDd /W3 /wd4275 /nologo /c /ZI /TP 

link /OUT:"runner.exe" /NOLOGO /LIBPATH:"../../Debug" /LIBPATH:"../../../Third_Party/lib/win32" /DEBUG /PDB:".\runner.pdb" Mantidkernel.lib MantidApi.lib runner.obj
  
echo "Copying in properties files..."
copy /Y ..\..\Build\Tests\*properties
echo "Running the tests..."

runner.exe

REM Remove the generated files to ensure that they're not inadvertently run
REM   when something in the chain has failed.
echo "Cleaning up..."
del runner.cpp
del *.obj
del *.pdb
del runner.lib
del runner.ilk
del runner.exp
del vc80.idb
del runner.exe