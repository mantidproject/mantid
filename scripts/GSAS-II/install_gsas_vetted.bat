@echo off
@echo Running install_gsas_vetted.bat
@echo Run with /q flag to quit without prompt after installation complete

@set DEV_PYTHON_EXE=%~dp0..\..\external\src\ThirdParty\lib\python2.7\python.exe
@set RELEASE_PYTHON_EXE=%~dp0..\..\bin\python.exe
@set VETTED_REVISION_NUMBER=3216

if exist %DEV_PYTHON_EXE% (
   @echo Using %DEV_PYTHON_EXE%
   %DEV_PYTHON_EXE% install_gsas_proxy.py -v %VETTED_REVISION_NUMBER%
) else (if exist %RELEASE_PYTHON_EXE% (
   @echo Using %RELEASE_PYTHON_EXE%
   %RELEASE_PYTHON_EXE% install_gsas_proxy.py -v %VETTED_REVISION_NUMBER%
) else (
   echo Could not find Mantid Python executable
)
)
if not "%1"=="/q" pause
