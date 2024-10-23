#!/usr/bin/env bash
set -ex

parent_dir="$(dirname "$RECIPE_DIR")"

bash "${parent_dir}"/archive_env_logs.sh "$BUILD_PREFIX" "$PREFIX" 'mantid'

mkdir build
cd build

# Check if the system is Linux and set the extra CMake flag if true
extra_cmake_flags=""
if [[ "$(uname)" == "Linux" ]]; then
    extra_cmake_flags="-DPROFILE_ALGORITHM_LINUX=ON"
fi

cmake \
  ${CMAKE_ARGS} \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$PREFIX \
  -DCMAKE_FIND_FRAMEWORK=LAST \
  -DENABLE_DOCS=OFF \
  -DWORKBENCH_SITE_PACKAGES=$SP_DIR \
  -DENABLE_PRECOMMIT=OFF \
  -DCONDA_BUILD=True \
  -DUSE_PYTHON_DYNAMIC_LIB=OFF \
  -DMANTID_FRAMEWORK_LIB=BUILD \
  -DMANTID_QT_LIB=OFF \
  -DENABLE_WORKBENCH=OFF \
  -DPython_EXECUTABLE=$PYTHON \
  ${extra_cmake_flags} \
  -GNinja \
  ../

ninja
ninja install
