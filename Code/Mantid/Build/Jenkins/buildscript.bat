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
:: Get or update the third party dependencies
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
cd %WORKSPACE%\Code
call fetch_Third_Party
cd %WORKSPACE%

set PATH=%WORKSPACE%\Code\Third_Party\lib\win64;%WORKSPACE%\Code\Third_Party\lib\win64\Python27;%PATH%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Create the build directory if it doesn't exist
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
md %WORKSPACE%\build
cd %WORKSPACE%\build

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: CMake configuration
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
cmake -G "Visual Studio 11 Win64" -D CONSOLE=OFF -D ENABLE_CPACK=ON -D USE_PRECOMPILED_HEADERS=ON ..\Code\Mantid

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
msbuild /nologo /m:%BUILD_THREADS% /nr:false /p:Configuration=Release Framework\Kernel\test\KernelTest.vcxproj
if ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the tests
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
ctest -C Release -j%BUILD_THREADS% --output-on-failure -R KernelTest_
::ctest -C Release -j%BUILD_THREADS% --timeout 30 --output-on-failure -E MantidPlot
:: Run GUI tests serially
::ctest -C Release --timeout 30 --output-on-failure -R MantidPlot

