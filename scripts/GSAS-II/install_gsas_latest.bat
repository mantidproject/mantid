@echo off
@echo Running install_gsas_latest.bat
@echo Run with /q flag to quit without prompt after installation complete

@set DEV_PYTHON_EXE=%~dp0..\..\external\src\ThirdParty\lib\python2.7\python.exe
@set RELEASE_PYTHON_EXE=%~dp0..\..\bin\python.exe

if exist %DEV_PYTHON_EXE% (
   @echo Using %DEV_PYTHON_EXE%
   %DEV_PYTHON_EXE% install_gsas_proxy.py -d %CD:~0,3%
) else (if exist %RELEASE_PYTHON_EXE% (
   @echo Using %RELEASE_PYTHON_EXE%
   %RELEASE_PYTHON_EXE% install_gsas_proxy.py -d %CD:~0,3%
) else (
   echo Could not find Mantid Python executable
)
)
if not "%1"=="/q" pause
