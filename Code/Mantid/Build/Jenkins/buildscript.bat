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
:: Find the repository directory. Look for the first directory that contains
:: a .git subdirectory. This assumes that there is only one repository cloned
:: underneath the WORKSPACE directory. Can be overridden by setting
:: the MANTID_REPO_DIR variable
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if "%MANTID_REPO_DIR%"=="" (
  for /f %%F in ('dir /ad /b/s .git') do (
    set MANTID_REPO_DIR=%%~dpF
    goto :loopend
  )
)
:: Break out for loop
:loopend

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
:: Get or update the third party dependencies
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
cd %MANTID_REPO_DIR%\Code
call fetch_Third_Party win64
cd %WORKSPACE%

set PATH=%MANTID_REPO_DIR%\Code\Third_Party\lib\win64;%MANTID_REPO_DIR%\Code\Third_Party\lib\win64\Python27;%PARAVIEW_DIR%\bin\%BUILD_CONFIG%;%PATH%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check whether this is a clean build (must have 'clean' in the job name)
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Set the build directory
set BUILD_DIR=%WORKSPACE%\build

set PACKAGE_DOCS=
if "%JOB_NAME%"=="%JOB_NAME:clean=%" (
    set CLEANBUILD=no
) else  (
    set CLEANBUILD=yes
    set PACKAGE_DOCS=-DPACKAGE_DOCS=True
    rmdir /S /Q %BUILD_DIR%
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Create the build directory if it doesn't exist
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
md %BUILD_DIR%
cd %BUILD_DIR%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: CMake configuration
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
"C:\Program Files (x86)\CMake 2.8\bin\cmake.exe" -G "Visual Studio 11 Win64" -DCONSOLE=OFF -DENABLE_CPACK=ON -DMAKE_VATES=ON -DParaView_DIR=%PARAVIEW_DIR% -DUSE_PRECOMPILED_HEADERS=ON %PACKAGE_DOCS% %MANTID_REPO_DIR%
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
msbuild /nologo /m:%BUILD_THREADS% /nr:false /p:Configuration=%BUILD_CONFIG% Mantid.sln
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the tests
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
"C:\Program Files (x86)\CMake 2.8\bin\ctest.exe" -C %BUILD_CONFIG% -j%BUILD_THREADS% --schedule-random --output-on-failure
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Create the install kit if this is a clean build
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if "%CLEANBUILD%" EQU "yes" (
    :: Build offline documentation
    msbuild /nologo /nr:false /p:Configuration=%BUILD_CONFIG% Code/Mantid/docs/docs-qthelp.vcxproj

    :: ignore errors as the exit code of the build isn't correct
    ::if ERRORLEVEL 1 exit /B %ERRORLEVEL%
    cpack -C %BUILD_CONFIG% --config CPackConfig.cmake
)
