#!/bin/bash -ex

# Create the build directory if it doesn't exist
[ -d $WORKSPACE/build ] || mkdir $WORKSPACE/build
cd $WORKSPACE/build

# Set locale to C for latex
export LC_ALL=C

cmake --version

cmake --preset=doxygen-ci ..

# Use mathjax over generating images. It's faster and we don't view the output here.
echo "USE_MATHJAX = YES " >> Framework/Doxygen/Mantid.doxyfile

# Build doxygen target
cmake --build . --target doxygen

