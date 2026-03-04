#!/usr/bin/env bash
set -ex

if [ -z "${MANTID_DATA_STORE}" ]; then
    MANTID_DATA_STORE="${HOME}"/MantidExternalData
fi
echo "Using MANTID_DATA_STORE=${MANTID_DATA_STORE}"

parent_dir="$(dirname "$RECIPE_DIR")"
bash "${parent_dir}"/archive_env_logs.sh "$BUILD_PREFIX" "$PREFIX" 'mantiddocs'

mkdir -p build
cd build

cmake \
  ${CMAKE_ARGS} \
  -DCMAKE_FIND_FRAMEWORK=LAST \
  -DCMAKE_CXX_SCAN_FOR_MODULES=OFF \
  -DMANTID_FRAMEWORK_LIB=SYSTEM \
  -DMANTID_QT_LIB=SYSTEM \
  -DENABLE_WORKBENCH=OFF \
  -DENABLE_DOCS=ON \
  -DDOCS_DOTDIAGRAMS=ON \
  -DDOCS_SCREENSHOTS=ON \
  -DDOCS_MATH_EXT=sphinx.ext.mathjax \
  -DDOCS_PLOTDIRECTIVE=ON \
  -DPACKAGE_DOCS=ON \
  -DENABLE_PRECOMMIT=OFF \
  -DCONDA_BUILD=True \
  -DUSE_PYTHON_DYNAMIC_LIB=OFF \
  -DMANTID_DATA_STORE=${MANTID_DATA_STORE} \
  -GNinja \
  ../

# Configure the 'datasearch.directories' in the Mantid.properties file so the test data is found
# Docs should only require DocTestData
if [ -f "${PREFIX}"/bin/Mantid.properties ]; then
    echo "Adding data directories to ${PREFIX}/bin/Mantid.properties"
    export STANDARD_TEST_DATA_DIR=$SRC_DIR/build/ExternalData/Testing/Data
    echo 'datasearch.directories = '$STANDARD_TEST_DATA_DIR'/DocTest/' >> $PREFIX/bin/Mantid.properties
else
    echo "Failed to find properties file at ${PREFIX}/bin/Mantid.properties"
    exit 1
fi

# Use MANTIDPROPERTIES to pick up the properties file that was modified
# Use QT_QPA_PLATFORM instead of Xvfb because Xvfb hides a lot of the useful output
MANTIDPROPERTIES="${PREFIX}"/bin/Mantid.properties QT_QPA_PLATFORM=offscreen cmake --build . --target docs-html

# install the docs into the final location
cmake --build . --target install
