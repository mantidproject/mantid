setlocal enableextensions enabledelayedexpansion
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WINDOWS SCRIPT TO DRIVE THE JENKINS BUILDS OF MANTID.
::
:: Notes:
::
:: WORKSPACE & JOB_NAME are environment variables that are set by Jenkins.
:: BUILD_THREADS & PARAVIEW_DIR should be set in the configuration of each slave.
:: CMake, git & git-lfs should be on the PATH
::
:: All nodes currently have PARAVIEW_DIR=5.3.0 and PARAVIEW_NEXT_DIR=5.4.0
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
call cmake.exe --version
echo %sha1%

:: Find the grep tool for later
for /f "delims=" %%I in ('where git') do @set GIT_EXE_DIR=%%~dpI
set GIT_ROOT_DIR=%GIT_EXE_DIR:~0,-4%
set GREP_EXE=%GIT_ROOT_DIR%\usr\bin\grep.exe

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
set PARAVIEW_DIR=%PARAVIEW_DIR%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Pre-processing steps on the workspace itself
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: If the third party repo exists ensure it is in a clean state
:: as updating errors on previous builds can leave it in a state that
:: cmake is unable to cope with
if EXIST %WORKSPACE%\external\src\ThirdParty\.git (
  cd %WORKSPACE%\external\src\ThirdParty
  git reset --hard HEAD
  git clean -fdx
  cd %WORKSPACE%
)

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
:: Check job requirements from the name
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set CLEANBUILD=
set BUILDPKG=
if not "%JOB_NAME%" == "%JOB_NAME:clean=%" (
  set CLEANBUILD=yes
  set BUILDPKG=yes
)

:: BUILD_PACKAGE can be provided as a job parameter on the pull requests
if not "%JOB_NAME%" == "%JOB_NAME:pull_requests=%" (
  if not "%BUILD_PACKAGE%" == "%BUILD_PACKAGE:true=%" (
    set BUILDPKG=yes
  ) else (
    set BUILDPKG=no
  )
)
:: Never want package for debug builds
if not "%JOB_NAME%" == "%JOB_NAME:debug=%" (
  set BUILDPKG=no
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Setup the build directory
:: For a clean build the entire thing is removed to guarantee it is clean. All
:: other build types are assumed to be incremental and the following items
:: are removed to ensure stale build objects don't interfere with each other:
::   - those removed by git clean -fdx --exclude=build
::   - build/bin: if libraries are removed from cmake they are not deleted
::                   from bin and can cause random failures
::   - build/ExternalData/**: data files will change over time and removing
::                            the links helps keep it fresh
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set BUILD_DIR_REL=build
set BUILD_DIR=%WORKSPACE%\%BUILD_DIR_REL%
call %~dp0setupcompiler.bat %BUILD_DIR%

if "!CLEANBUILD!" == "yes" (
  echo Removing build directory for a clean build
  rmdir /S /Q %BUILD_DIR%
)

if EXIST %BUILD_DIR% (
  git clean -fdx --exclude=%BUILD_DIR_REL%
  rmdir /S /Q %BUILD_DIR%\bin %BUILD_DIR%\ExternalData
  for /f %%F in ('dir /b /a-d /S "TEST-*.xml"') do del /Q %%F >/nul
  if "!CLEAN_EXTERNAL_PROJECTS!" == "true" (
    rmdir /S /Q %BUILD_DIR%\eigen-prefix
    rmdir /S /Q %BUILD_DIR%\googletest-download %BUILD_DIR%\googletest-src
    rmdir /S /Q %BUILD_DIR%\python-xmlrunner-download %BUILD_DIR%\python-xmlrunner-src
  )
) else (
  md %BUILD_DIR%
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Packaging options
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set PACKAGE_OPTS=
if "%BUILDPKG%" == "yes" (
  set PACKAGE_OPTS=-DPACKAGE_DOCS=ON -DCPACK_PACKAGE_SUFFIX=
)

cd %BUILD_DIR%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Clean up any artifacts from last build so that if it fails
:: they don't get archived again.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
del /Q *.exe

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check the required build configuration
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set BUILD_CONFIG=
if not "%JOB_NAME%"=="%JOB_NAME:debug=%" (
  set BUILD_CONFIG=Debug
) else (
if not "%JOB_NAME%"=="%JOB_NAME:relwithdbg=%" (
  set BUILD_CONFIG=RelWithDbg
) else (
    set BUILD_CONFIG=Release
    ))

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: CMake configuration
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Note the exception: Vates disabled in Debug mode for now.
if not "%JOB_NAME%"=="%JOB_NAME:debug=%" (
  set VATES_OPT_VAL=OFF
) else (
  set VATES_OPT_VAL=ON
)
call cmake.exe -G "%CM_GENERATOR%" -DCMAKE_SYSTEM_VERSION=%SDK_VERSION% -DCONSOLE=OFF -DENABLE_CPACK=ON -DMAKE_VATES=%VATES_OPT_VAL% -DParaView_DIR=%PARAVIEW_DIR% -DMANTID_DATA_STORE=!MANTID_DATA_STORE! -DENABLE_WORKBENCH=ON -DUSE_PRECOMPILED_HEADERS=ON -DENABLE_FILE_LOGGING=OFF %PACKAGE_OPTS% ..
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
call %BUILD_DIR%\buildenv.bat
msbuild /nologo /m:%BUILD_THREADS% /nr:false /p:Configuration=%BUILD_CONFIG% Mantid.sln
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the tests
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Remove any user configuration and create a blank user properties file
:: This prevents race conditions when creating the user config directory
set USERPROPS=bin\%BUILD_CONFIG%\Mantid.user.properties
del %USERPROPS%
set CONFIGDIR=%APPDATA%\mantidproject\mantid
rmdir /S /Q %CONFIGDIR%
mkdir %CONFIGDIR%
call cmake.exe -E touch %USERPROPS%
call ctest.exe -C %BUILD_CONFIG% -j%BUILD_THREADS% --schedule-random --output-on-failure
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run docs-tests if in the special Debug builds
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo Note: not running doc-test target as it currently takes too long
:: if not "%JOB_NAME%"=="%JOB_NAME:debug=%" (
::   call cmake.exe --build . --target StandardTestData
::   call cmake.exe --build . --target docs-test
:: )

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Create the install kit if required
:: Disabled while it takes 10 minutes to create & 5-10 mins to archive!
:: Just create the docs to check they work
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

if "%BUILDPKG%" == "yes" (
  :: Build offline documentation
  msbuild /nologo /nr:false /p:Configuration=%BUILD_CONFIG% docs/docs-qthelp.vcxproj
  :: Ignore errors as the exit code of msbuild is wrong here.
  :: It always marks the build as a failure even thought the MantidPlot exit
  :: code is correct!
  echo Building package
  cpack.exe -C %BUILD_CONFIG% --config CPackConfig.cmake
)
