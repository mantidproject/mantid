#!/usr/bin/env bash
set -ex

parent_dir="$(dirname "$RECIPE_DIR")"
bash "${parent_dir}"/archive_env_logs.sh "$BUILD_PREFIX" "$PREFIX" 'mantidworkbench'


mkdir -p build
cd build

cmake \
  ${CMAKE_ARGS} \
  -DCONDA_BUILD=True \
  -DENABLE_PRECOMMIT=OFF \
  -DMANTID_FRAMEWORK_LIB=SYSTEM \
  -DMANTID_QT_LIB=SYSTEM \
  -DENABLE_DOCS=OFF \
  -DENABLE_WORKBENCH=ON \
  -DWORKBENCH_SITE_PACKAGES=$SP_DIR \
  -DCMAKE_FIND_FRAMEWORK=LAST \
  -DCMAKE_CXX_SCAN_FOR_MODULES=OFF \
  -DWORKBENCH_BIN_DIR=$PREFIX/bin \
  -DUSE_PYTHON_DYNAMIC_LIB=OFF \
  -GNinja \
  ../

cmake --build .
cmake --build . --target install
