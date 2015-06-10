setlocal enableextensions enabledelayedexpansion
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WINDOWS SCRIPT TO DRIVE THE DOCUMENTATION TESTS OF MANTID
::
:: Notes:
::
:: WORKSPACE, JOB_NAME & NODE_LABEL are environment variables that
:: are set by Jenkins. The last one corresponds to any labels set on a slave.
:: MANTID_DATA_STORE should be set in the configuration of each slave.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Print out the versions of things we are using
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set CMAKE_BIN_DIR=C:\Program Files (x86)\CMake 2.8\bin
"%CMAKE_BIN_DIR%\cmake" --version
echo %sha1%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Get or update the third party dependencies (basically just to get python)
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
cd %WORKSPACE%\Code
call fetch_Third_Party --libs-only win64
cd %WORKSPACE%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Find and install package
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set PYTHON_EXE=%WORKSPACE%\Code\Third_Party\lib\win64\Python27\python.exe
set INSTALLER_SCRIPT=%WORKSPACE%/Code/Mantid/Testing/SystemTests/scripts/mantidinstaller.py
start "Install package" /B /WAIT %PYTHON_EXE% %INSTALLER_SCRIPT% install %WORKSPACE%
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Update the PATH so that we can find everything for cmake
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set PATH_AT_START=%PATH%
set PATH=%WORKSPACE%\Code\Third_Party\lib\win64;%WORKSPACE%\Code\Third_Party\lib\win64\Python27;%PATH%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Set up the location for local object store outside of the build and source
:: tree, which can be shared by multiple builds.
:: It defaults to a MantidExternalData directory within the USERPROFILE
:: directory. It can be overridden by setting the MANTID_DATA_STORE environment
:: variable.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if NOT DEFINED MANTID_DATA_STORE (
  set MANTID_DATA_STORE=%USERPROFILE%\MantidExternalData
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Setup the build directory
:: Remove build/ExternalData/**: data files will change over time and removing
::                               the links helps keep it fresh
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set BUILD_DIR=%WORKSPACE%\build
if EXIST %BUILD_DIR% (
  rmdir /S /Q %BUILD_DIR%\ExternalData
) else (
  md %BUILD_DIR%
)
cd %BUILD_DIR%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: CMake configuration. We only need the doc-tests targets
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
"%CMAKE_BIN_DIR%\cmake" -G "Visual Studio 11 Win64" -DMANTID_DATA_STORE=!MANTID_DATA_STORE! ..\Code\Mantid

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
msbuild /nologo /nr:false /p:Configuration=Release StandardTestData.vcxproj

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the tests
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Reset the PATH so we don't pick up the 3rd party directory and use the
:: package instead
set PATH=%PATH_AT_START%;C:\MantidInstall\bin

::Remove user properties, disable instrument updating & usage reports and add data paths
del /Q C:\MantidInstall\bin\Mantid.user.properties
echo UpdateInstrumentDefinitions.OnStartup = 0 > C:\MantidInstall\bin\Mantid.user.properties
echo usagereports.enabled = 0 >> C:\MantidInstall\bin\Mantid.user.properties
:: User properties file cannot contain backslash characters
set WORKSPACE_UNIX_STYLE=%WORKSPACE:\=/%
set DATA_ROOT=!WORKSPACE_UNIX_STYLE!/build/ExternalData/Testing/Data
echo datasearch.directories = !DATA_ROOT!/UnitTest;!DATA_ROOT!/DocTest;!WORKSPACE_UNIX_STYLE!/Code/Mantid/instrument >> C:\MantidInstall\bin\Mantid.user.properties

:: Run tests
C:\MantidInstall\bin\MantidPlot.exe -xq %BUILD_DIR%\docs\runsphinx_doctest.py
set RETCODE=!ERRORLEVEL!

start "Remove package" /B /WAIT %PYTHON_EXE% %INSTALLER_SCRIPT% uninstall %WORKSPACE%
if !RETCODE! NEQ 0 exit /B 1
