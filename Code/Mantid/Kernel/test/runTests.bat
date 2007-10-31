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
cl runner.cpp /I "..\..\..\Third_Party\include" /I "..\.." /EHsc /MTd /W3 /nologo /c /ZI /TP 

link /OUT:"runner.exe" /NOLOGO /LIBPATH:"../../Debug" /LIBPATH:"../../../Third_Party/lib/win32" /DEBUG /PDB:".\runner.pdb" kernel.lib runner.obj

echo "Copying in required dlls..."
copy ..\..\..\Third_Party\lib\win32\*.dll .
copy ..\..\debug\*.dll .
  
echo "Running the tests..."
runner.exe

REM Remove the generated files to ensure that they're not inadvertently run
REM   when something in the chain has failed.
echo "Cleaning up..."
del runner.cpp
del *.obj
del *.pdb
del *.dll
del runner.lib
del runner.ilk
del runner.exp
del vc80.idb
del runner.exe