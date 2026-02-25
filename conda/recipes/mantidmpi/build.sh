#!/usr/bin/env bash
set -ex

parent_dir="$(dirname "$RECIPE_DIR")"
bash "${parent_dir}"/archive_env_logs.sh "$BUILD_PREFIX" "$PREFIX" 'mantidmpi'

mkdir -p build
cd build

cmake \
  ${CMAKE_ARGS} \
  -DCONDA_BUILD=True \
  -DENABLE_PRECOMMIT=OFF \
  -DMANTID_FRAMEWORK_LIB=SYSTEM \
  -DMANTID_QT_LIB=OFF \
  -DENABLE_DOCS=OFF \
  -DENABLE_WORKBENCH=OFF \
  -DWORKBENCH_SITE_PACKAGES=$SP_DIR \
  -DCMAKE_FIND_FRAMEWORK=LAST \
  -DCMAKE_CXX_SCAN_FOR_MODULES=OFF \
  -DUSE_PYTHON_DYNAMIC_LIB=OFF \
  -DPLUGINS_DIR=$PREFIX/plugins \
  -DWORKBENCH_PLUGINS_DIR=$PREFIX/plugins \
  -DMPI_BUILD=ON \
  -DBUILD_SHARED_LIBS=on \
  -GNinja \
  ../

cmake --build . --target MPIAlgorithms
cmake --build . --target install
