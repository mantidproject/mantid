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

echo "Generating the source file from the test header files..."
# Chaining all tests together can have effects that you don't think of
#  - it's always a good idea to run your new/changed test on its own
test_files=""
if [ $# -eq 0 ]; then
    test_files=*.h
else
    test_files=$*
fi

cxxtestgen=../../../../Third_Party/src/cxxtest/cxxtestgen.py
python $cxxtestgen --runner=MantidPrinter -o runner.cpp $test_files
echo

echo "Compiling the test executable..."
mantid_libpath=/home/owen/workspace/MantidDebug/bin
gmock_libpath=../../../TestingTools/lib/rhel5
g++ -O0 -g3 -DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG  -o runner.exe runner.cpp \
 -I../../Kernel/inc -I../../Geometry/inc -I ../inc -I../../MDDataObjects/inc \
 -I../../MDAlgorithms/inc -I../../API/inc -I../../../TestingTools/include \
 -I ../../../../Third_Party/src/cxxtest \
 -L$mantid_libpath -L$gmock_libpath \
 -lMantidKernel -lMantidGeometry -lMantidAPI -lboost_date_time -lgmock -lMantidMDDataObjects -lhdf5 -lMantidMDAlgorithms

echo

echo "Running the tests..."
ln ../../Build/Tests/*.properties .
LD_LIBRARY_PATH=$mantid_libpath:$LD_LIBRARY_PATH ./runner.exe
#valgrind --leak-check=full --show-reachable=yes --track-origins=yes ~/mantid/Code/Mantid/Vates/VisitPresenters/test/runner.exe
echo

# Remove the generated files to ensure that they're not inadvertently run
#   when something in the chain has failed.
echo "Cleaning up..."
rm -f *.properties
rm -f *Test.log
echo "Done."
