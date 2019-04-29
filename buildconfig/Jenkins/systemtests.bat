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

:: CMake generator
call %~dp0cmakegenerator.bat

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

:: Remove (possibly) stale files
::   build/ExternalData/**: data files will change over time and removing
::                          the links helps keep it fresh
rmdir /S /Q %WORKSPACE%\build\ExternalData

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: CMake configuration if it has not already been configured.
:: We use the special flag that only creates the targets for the data
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if not EXIST %WORKSPACE%\build\CMakeCache.txt (
  call cmake.exe -G "%CM_GENERATOR%" -DCMAKE_SYSTEM_VERSION=%SDK_VERS% -DMANTID_DATA_STORE=!MANTID_DATA_STORE! -DDATA_TARGETS_ONLY=ON ..
  if ERRORLEVEL 1 exit /b %ERRORLEVEL%
) else (
  :: This ensures that any new data files are picked up by the cmake globbing
  call cmake.exe .
  if ERRORLEVEL 1 exit /b %ERRORLEVEL%
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
call cmake.exe --build . --target StandardTestData -- /nologo /p:Configuration=Release /verbosity:minimal
if ERRORLEVEL 1 exit /b %ERRORLEVEL%
call cmake.exe --build . --target SystemTestData -- /nologo /p:Configuration=Release /verbosity:minimal
if ERRORLEVEL 1 exit /b %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the tests
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Remove any Mantid.user.properties file
set USERPROPS_RELEASE=C:\MantidInstall\bin\Mantid.user.properties
set USERPROPS_NIGHTLY=C:\MantidNightlyInstall\bin\Mantid.user.properties
del /Q %USERPROPS_RELEASE% %USERPROPS_NIGHTLY%
:: Remove user settings
set CONFIGDIR=%APPDATA%\mantidproject
rmdir /S /Q %CONFIGDIR%
:: Create the directory to avoid any race conditions
mkdir %CONFIGDIR%\mantid

:: Turn off any auto updating on startup
echo UpdateInstrumentDefinitions.OnStartup = 0 > %USERPROPS_RELEASE%
echo usagereports.enabled = 0 >> %USERPROPS_RELEASE%
echo CheckMantidVersion.OnStartup = 0 >> %USERPROPS_RELEASE%
:: Nightly
echo UpdateInstrumentDefinitions.OnStartup = 0 > %USERPROPS_NIGHTLY%
echo usagereports.enabled = 0 >> %USERPROPS_NIGHTLY%
echo CheckMantidVersion.OnStartup = 0 >> %USERPROPS_NIGHTLY%

:: Run
set PKGDIR=%WORKSPACE%\build
:: A completely clean build will not have Mantid installed but will need Python to
:: run the testing setup scripts. Assume it is in the PATH
set NTHREADS=%BUILD_THREADS%
set MAXTHREADS=12
if %NTHREADS% gtr %MAXTHREADS% set NTHREADS=%MAXTHREADS%
set PYTHON_EXE=python.exe
%PYTHON_EXE% %WORKSPACE%\Testing\SystemTests\scripts\InstallerTests.py -o -d %PKGDIR% -j %NTHREADS% %EXTRA_ARGS%
