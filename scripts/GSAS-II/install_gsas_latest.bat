@echo off
@echo Running install_gsas_proxy.bat

@set DEV_PYTHON_EXE=%~dp0..\..\external\src\ThirdParty\lib\python2.7\python.exe
@set RELEASE_PYTHON_EXE=%~dp0..\..\bin\python.exe

if exist %DEV_PYTHON_EXE% (
   @echo Using %DEV_PYTHON_EXE%
   %DEV_PYTHON_EXE% install_gsas_proxy.py
) else (if exist %RELEASE_PYTHON_EXE% (
  @echo Using %RELEASE_PYTHON_EXE%
   %RELEASE_PYTHON_EXE% install_gsas_proxy.py
) else (
   echo Could not find Mantid Python executable
)
)
pause
