@echo on

set "parent_dir=%RECIPE_DIR%\.."
CALL "%parent_dir%\archive_env_logs.bat" %BUILD_PREFIX% %PREFIX% mantid

:: Specify MSVC version to avoid bug in 14.44
CALL "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvarsall.bat" x64 -vcvars_ver=14.38.17.8

mkdir -p build && cd build

cmake ^
    %CMAKE_ARGS% ^
    -DCMAKE_INSTALL_PREFIX=%LIBRARY_PREFIX% ^
    -DCMAKE_INSTALL_LIBDIR=%LIBRARY_LIB% ^
    -DCMAKE_PREFIX_PATH=%LIBRARY_PREFIX% ^
    -DCONDA_BUILD=True ^
    -DENABLE_PRECOMMIT=OFF ^
    -DENABLE_DOCS=OFF ^
    -DWORKBENCH_SITE_PACKAGES=%SP_DIR% ^
    -DMANTID_FRAMEWORK_LIB=BUILD ^
    -DMANTID_QT_LIB=OFF ^
    -DENABLE_WORKBENCH=OFF ^
    ..

if errorlevel 1 exit 1
cmake --build . --config Release
cmake --build . --config Release --target install
if errorlevel 1 exit 1
