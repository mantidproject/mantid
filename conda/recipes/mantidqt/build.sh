#!/usr/bin/env bash
set -ex

parent_dir="$(dirname "$RECIPE_DIR")"
bash "${parent_dir}"/archive_env_logs.sh "$BUILD_PREFIX" "$PREFIX" 'mantidqt'

mkdir -p build
cd build

# unset LD_PRELOAD as this causes cmake to segfault
LD_PRELOAD="" \
  cmake \
  ${CMAKE_ARGS} \
  -DCMAKE_FIND_FRAMEWORK=LAST \
  -DCMAKE_CXX_SCAN_FOR_MODULES=OFF \
  -DENABLE_DOCS=OFF \
  -DWORKBENCH_SITE_PACKAGES=$SP_DIR \
  -DENABLE_PRECOMMIT=OFF \
  -DCONDA_BUILD=True \
  -DUSE_PYTHON_DYNAMIC_LIB=OFF \
  -DMANTID_FRAMEWORK_LIB=SYSTEM \
  -DMANTID_QT_LIB=BUILD \
  -DENABLE_WORKBENCH=OFF \
  -GNinja \
  ../

cmake --build .
cmake --build . --target install
