#!/bin/bash
# Simple script to build and run the tests.
# Will run all tests in the directory if no arguments are supplied,
#      or alternatively just the test files given as arguments.
#
# You will need to have the directories containing the Mantid
#      .so libraries in your LD_LIBRARY_PATH environment variable
#
# Author: Owen Arnold 03/11/2010
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

cxxtestgen=../../../Third_Party/src/cxxtest/cxxtestgen.py
python $cxxtestgen --runner=MantidPrinter -o runner.cpp $test_files
echo

echo "Building Subset Library..."

g++ -shared -fPIC -O0 -g3 -Wall -c -fmessage-length=0 -o  libInterfaceVatesMantid.so ../src/InterfaceVatesMantid.C -I ../inc/VisitPlugins -I../../../Mantid/Kernel/inc -I../../../Mantid/MDAlgorithms/inc     -I../../../Mantid/API/inc -I../../../Mantid/Geometry/inc  -I /usr/local/2.1.0/linux-intel/include/vtk/include/vtk-5.0 -L /usr/local/2.1.0/linux-intel/lib -lvtkCommon -lvtkFiltering  -lMantidKernel -lMantidGeometry -lMantidAPI -lMDAlgorithms


echo

echo "Compiling the test executable..."
mantid_libpath=../../../Mantid/debug
visit_libpath=/usr/local/2.1.0/linux-intel/lib
visit_pluginlibpath=/home/spu92482/.visit/linux-intel/plugins/operators
g++ -O0 -g3 -DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG  -o runner.exe runner.cpp -I../../../Mantid/Kernel/inc -I../../../Mantid/MDAlgorithms/inc -I../../../Mantid/API/inc -I../inc/VisitPlugins -I/usr/local/2.1.0/linux-intel/include/vtk/include/vtk-5.0 -I/usr/local/2.1.0/linux-intel/include -I../../../Mantid/Geometry/inc -I ../inc \
    -I ../../../Third_Party/src/cxxtest -I ../../../Third_Party/include -I /usr/local/2.1.0/linux-intel/include/vtk/include/vtk-5.0 -L$mantid_libpath -L$visit_libpath -L$visit_pluginlibpath  -L /usr/local/2.1.0/linux-intel/lib  -L./ -lvtkCommon -lvtkFiltering -lInterfaceVatesMantid -lMantidKernel -lMantidGeometry -lMantidAPI -lboost_date_time-mt -lgmock -lMDAlgorithms 

echo

echo "Running the tests..."
ln ../../../Mantid/Build/Tests/*.properties .
LD_LIBRARY_PATH=$mantid_libpath:$visit_libpath:$LD_LIBRARY_PATH ./runner.exe
echo

# Remove the generated files to ensure that they're not inadvertently run
#   when something in the chain has failed.
echo "Cleaning up..."
rm -f *.properties
rm -f *Test.log
rm -f *.so
echo "Done."
