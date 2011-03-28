@echo off
SETLOCAL

REM 
REM Build a Windows MSI
REM 

if "%1" == "" goto usage
REM Location of temporary build files
if "%2" == "" (
  SET TMPDIR=%~dp0
) ELSE (
  SET TMPDIR=%CD%\%2
)
if not exist "%TMPDIR%" mkdir "%TMPDIR%"

REM Location of final output
if "%3" == "" (
  SET OUTPUTDIR=%TMPDIR%
) ELSE (
  SET OUTPUTDIR=%CD%\%3
)

if /i %1 == x86 SET MSI_NAME=Mantid-32bit.msi && SET WIX_LOC="C:\Program Files\Windows Installer XML\bin"
if /i %1 == amd64 SET MSI_NAME=Mantid-64bit.msi && SET WIX_LOC="C:\Program Files (x86)\Windows Installer XML\bin"

if /i not %1 == x86 (
    if /i not %1 == amd64 (
        goto usage
    )
)

SET STARTDIR=%CD%
cd "%~dp0"

REM - File names
SET TMPWXS=%TMPDIR%\msi_input.wxs
SET WXSOBJ=%TMPDIR%\wix_obj.wixobj
SET FINALMSI=%OUTPUTDIR%\%MSI_NAME%

REM Clean previous output
echo Cleaning previous output
del "%TMPWXS%" "%WXSOBJ%" "%FINALMSI%"

REM Generate input wxs file
echo Running generateWxs to generate %TMPWXS%
python generateWxs.py "%OUTPUTDIR%" "%TMPDIR%"
if errorlevel 1 goto wxs_error

REM Generate wix object file
echo Running candle to generate %WXSOBJ%
candle -out "%WXSOBJ%" "%TMPWXS%"  
if errorlevel 1 goto candle_error

REM Generate final MSI
echo Running light to generate %FINALMSI%
light -out "%FINALMSI%" "%WXSOBJ%" %WIX_LOC%\wixui.wixlib %WIX_LOC%\wixca.wixlib  -loc WixUI_en-us.wxl
if errorlevel 1 goto light_error

ENDLOCAL
goto :end

:wxs_error
echo "Error creating wxs file"
cd %START_DIR%
goto failed
:candle_error
echo "Error processing wxs file"
cd %START_DIR%
goto failed
:light_error
echo "MSI creation failed"
cd %START_DIR%
goto failed
:usage
echo "Usage: build.bat x86|amd64 [TMPDIR] [OUTPUTDIR]\n\nIf OUTPUTDIR is left blank then TMPDIR is used as the build directory"
cd %START_DIR%
goto failed

:end
echo "Successfully created %FINALMSI%"
echo "MSI build succeeded"
cd %START_DIR%
ENDLOCAL
EXIT /B 0

:failed
ENDLOCAL
EXIT /B 1
