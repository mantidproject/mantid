#!/usr/bin/env bash
set -ex

mkdir tbuild
cd tbuild

# unset LD_PRELOAD as this causes cmake to segfault
LD_PRELOAD="" \
cmake \
  ${CMAKE_ARGS} \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$PREFIX \
  -DCMAKE_FIND_FRAMEWORK=LAST \
  -DCMAKE_CXX_SCAN_FOR_MODULES=OFF \
  -DMANTID_FRAMEWORK_LIB=SYSTEM \
  -DMANTID_QT_LIB=SYSTEM \
  -DENABLE_WORKBENCH=OFF \
  -DENABLE_DOCS=ON \
  -DENABLE_QTASSISTANT=OFF \
  -DDOCS_DOTDIAGRAMS=OFF \
  -DDOCS_SCREENSHOTS=OFF \
  -DDOCS_MATH_EXT=sphinx.ext.imgmath \
  -DDOCS_PLOTDIRECTIVE=OFF \
  -DPACKAGE_DOCS=OFF \
  -DENABLE_PRECOMMIT=OFF \
  -DCONDA_BUILD=True \
  -DUSE_PYTHON_DYNAMIC_LIB=OFF \
  -DPython_EXECUTABLE=$PYTHON \
  -DExternalData_SHOW_PROGRESS=ON \
  -DCMAKE_MESSAGE_LOG_LEVEL=DEBUG \
  -DExternalData_DEBUG_DOWNLOAD=1 \
  -GNinja \
  ../

cmake --build .

# Build the StandardTestData target.
# This might not be strictly necessary if docs-qthelp target is not built due to DOCS_QTHELP=OFF
cmake --build . --target StandardTestData --verbose
