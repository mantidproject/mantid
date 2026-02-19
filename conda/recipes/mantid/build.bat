@echo on

set "parent_dir=%RECIPE_DIR%\.."
CALL "%parent_dir%\archive_env_logs.bat" %BUILD_PREFIX% %PREFIX% mantid

:: Specify MSVC version to avoid bug in 14.44
CALL "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvarsall.bat" x64 -vcvars_ver=14.38.17.8

:: Determine MPI build flag (mantidmpi is skipped on Windows, but keep consistent)
set "MPI_BUILD_FLAG=OFF"
if "%PKG_NAME%"=="mantidmpi" set "MPI_BUILD_FLAG=ON"

mkdir -p build && cd build

cmake ^
    %CMAKE_ARGS% ^
    -DCONDA_BUILD=True ^
    -DENABLE_PRECOMMIT=OFF ^
    -DMANTID_FRAMEWORK_LIB=BUILD ^
    -DMANTID_QT_LIB=OFF ^
    -DENABLE_DOCS=OFF ^
    -DENABLE_WORKBENCH=OFF ^
    -DWORKBENCH_SITE_PACKAGES=%SP_DIR% ^
    -DCMAKE_INSTALL_PREFIX=%LIBRARY_PREFIX% ^
    -DCMAKE_INSTALL_LIBDIR=%LIBRARY_LIB% ^
    -DCMAKE_PREFIX_PATH=%LIBRARY_PREFIX% ^
    -DMPI_BUILD=%MPI_BUILD_FLAG% ^
    ..

if errorlevel 1 exit 1
cmake --build . --config Release
cmake --build . --config Release --target install
if errorlevel 1 exit 1
