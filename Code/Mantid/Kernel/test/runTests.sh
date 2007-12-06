#!/bin/bash
# Simple script to build and run the tests.
# Will run all tests in the directory if no arguments are supplied,
#      or alternatively just the test files given as arguments.
#
# This script is optimised for linuxs1 (i.e. it probably won't work anywhere else!)
#
# You will need to have the directories containing the Mantid and Third Party 
#      .so libraries in your LD_LIBRARY_PATH environment variable
#
# Author: Russell Taylor, 07/11/07
#

echo "Generating the source file from the test header files..."
# Chaining all tests together can have effects that you don't think of
#  - it's always a good idea to run your new/changed test on its own
if [ $# -eq 0 ]; then
	cxxtestgen.pl --error-printer -o runner.cpp *.h
else
	cxxtestgen.pl --error-printer -o runner.cpp $*
fi
echo

echo "Compiling the test executable..."
g++ -O0 -g3 -o runner.exe runner.cpp -I ../inc -I ../../API/inc \
            -L ../../Debug -L ../../Build -L ../../../Third_Party/lib/linux64 \
            -lMantid -lPocoFoundation -lPocoUtil -lPocoXML -lPocoNet -lboost_python -lpython2.3 -lboost_regex -lboost_filesystem
echo

echo "Running the tests..."
./runner.exe
echo

# Remove the generated files to ensure that they're not inadvertently run
#   when something in the chain has failed.
echo "Cleaning up..."
rm -rf runner.*
echo "Done."
