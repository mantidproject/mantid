#!/bin/sh
#
# Build the Mantid library using scons-local in the Third_Party directory
#
# Also Passes through command line arguments to Scons.
#
#sc=`which scons 2>/dev/null`
echo 1.0 > vers.txt
svnversion . >> vers.txt
python release_version.py
