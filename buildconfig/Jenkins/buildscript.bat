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

:: Set cmake generator
call %~dp0cmakegenerator.bat

:: Find grep
for /f "delims=" %%I in ('where git') do (
  @set _grep_exe=%%~dpI..\usr\bin\grep.exe
  @echo Checking for grep at: !_grep_exe!
  if EXIST "!_grep_exe!" (
    goto :endfor
  ) else (
    @set _grep_exe=""
  )
)
:endfor
if !_grep_exe! == "" (
  @echo Unable to find grep.exe
  exit /b 1
)
@echo Using grep: !_grep_exe!

:: ParaView version
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
  git clean -d -x --force
  cd %WORKSPACE%
)

set BUILD_DIR_REL=build
set BUILD_DIR=%WORKSPACE%\%BUILD_DIR_REL%

:: Clean the source tree to remove stale configured files but make sure to
:: leave external/ and the BUILD_DIR directory intact.
:: There is a later check to see if this is a clean build and remove BUILD_DIR.
git clean -d -x --force --exclude=external --exclude=%BUILD_DIR_REL%

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
::   - build/bin: if libraries are removed from cmake they are not deleted
::                   from bin and can cause random failures
::   - build/ExternalData/**: data files will change over time and removing
::                            the links helps keep it fresh
::   - build/Testing/**: old ctest xml files will change over time and removing
::                       the links helps keep it fresh
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if EXIST %BUILD_DIR%\CMakeCache.txt (
  call "%_grep_exe%" CMAKE_GENERATOR:INTERNAL %BUILD_DIR%\CMakeCache.txt > %BUILD_DIR%\cmake_generator.log
  call "%_grep_exe%" -q "%CM_GENERATOR%" %BUILD_DIR%\cmake_generator.log
  if ERRORLEVEL 1 (
    set CLEANBUILD=yes
    echo Previous build used a different compiler. Performing a clean build.
  ) else (
    set CLEANBUILD=no
    echo Previous build used the same compiler. No need to clean.
  )
)


if "!CLEANBUILD!" == "yes" (
  echo Removing build directory for a clean build
  rmdir /S /Q %BUILD_DIR%
)

if EXIST %BUILD_DIR% (
  rmdir /S /Q %BUILD_DIR%\bin %BUILD_DIR%\ExternalData %BUILD_DIR%\Testing
  for /f %%F in ('dir /b /a-d /S "TEST-*.xml"') do del /Q %%F >/nul
  if "!CLEAN_EXTERNAL_PROJECTS!" == "true" (
    rmdir /S /Q %BUILD_DIR%\eigen-prefix
    rmdir /S /Q %BUILD_DIR%\googletest-download %BUILD_DIR%\googletest-src
  )
) else (
  md %BUILD_DIR%
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Packaging options
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set PACKAGE_OPTS=
if "%BUILDPKG%" == "yes" (
  :: If package name is provided on the Jenkins job, use the custom package name
  :: otherwise determine the correct suffix based on the branch, the else
  :: captures pull requests and they have suffix unstable
  if not "%PACKAGE_SUFFIX%" == "" (
    echo Using PACKAGE_SUFFIX=%PACKAGE_SUFFIX% from job parameter
  ) else if not "%JOB_NAME%" == "%JOB_NAME:release=%" (
    set PACKAGE_SUFFIX=
  ) else if not "%JOB_NAME%" == "%JOB_NAME:master=%" (
    set PACKAGE_SUFFIX=nightly
  ) else (
    set PACKAGE_SUFFIX=unstable
  )
  set PACKAGE_OPTS=-DPACKAGE_DOCS=ON -DCPACK_PACKAGE_SUFFIX=!PACKAGE_SUFFIX!
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
  )
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: CMake configuration
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Note the exception: Vates disabled in Debug mode for now.
if not "%JOB_NAME%"=="%JOB_NAME:debug=%" (
  set VATES_OPT_VAL=OFF
) else (
  set VATES_OPT_VAL=ON
)

call cmake.exe -G "%CM_GENERATOR%" -DCMAKE_SYSTEM_VERSION=%SDK_VERS% -DCONSOLE=OFF -DENABLE_CPACK=ON -DMAKE_VATES=%VATES_OPT_VAL% -DParaView_DIR=%PARAVIEW_DIR% -DMANTID_DATA_STORE=!MANTID_DATA_STORE! -DUSE_PRECOMPILED_HEADERS=ON %PACKAGE_OPTS% ..

if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
call %BUILD_DIR%\thirdpartypaths.bat
cmake --build . -- /nologo /m:%BUILD_THREADS% /verbosity:minimal /p:Configuration=%BUILD_CONFIG%
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
cmake --build . --target AllTests -- /nologo /m:%BUILD_THREADS% /verbosity:minimal /p:Configuration=%BUILD_CONFIG%
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the tests
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Remove any user configuration and create a blank user properties file
:: This prevents race conditions when creating the user config directory
set USERPROPS=bin\%BUILD_CONFIG%\Mantid.user.properties
del %USERPROPS%
set CONFIGDIR=%APPDATA%\mantidproject
rmdir /S /Q %CONFIGDIR%
:: remove old MantidPlot state (sets errorlevel on failure but we don't check so script continues)
reg delete HKCU\Software\Mantid /f
:: create the config directory to avoid any race conditions
mkdir %CONFIGDIR%\mantid
:: use a fixed number of openmp threads to avoid overloading the system
echo MultiThreaded.MaxCores=2 > %USERPROPS%

call ctest.exe -C %BUILD_CONFIG%  --no-compress-output -T Test -j%BUILD_THREADS% --schedule-random --output-on-failure
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run docs-tests if in the special Debug builds
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo Note: not running doc-test target as it currently takes too long
:: if not "%JOB_NAME%"=="%JOB_NAME:debug=%" (
::   del /Q %USERPROPS%
::   call cmake.exe --build . --target StandardTestData
::   call cmake.exe --build . --target docs-test
:: )

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Create the install kit if required
:: Disabled while it takes 10 minutes to create & 5-10 mins to archive!
:: If the install kit needs to be built,  create the docs to check they work
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

if "%BUILDPKG%" == "yes" (
  :: Build offline documentation
  cmake --build . --target docs-qthelp -- /p:Configuration=%BUILD_CONFIG%  /verbosity:minimal
  :: Ignore errors as the exit code of msbuild is wrong here.
  :: It always marks the build as a failure even though MantidPlot exits correctly
  echo Building package
  cpack.exe -C %BUILD_CONFIG% --config CPackConfig.cmake
)
