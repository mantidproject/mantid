#!/bin/bash -ex

# Create the build directory if it doesn't exist
[ -d $WORKSPACE/build ] || mkdir $WORKSPACE/build
cd $WORKSPACE/build

# Set locale to C for latex
export LC_ALL=C

cmake --preset=doxygen-ci ..

# Build doxygen target
cmake --build . --target doxygen

