#!/usr/bin/env bash
set -ex

parent_dir="$(dirname "$RECIPE_DIR")"
bash "${parent_dir}"/archive_env_logs.sh "$BUILD_PREFIX" "$PREFIX" 'mantid'

# Determine if this is an MPI build based on the package name
MPI_BUILD_FLAG="OFF"
if [[ "${PKG_NAME}" == "mantidmpi" ]]; then
  MPI_BUILD_FLAG="ON"
fi

mkdir -p build
cd build

cmake \
  ${CMAKE_ARGS} \
  -DCONDA_BUILD=True \
  -DENABLE_PRECOMMIT=OFF \
  -DMANTID_FRAMEWORK_LIB=BUILD \
  -DMANTID_QT_LIB=OFF \
  -DENABLE_DOCS=OFF \
  -DENABLE_WORKBENCH=OFF \
  -DWORKBENCH_SITE_PACKAGES=$SP_DIR \
  -DCMAKE_FIND_FRAMEWORK=LAST \
  -DCMAKE_CXX_SCAN_FOR_MODULES=OFF \
  -DUSE_PYTHON_DYNAMIC_LIB=OFF \
  -DMPI_BUILD=${MPI_BUILD_FLAG} \
  -GNinja \
  ../

cmake --build .
cmake --build . --target install
