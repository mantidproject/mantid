set "parent_dir=%RECIPE_DIR%\.."
CALL "%parent_dir%\archive_env_logs.bat" %BUILD_PREFIX% %PREFIX% mantidworkbench

:: Specify MSVC version to avoid bug in 14.44
CALL "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvarsall.bat" x64 -vcvars_ver=14.38.17.8

mkdir build && cd build

cmake ^
    -DCMAKE_INSTALL_PREFIX=%LIBRARY_PREFIX% ^
    -DCMAKE_INSTALL_LIBDIR=%LIBRARY_LIB% ^
    -DCMAKE_PREFIX_PATH=%LIBRARY_PREFIX% ^
    -DCONDA_BUILD=True ^
    -DENABLE_PRECOMMIT=OFF ^
    -DENABLE_DOCS=OFF ^
    -DMANTID_FRAMEWORK_LIB=SYSTEM ^
    -DMANTID_QT_LIB=SYSTEM ^
    -DENABLE_WORKBENCH=ON ^
    -DWORKBENCH_SITE_PACKAGES=%SP_DIR% ^
    -GNinja ^
    -DCMAKE_BUILD_TYPE=Release ^
    ..

if errorlevel 1 exit 1
ninja
ninja docs-qthelp
ninja install
if errorlevel 1 exit 1
