#!/bin/sh
#
# Build the Mantid library using scons-local in the Third_Party directory.
# Passes through command line arguments to Scons.
#
python ../../Third_Party/src/scons-local/scons.py $*

