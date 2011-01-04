#!/bin/bash
# Simple script to build and run the tests.
# Will run all tests in the directory if no arguments are supplied,
#      or alternatively just the test files given as arguments.
#
# You will need to have the directories containing the Mantid
#      .so libraries in your LD_LIBRARY_PATH environment variable
#
# Author: Russell Taylor, 07/11/07
#

# Clean up any old executable
rm -rf runner.*
#rm -f *.properties

echo "Generating the source file from the test header files..."
# Chaining all tests together can have effects that you don't think of
#  - it's always a good idea to run your new/changed test on its own
test_files=""
if [ $# -eq 0 ]; then
    test_files=*.h
else
    test_files=$*
fi

# First command line argument can be 1 or 2, to allow to compile two halves of the algorithms in parallel
executable="runner"
if [ "$1" = "1" ]; then
	executable="runner1"
fi
if [ "$1" = "2" ]; then
	executable="runner2"
fi
echo "Compiling to $executable"

cxxtest_dir=../../../TestingTools/cxxtest
cxxtestgen=${cxxtest_dir}/python/scripts/cxxtestgen
python ${cxxtestgen} --runner=MantidPrinter -o $executable.cpp $test_files
echo

echo "Compiling the test executable..."
if [ -z "$MANTIDPATH" ]; then
    mantid_libpath=../../bin
else
    mantid_libpath=$MANTIDPATH
fi
echo "Libraries in $mantid_libpath"
g++ -O0 -g3 -DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG -o $executable.exe $executable.cpp -I../../Kernel/inc -I../../Geometry/inc -I../../API/inc \
    -I../../DataObjects/inc -I ../../DataHandling/inc -I ../../Nexus/inc -I ../../CurveFitting/inc -I ../inc -I${cxxtest_dir} \
    -L$mantid_libpath -lMantidAlgorithms -lMantidKernel -lMantidGeometry -lMantidAPI -lMantidDataObjects -lMantidDataHandling -lMantidNexus -lMantidCurveFitting
echo

echo "Running the tests..."
ln ../../Build/Tests/*.properties .
LD_LIBRARY_PATH=$mantid_libpath:$LD_LIBRARY_PATH ./$executable.exe
echo

# Remove the generated files to ensure that they're not inadvertently run
#   when something in the chain has failed.
echo "Cleaning up..."
rm -f *Test.log
echo "Done."
