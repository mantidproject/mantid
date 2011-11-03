@echo off
SETLOCAL

REM ---------------------------------------------------------------------------
REM Build a Windows MSI
REM ---------------------------------------------------------------------------

REM ---------------------------------------------------------------------------
if "%1" == "" goto usage

REM Architecture
if /i %1 == x86 (
  echo Building 32-bit installer
  set ARCH=x86
  set WIX_LOC="C:\Program Files\Windows Installer XML\bin"
  set WIX_SDK="C:\Program Files\Windows Installer XML\bin\sdk\"
) else (
  if /i %1 == amd64 (
    echo Building 64-bit installer
    set ARCH=amd64
    set WIX_LOC="C:\Program Files (x86)\Windows Installer XML\bin"
    set WIX_SDK="C:\Program Files (x86)\Windows Installer XML\bin\sdk\"
  ) else (
    goto usage
  )
)

REM Location of temporary build files
if "%2" == "" (
  set TMPDIR=%~dp0
) else (
  set TMPDIR=%CD%\%2
)
if not exist "%TMPDIR%" mkdir "%TMPDIR%"

REM Location of final output
if "%3" == "" (
  set OUTPUTDIR=%TMPDIR%
) else (
  set OUTPUTDIR=%CD%\%3
)

REM ---------------------------------------------------------------------------
set STARTDIR=%CD%
cd "%~dp0"

REM - File names
set TMPWXS=%TMPDIR%\msi_input.wxs
set WXSOBJ=%TMPDIR%\wix_obj.wixobj
REM Clean previous output
echo Cleaning previous output old temporary files
del /Q "%TMPWXS%" "%WXSOBJ%"

REM Build our custom action extension
cd MantidMSI
echo Building WIX custom action extension
C:\Windows\Microsoft.NET\Framework\v4.0.30319\msbuild.exe /nologo /verbosity:minimal /p:Configuration="Release" /p:Platform=Win32 /t:MantidMSI MantidMSI.sln
if errorlevel 1 goto msbuild_error
cd ../

REM ---------------------------------------------------------------------------
REM Generate input wxs file. This generates a file called version.txt in the temp directory from which we read the version. It's a bit clunky but
REM but then so is the whole generation script.
echo ""
echo Running generateWxs to generate %TMPWXS%
python generateWxs.py "%OUTPUTDIR%" "%TMPDIR%"
if errorlevel 1 goto wxs_error

REM ---------------------------------------------------------------------------
REM Get the version to construct the file name
set /p VERSION= < "%TMPDIR%\version.txt"
if /i %ARCH% == x86 (
    REM set MSI_NAME=mantid-%VERSION%-win32.msi
    set MSI_NAME=Mantid-32bit.msi
) else (
    REM set MSI_NAME=mantid-%VERSION%-win64.msi
    set MSI_NAME=Mantid-64bit.msi
)
set FINALMSI=%OUTPUTDIR%\%MSI_NAME%
REM Delete an old one of the same name just in case this falls over part way through
del /Q "%FINALMSI%"

REM ---------------------------------------------------------------------------
REM Generate wix object file
echo ""
echo Running candle to generate %WXSOBJ%
candle -out "%WXSOBJ%" "%TMPWXS%"  
if errorlevel 1 goto candle_error

REM ---------------------------------------------------------------------------
REM Generate final MSI
echo ""
echo Running light to generate %FINALMSI%
light -out "%FINALMSI%" "%WXSOBJ%" %WIX_LOC%\wixui.wixlib %WIX_LOC%\wixca.wixlib -loc %WIX_LOC%\WixUI_en-us.wxl
if errorlevel 1 goto light_error

goto :end

REM ---------------------------------------------------------------------------

:msbuild_error
echo "Error building WIX extenion"
cd %START_DIR%
goto failed
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
