:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WINDOWS SCRIPT TO DRIVE THE JENKINS BUILDS OF MANTID.
::
:: Notes:
::
:: WORKSPACE & NODE_LABEL are environment variables that are set by Jenkins.
::   The latter corresponds to any labels set on a slave.
:: BUILD_THREADS should be set in the configuration of each slave.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Test what architecture we are building for
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if "%NODE_LABELS%"=="%NODE_LABELS:win32=%" (
    set ARCH=win64
    set GENERATOR="Visual Studio 11 Win64"
) ELSE (
    set ARCH=win32
    set GENERATOR="Visual Studio 11"
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Get or update the third party dependencies
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
cd %WORKSPACE%\Code
call fetch_Third_Party %ARCH%
cd %WORKSPACE%

set PATH=%WORKSPACE%\Code\Third_Party\lib\%ARCH%;%WORKSPACE%\Code\Third_Party\lib\%ARCH%\Python27;%PATH%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Create the build directory if it doesn't exist
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
md %WORKSPACE%\build
cd %WORKSPACE%\build

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: CMake configuration
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
"C:\Program Files (x86)\CMake 2.8\bin\cmake.exe" -G %GENERATOR% -D CONSOLE=OFF -D ENABLE_CPACK=ON -D USE_PRECOMPILED_HEADERS=ON ..\Code\Mantid

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
msbuild /nologo /m:%BUILD_THREADS% /nr:false /p:Configuration=Release Mantid.sln
if ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the tests
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
"C:\Program Files (x86)\CMake 2.8\bin\ctest.exe" -C Release -j%BUILD_THREADS% --timeout 30 --output-on-failure -E MantidPlot
:: Run GUI tests serially
::ctest -C Release --timeout 30 --output-on-failure -R MantidPlot

