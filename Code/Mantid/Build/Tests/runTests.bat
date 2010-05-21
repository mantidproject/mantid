@echo off
IF "%VCINSTALLDIR%"=="" SET PATH=%CD%\..\..\..\Third_Party\lib\win32;%CD%\..\..\release;%PATH%
IF "%VCINSTALLDIR%"=="" CALL "C:\Program Files\Microsoft Visual Studio 8\VC\vcvarsall.bat"

IF "%1" == "" GOTO RUN_ALL ELSE GOTO RUN_ONE
:BUILD_ONE 
ECHO Running only %1
ECHO %1 & %1
GOTO EXIT

:RUN_ALL 
ECHO Running all .exe files
FOR %%f IN (*.exe) DO (ECHO %%f & %%f)
GOTO EXIT

:EXIT
@echo on