#!/usr/bin/env bash
set -ex

parent_dir="$(dirname "$RECIPE_DIR")"
bash "${parent_dir}"/archive_env_logs.sh "$BUILD_PREFIX" "$PREFIX" 'mantiddocs'

mkdir build
cd build

cmake \
  ${CMAKE_ARGS} \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$PREFIX \
  -DCMAKE_FIND_FRAMEWORK=LAST \
  -DMANTID_FRAMEWORK_LIB=SYSTEM \
  -DMANTID_QT_LIB=SYSTEM \
  -DENABLE_WORKBENCH=OFF \
  -DENABLE_DOCS=ON \
  -DDOCS_DOTDIAGRAMS=ON \
  -DDOCS_SCREENSHOTS=ON \
  -DDOCS_MATH_EXT=sphinx.ext.imgmath \
  -DDOCS_PLOTDIRECTIVE=ON \
  -DPACKAGE_DOCS=ON \
  -DENABLE_PRECOMMIT=OFF \
  -DCONDA_BUILD=True \
  -DUSE_PYTHON_DYNAMIC_LIB=OFF \
  -DPython_EXECUTABLE=$PYTHON \
  -GNinja \
  ../

cmake --build .

# Build the StandardTestData target. We need this test data to build docs-qthelp
cmake --build . --target StandardTestData

# Configure the 'datasearch.directories' in the Mantid.properties file so the test data is found
export STANDARD_TEST_DATA_DIR=$SRC_DIR/build/ExternalData/Testing/Data
echo 'datasearch.directories = '$STANDARD_TEST_DATA_DIR'/UnitTest/;'$STANDARD_TEST_DATA_DIR'/DocTest/' >> $PREFIX/bin/Mantid.properties

# Use QT_QPA_PLATFORM instead of Xvfb because Xvfb hides a lot of the useful output
QT_QPA_PLATFORM=offscreen cmake --build . --target docs-qthelp

cmake --build . --target install
