#!/bin/sh
#
# Build the Mantid library using scons-local in the Third_Party directory
#
# Also Passes through command line arguments to Scons.
#
#sc=`which scons 2>/dev/null`
python ../Third_Party/src/scons-local/scons.py $*
