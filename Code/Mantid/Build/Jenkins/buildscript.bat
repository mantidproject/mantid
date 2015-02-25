setlocal enableextensions enabledelayedexpansion
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WINDOWS SCRIPT TO DRIVE THE JENKINS BUILDS OF MANTID.
::
:: Notes:
::
:: WORKSPACE & JOB_NAME are environment variables that are set by Jenkins.
:: BUILD_THREADS & PARAVIEW_DIR should be set in the configuration of each slave.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set CMAKE_BIN_DIR=C:\Program Files (x86)\CMake 2.8\bin
"%CMAKE_BIN_DIR%\cmake.exe" --version
echo %sha1%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Get or update the third party dependencies
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
cd %WORKSPACE%\Code
call fetch_Third_Party win64
cd %WORKSPACE%

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
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set CLEANBUILD=
set BUILDPKG=
if not "%JOB_NAME%" == "%JOB_NAME:clean=%" (
  set CLEANBUILD=yes
  set BUILDPKG=yes
)

if EXIST %WORKSPACE%\build\CMakeCache.txt (
  FINDSTR "Code/Mantid/TestingTools/cxxtest" %WORKSPACE%\build\CMakeCache.txt && (
    rmdir /S /Q %WORKSPACE%\build
  )
)

if not "%JOB_NAME%" == "%JOB_NAME:pull_requests=%" (
  set BUILDPKG=yes
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Packaging options
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set PACKAGE_DOCS=
if "%BUILDPKG%" == "yes" (
  set PACKAGE_DOCS=-DPACKAGE_DOCS=ON
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Setup the build directory
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if "%CLEANBUILD%" == "yes" (
  rmdir /S /Q %WORKSPACE%\build
)
md %WORKSPACE%\build
cd %WORKSPACE%\build

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
:: Update the PATH so that we can find everything
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set PATH=%WORKSPACE%\Code\Third_Party\lib\win64;%WORKSPACE%\Code\Third_Party\lib\win64\Python27;%PARAVIEW_DIR%\bin\%BUILD_CONFIG%;%PATH%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: CMake configuration
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
"%CMAKE_BIN_DIR%\cmake.exe" -G "Visual Studio 11 Win64" -DCONSOLE=OFF -DENABLE_CPACK=ON -DMAKE_VATES=ON -DParaView_DIR=%PARAVIEW_DIR% -DMANTID_DATA_STORE=!MANTID_DATA_STORE! -DUSE_PRECOMPILED_HEADERS=ON %PACKAGE_DOCS% ..\Code\Mantid
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
msbuild /nologo /m:%BUILD_THREADS% /nr:false /p:Configuration=%BUILD_CONFIG% Mantid.sln
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the tests
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Remove the user properties file just in case anything polluted it
set USERPROPS=bin\%BUILD_CONFIG%\Mantid.user.properties
del %USERPROPS%
"%CMAKE_BIN_DIR%\ctest.exe" -C %BUILD_CONFIG% -j%BUILD_THREADS% --schedule-random --output-on-failure
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Create the install kit if required
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if "%BUILDPKG%" == "yes" (
  echo Building package
  :: Build offline documentation
  msbuild /nologo /nr:false /p:Configuration=%BUILD_CONFIG% docs/docs-qthelp.vcxproj

  :: Ignore errors as the exit code of msbuild is wrong here.
  :: It always marks the build as a failure even thought the MantidPlot exit
  :: code is correct!
  ::if ERRORLEVEL 1 exit /B %ERRORLEVEL%
  "%CMAKE_BIN_DIR%\cpack.exe" -C %BUILD_CONFIG% --config CPackConfig.cmake
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the doc tests when doing a pull request build. Run from a package
:: from a package to have at least one Linux checks it install okay
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if not "%JOB_NAME%"=="%JOB_NAME:pull_requests=%" (
  :: Install package
  set SYSTEMTESTS_DIR=%WORKSPACE%\Code\Mantid\Testing\SystemTests
  python !SYSTEMTESTS_DIR!\scripts\mantidinstaller.py install %WORKSPACE%\build

  ::Remove user properties, disable instrument updating & usage reports and add data paths
  del /Q C:\MantidInstall\bin\Mantid.user.properties
  echo UpdateInstrumentDefinitions.OnStartup = 0 > C:\MantidInstall\bin\Mantid.user.properties
  echo usagereports.enabled = 0 >> C:\MantidInstall\bin\Mantid.user.properties
  :: User properties file cannot contain backslash characters
  set WORKSPACE_UNIX_STYLE=%WORKSPACE:\=/%
  set DATA_ROOT=!WORKSPACE_UNIX_STYLE!/build/ExternalData/Testing/Data
  echo datasearch.directories = !DATA_ROOT!/UnitTest;!DATA_ROOT!/DocTest;!WORKSPACE_UNIX_STYLE!/Code/Mantid/instrument >> C:\MantidInstall\bin\Mantid.user.properties

  :: Run tests
  cd %WORKSPACE%\build\docs
  C:\MantidInstall\bin\MantidPlot.exe -xq runsphinx_doctest.py
  set RETCODE=!ERRORLEVEL!

  :: Remove Mantid
  cd %WORKSPACE%\build
  python !SYSTEMTESTS_DIR!\scripts\mantidinstaller.py uninstall %WORKSPACE%\build
  if !RETCODE! NEQ 0 exit /B 1
)

