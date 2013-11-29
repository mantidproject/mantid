@echo off
:: Sets up a python bundle for a given build type
:: Currently the DLLs directory & Lib directory of the source distribution
:: are copied to the final location
if "%1" == "" goto usage
if "%2" == "" goto usage
if "%3" == "" goto usage

echo Setting/updating up bundled Python installation

set SRC_DIR=%2%
set DEST_DIR=%3

:: Module directories. Use xcopy as it will only copy updated/new files
:: We provide a file that allows certain files to be skipped during the copy
mkdir %DEST_DIR%\DLLs
xcopy /Y /E /F /D /I %SRC_DIR%\DLLs %DEST_DIR%\DLLs
if %ERRORLEVEL% == 1 goto :error
mkdir %DEST_DIR%\Lib
xcopy /Y /E /F /D /I %SRC_DIR%\Lib %DEST_DIR%\Lib /EXCLUDE:%~dp0\xcopy_excludes.txt
if %ERRORLEVEL% == 1 goto :error

:: zmq requires that the only a release or debug pyd file in the final zmq directory. Depending on the configuration
:: this will copy in the appropriate pyd and also copy the appropriate python exe & dll files
if "%1" == "Debug" goto :binary_debug
if "%1" == "Release" goto :binary_release
if "%1" == "MinSizeRel" goto :binary_release
if "%1" == "RelWithDebInfo" goto :binary_release
goto :unknown_config

:binary_release
:: Copy release zmq pyd
xcopy /Y /D %SRC_DIR%\Lib\site-packages\zmq\libzmq.pyd %DEST_DIR%\Lib\site-packages\zmq
:: Python binaries
xcopy /Y /D %SRC_DIR%\python.exe %DEST_DIR%
xcopy /Y /D %SRC_DIR%\pythonw.exe %DEST_DIR%
xcopy /Y /D %SRC_DIR%\python27.dll %DEST_DIR%
goto :success

:binary_debug
:: Copy debug zmq pyd
xcopy /Y /D %SRC_DIR%\Lib\site-packages\zmq\libzmq_d.pyd %DEST_DIR%\Lib\site-packages\zmq
:: Python binaries
xcopy /Y /D %SRC_DIR%\python_d.exe %DEST_DIR%
xcopy /Y /D %SRC_DIR%\pythonw_d.exe %DEST_DIR%
xcopy /Y /D %SRC_DIR%\python27_d.dll %DEST_DIR%
goto :success

:unknown_config
echo Unknown configuration passed to %0
exit /b/ 1

:usage
echo Error in script usage. The correct usage is
echo     %0 CONFIG ROOT_PYTHON_DIST_SRC DEST_DIR
echo where 
echo   CONFIG is CMake build type, which should be one of: Debug, Release, MinSizeRel, RelWithDebInfo
echo   ROOT_PYTHON_DIST_SRC is a directory containing the python distribution, i.e executables, the DLLs & Lib directory
echo   DEST_DIR The destination directory where the bundle should end up
goto :eof

:success
goto :eof

:error
echo Error setting up development python distribution
exit /b 1