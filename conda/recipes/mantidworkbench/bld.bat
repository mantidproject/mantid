set "parent_dir=%RECIPE_DIR%\.."
CALL "%parent_dir%\archive_env_logs.bat" %BUILD_PREFIX% %PREFIX% mantidworkbench

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
    -DUSE_PRECOMPILED_HEADERS=OFF ^
    ..

if errorlevel 1 exit 1
cmake --build . --config Release
cmake --build . --config Release --target docs-qthelp
cmake --build . --config Release --target install
if errorlevel 1 exit 1
