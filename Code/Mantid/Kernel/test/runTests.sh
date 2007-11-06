#!/bin/bash
# Simple script to build and run the tests.
# Have kept separate from the makefile since that's automatically generated
#   by Eclipse.
#
# Author: Russell Taylor, 19/09/07
#

echo
echo "Making sure that the Mantid library is built and up-to-date..."
echo
# make -C ../../Build
echo

echo "Generating the source from the test header files..."
cxxtestgen.pl --error-printer -o runner.cpp *.h
echo

echo "Compiling the test executable..."
g++ -o runner.exe runner.cpp -L ../../Release -L ../../../Third_Party/lib/linux64 -lMantid -lGet -lg2c -lPocoFoundation -lPocoUtil -lPocoXML
echo

echo "Running the tests..."
./runner.exe

# Remove the generated files to ensure that they're not inadvertently run
#   when something in the chain has failed.
rm -rf runner.*
