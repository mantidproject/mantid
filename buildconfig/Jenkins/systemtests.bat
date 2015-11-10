setlocal enableextensions enabledelayedexpansion
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WINDOWS SCRIPT TO DRIVE THE SYSTEM TESTS OF MANTID
::
:: Notes:
::
:: WORKSPACE, JOB_NAME & NODE_LABEL are environment variables that 
:: are set by Jenkins. The last one corresponds to any labels set on a slave.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Print out the versions of things we are using
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
call cmake --version 
echo %sha1%

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
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
md %WORKSPACE%\build
cd %WORKSPACE%\build

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: CMake configuration if it has not already been configured.
:: We use the special flag that only creates the targets for the data
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if not EXIST %WORKSPACE%\build\CMakeCache.txt (
  call cmake -DMANTID_DATA_STORE=!MANTID_DATA_STORE! -DDATA_TARGETS_ONLY=ON ..
  if ERRORLEVEL 1 exit /b %ERRORLEVEL%
) else (
  :: This ensures that any new data files are picked up by the cmake globbing
  call cmake .
  if ERRORLEVEL 1 exit /b %ERRORLEVEL%
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
msbuild /nologo /nr:false /p:Configuration=Release StandardTestData.vcxproj
if ERRORLEVEL 1 exit /b %ERRORLEVEL%
msbuild /nologo /nr:false /p:Configuration=Release SystemTestData.vcxproj
if ERRORLEVEL 1 exit /b %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the tests
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Remove any Mantid.user.properties file
set USERPROPS=C:\MantidInstall\bin\Mantid.user.properties
del /Q %USERPROPS%
:: Turn off usage reports and instrument downloading for the mantid call
:: that creates the properties file
echo UpdateInstrumentDefinitions.OnStartup = 0 > %USERPROPS%
echo usagereports.enabled = 0 >> %USERPROPS%

:: Run
set PKGDIR=%WORKSPACE%\build
set PATH=C:\MantidInstall\bin;C:\MantidInstall\plugins;%PATH%
python %WORKSPACE%\Testing\SystemTests\scripts\InstallerTests.py -o -d %PKGDIR%
