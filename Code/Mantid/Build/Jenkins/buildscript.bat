:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WINDOWS SCRIPT TO DRIVE THE JENKINS BUILDS OF MANTID.
::
:: Notes:
::
:: WORKSPACE & JOB_NAME are environment variables that are set by Jenkins.
:: BUILD_THREADS & PARAVIEW_DIR should be set in the configuration of each slave.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

"C:\Program Files (x86)\CMake 2.8\bin\cmake.exe" --version 
echo %sha1%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Get or update the third party dependencies
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
cd %WORKSPACE%\Code
call fetch_Third_Party win64
cd %WORKSPACE%

set PATH=%WORKSPACE%\Code\Third_Party\lib\win64;%WORKSPACE%\Code\Third_Party\lib\win64\Python27;%PARAVIEW_DIR%\bin\%BUILD_CONFIG%;%PATH%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Set up the location for local object store outside of the build and source
:: tree, which can be shared by multiple builds.
:: It defaults to the parent directory of the workspace but can be overridden
:: by setting the MANTID_DATA_STORE environment variable.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if NOT DEFINED MANTID_DATA_STORE (
  for %%F in ("%WORKSPACE%") do set MANTID_DATA_STORE=%%~dpF
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check job requirements from the name
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if "%JOB_NAME%"=="%JOB_NAME:clean=%" (
  set CLEANBUILD=yes
  set BUILDPKG=yes
)
if "%JOB_NAME%"=="%JOB_NAME:pull_requests=%" (
  set BUILDPKG=yes
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Packaging options
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set PACKAGE_DOCS=
if "%BUILDPKG%" EQU "yes" (
  set PACKAGE_DOCS=-DPACKAGE_DOCS=ON
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Setup the build directory
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if "%CLEANBUILD%" EQU "yes" (
  rmdir /S /Q %WORKSPACE%\build
)
md %WORKSPACE%\build
cd %WORKSPACE%\build

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Clean up any artifacts from last build so that if it fails
:: they don't get archived again.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
del *.exe

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
"C:\Program Files (x86)\CMake 2.8\bin\cmake.exe" -G "Visual Studio 11 Win64" -DCONSOLE=OFF -DENABLE_CPACK=ON -DMAKE_VATES=ON -DParaView_DIR=%PARAVIEW_DIR% -DMANTID_DATA_STORE=%MANTID_DATA_STORE% -DUSE_PRECOMPILED_HEADERS=ON %PACKAGE_DOCS% ..\Code\Mantid
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
msbuild /nologo /m:%BUILD_THREADS% /nr:false /p:Configuration=%BUILD_CONFIG% Mantid.sln
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the tests
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
# Remove the user properties file just in case anything polluted it
set USERPROPS=bin\%BUILD_CONFIG%\Mantid.user.properties
del %USERPROPS%
"C:\Program Files (x86)\CMake 2.8\bin\ctest.exe" -C %BUILD_CONFIG% -j%BUILD_THREADS% --schedule-random --output-on-failure
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Create the install kit if required
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if "%BUILDPKG%" EQU "yes" (
  :: Build offline documentation
  msbuild /nologo /nr:false /p:Configuration=%BUILD_CONFIG% docs/docs-qthelp.vcxproj

  :: Ignore errors as the exit code of msbuild is wrong here.
  :: It always marks the build as a failure even thought the MantidPlot exit
  :: code is correct!
  ::if ERRORLEVEL 1 exit /B %ERRORLEVEL%
  cpack -C %BUILD_CONFIG% --config CPackConfig.cmake
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the doc tests when doing a pull request build. Run from a package
:: from a package to have at least one Linux checks it install okay
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if "%JOB_NAME%"=="%JOB_NAME:pull_requests=%" (
  set SYSTEMTEST_DIR=%WORKSPACE%\Code\Mantid\Testing\SystemTest
  :: Install package
  python %SYSTEMTEST_DIR%\scripts\mantidinstaller.py install %WORKSPACE%\build
  cd %WORKSPACE%\build\docs
  :: Run tests
  C:\MantidInstall\bin\MantidPlot.exe -xq runsphinx_doctest.py
  if ERRORLEVEl 1 exit /B %ERRORLEVEL%
  :: Remove
  cd %WORKSPACE%\build
  python %SYSTEMTEST_DIR%\scripts\mantidinstaller.py uninstall %WORKSPACE%\build
)

