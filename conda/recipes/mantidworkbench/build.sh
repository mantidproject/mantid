#!/usr/bin/env bash
set -ex

parent_dir="$(dirname "$RECIPE_DIR")"
bash "${parent_dir}"/archive_env_logs.sh "$BUILD_PREFIX" "$PREFIX" 'mantidworkbench'


mkdir build
cd build

cmake \
  ${CMAKE_ARGS} \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$PREFIX \
  -DCMAKE_FIND_FRAMEWORK=LAST \
  -DCMAKE_CXX_SCAN_FOR_MODULES=OFF \
  -DMANTID_FRAMEWORK_LIB=SYSTEM \
  -DMANTID_QT_LIB=SYSTEM \
  -DENABLE_WORKBENCH=ON \
  -DENABLE_QTASSISTANT=OFF \
  -DENABLE_DOCS=OFF \
  -DWORKBENCH_SITE_PACKAGES=$SP_DIR \
  -DWORKBENCH_BIN_DIR=$PREFIX/bin \
  -DENABLE_PRECOMMIT=OFF \
  -DCONDA_BUILD=True \
  -DUSE_PYTHON_DYNAMIC_LIB=OFF \
  -DPython_EXECUTABLE=$PYTHON \
  -GNinja \
  ../

cmake --build .
cmake --build . --target install
