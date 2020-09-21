#!/bin/bash -ex

###############################################################################
# Create the build directory if it doesn't exist
###############################################################################
[ -d $WORKSPACE/build ] || mkdir $WORKSPACE/build
cd $WORKSPACE/build

if [ "$(command -v ninja)" ]; then
  CMAKE_GENERATOR="-G Ninja"
elif [ "$(command -v ninja-build)" ]; then
  CMAKE_GENERATOR="-G Ninja"
fi

cmake ${CMAKE_GENERATOR} -DCMAKE_BUILD_TYPE=Debug -DCPPCHECK_GENERATE_XML=TRUE -DCPPCHECK_NUM_THREADS=$BUILD_THREADS -DMAKE_VATES=ON -DParaView_DIR=${PARAVIEW_DIR} ..
cmake --build . --target cppcheck
