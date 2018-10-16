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
:: Environment setup
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Source the VS setup script
set VS_VERSION=14
:: 8.1 is backwards compatible with Windows 7. It allows us to target Windows 7
:: when building on newer versions of Windows. This value must be supplied
:: externally and cannot be supplied in the cmake configuration
set SDK_VERSION=8.1
call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64 %SDK_VERSION%
set UseEnv=true
set CM_GENERATOR=Visual Studio 14 2015 Win64

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
  call cmake.exe -G "%CM_GENERATOR%" -DCMAKE_SYSTEM_VERSION=%SDK_VERSION% -DMANTID_DATA_STORE=!MANTID_DATA_STORE! -DDATA_TARGETS_ONLY=ON ..
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
:: Turn off any auto updating on startup
echo UpdateInstrumentDefinitions.OnStartup = 0 > %USERPROPS%
echo usagereports.enabled = 0 >> %USERPROPS%
echo CheckMantidVersion.OnStartup = 0 >> %USERPROPS%

:: Run
set PKGDIR=%WORKSPACE%\build
:: A completely clean build will not have Mantid installed but will need Python to
:: run the testing setup scripts. Assume it is in the PATH
set PYTHON_EXE=python.exe
%PYTHON_EXE% %WORKSPACE%\Testing\SystemTests\scripts\InstallerTests.py -o -d %PKGDIR% -j %BUILD_THREADS% %EXTRA_ARGS%
