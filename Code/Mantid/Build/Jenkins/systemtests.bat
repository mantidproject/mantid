setlocal enbaleextensions enabledelayedexpansion
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
set CMAKE_BIN_DIR=C:\Program Files (x86)\CMake 2.8\bin
"%CMAKE_BIN_DIR%\cmake" --version 
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
  "%CMAKE_BIN_DIR%\cmake" -DMANTID_DATA_STORE=!MANTID_DATA_STORE! -DDATA_TARGETS_ONLY=ON ..\Code\Mantid
) else (
  :: This ensures that any new data files are picked up by the cmake globbing
  "%CMAKE_BIN_DIR%\cmake" .
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
"%CMAKE_BIN_DIR%\cmake" --build . -- StandardTestData
"%CMAKE_BIN_DIR%\cmake" --build . -- SystemTestData

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
python %WORKSPACE%\Code\Mantid\Testing\SystemTests\scripts\InstallerTests.py -o -d %PKGDIR%

