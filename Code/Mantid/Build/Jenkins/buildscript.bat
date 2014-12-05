:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WINDOWS SCRIPT TO DRIVE THE JENKINS BUILDS OF MANTID.
::
:: Notes:
::
:: WORKSPACE & JOB_NAME are environment variables that are set by Jenkins.
:: BUILD_THREADS & PARAVIEW_DIR should be set in the configuration of each slave.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: If pvnext in JOB_NAME, use PARAVIEW_NEXT_DIR for PARAVIEW_DIR
:: Also, occasionally pvnext needs a different build type. Get it from 
:: PVNEXT_BUILD_TYPE
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if NOT "%JOB_NAME%"=="%JOB_NAME:pvnext=%" (
    set PARAVIEW_DIR=%PARAVIEW_NEXT_DIR%
    set BUILD_TYPE=%PVNEXT_BUILD_TYPE%
) else (
    set BUILD_TYPE=Release
)

"C:\Program Files (x86)\CMake 2.8\bin\cmake.exe" --version 
echo %sha1%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check the required build configuration
:: Use RelWithDbgInfo unless specified by job name
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set BUILD_CONFIG=
if not "%JOB_NAME%"=="%JOB_NAME:debug=%" (
    set BUILD_CONFIG=Debug
) else (
if not "%JOB_NAME%"=="%JOB_NAME:release=%" (
    set BUILD_CONFIG=Release
) else (
    set BUILD_CONFIG=RelWithDebInfo
    ))

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Get or update the third party dependencies
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
cd %WORKSPACE%\Code
call fetch_Third_Party win64
cd %WORKSPACE%

set PATH=%WORKSPACE%\Code\Third_Party\lib\win64;%WORKSPACE%\Code\Third_Party\lib\win64\Python27;%PARAVIEW_DIR%\bin\%BUILD_CONFIG%;%PATH%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check whether this is a clean build (must have 'clean' in the job name)
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set PACKAGE_DOCS=
if "%JOB_NAME%"=="%JOB_NAME:clean=%" (
    set CLEANBUILD=no
) else  (
    set CLEANBUILD=yes
    set PACKAGE_DOCS=-DPACKAGE_DOCS=True
    rmdir /S /Q build
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Create the build directory if it doesn't exist
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
md %WORKSPACE%\build
cd %WORKSPACE%\build

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: CMake configuration
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
"C:\Program Files (x86)\CMake 2.8\bin\cmake.exe" -G "Visual Studio 11 Win64" -DCONSOLE=OFF -DENABLE_CPACK=ON -DMAKE_VATES=ON -DParaView_DIR=%PARAVIEW_DIR% -DUSE_PRECOMPILED_HEADERS=ON %PACKAGE_DOCS% ..\Code\Mantid
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

msbuild /nologo /m:%BUILD_THREADS% /nr:false /p:Configuration=%BUILD_CONFIG% Mantid.sln
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the tests
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
"C:\Program Files (x86)\CMake 2.8\bin\ctest.exe" -C %BUILD_CONFIG% -j%BUILD_THREADS% --schedule-random --output-on-failure -E MantidPlot
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
:: Run GUI tests serially
:: ctest -C %BUILD_CONFIG% --output-on-failure -R MantidPlot
:: if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Create the install kit if this is a clean build
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if "%CLEANBUILD%" EQU "yes" (
    :: Build offline documentation
    msbuild /nologo /nr:false /p:Configuration=%BUILD_CONFIG% docs/docs-qthelp.vcxproj

    :: ignore errors as the exit code of the build isn't correct
    ::if ERRORLEVEL 1 exit /B %ERRORLEVEL%
    cpack -C %BUILD_CONFIG% --config CPackConfig.cmake
)
