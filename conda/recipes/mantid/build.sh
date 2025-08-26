#!/usr/bin/env bash
set -ex

parent_dir="$(dirname "$RECIPE_DIR")"

bash "${parent_dir}"/archive_env_logs.sh "$BUILD_PREFIX" "$PREFIX" 'mantid'

mkdir -p build
cd build

# sidestep incorrect flag in rattler-buid v0.46 setting wrong deploy target
# for cross-compiled intel mac
if [[ ${CONDA_TOOLCHAIN_HOST} == x86_64-apple-darwin* ]]; then
    echo "Overriding CMAKE_OSX_DEPLOYMENT_TARGET"
    CMAKE_ARGS=("${CMAKE_ARGS}" "-DCMAKE_OSX_DEPLOYMENT_TARGET=10.13")
fi

cmake \
  ${CMAKE_ARGS} \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$PREFIX \
  -DCMAKE_FIND_FRAMEWORK=LAST \
  -DCMAKE_CXX_SCAN_FOR_MODULES=OFF \
  -DENABLE_DOCS=OFF \
  -DWORKBENCH_SITE_PACKAGES=$SP_DIR \
  -DENABLE_PRECOMMIT=OFF \
  -DCONDA_BUILD=True \
  -DUSE_PYTHON_DYNAMIC_LIB=OFF \
  -DMANTID_FRAMEWORK_LIB=BUILD \
  -DMANTID_QT_LIB=OFF \
  -DENABLE_WORKBENCH=OFF \
  -DPython_EXECUTABLE=$PYTHON \
  -GNinja \
  ../

ninja
ninja install
