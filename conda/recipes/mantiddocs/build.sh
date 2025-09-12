#!/usr/bin/env bash
set -ex

parent_dir="$(dirname "$RECIPE_DIR")"
bash "${parent_dir}"/archive_env_logs.sh "$BUILD_PREFIX" "$PREFIX" 'mantiddocs'

mkdir -p build
cd build

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
  -DDOCS_DOTDIAGRAMS=ON \
  -DDOCS_SCREENSHOTS=ON \
  -DDOCS_MATH_EXT=sphinx.ext.mathjax \
  -DDOCS_PLOTDIRECTIVE=ON \
  -DPACKAGE_DOCS=ON \
  -DENABLE_PRECOMMIT=OFF \
  -DCONDA_BUILD=True \
  -DUSE_PYTHON_DYNAMIC_LIB=OFF \
  -DPython_EXECUTABLE=$PYTHON \
  -GNinja \
  ../

cmake --build .

# Use QT_QPA_PLATFORM instead of Xvfb because Xvfb hides a lot of the useful output
QT_QPA_PLATFORM=offscreen cmake --build . --target docs-html

cmake --build . --target install
