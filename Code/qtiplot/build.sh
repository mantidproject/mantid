#!/bin/sh
#
# Build the Mantidplot user interface
#
#
python release_date.py

# The top-level directory 
ROOTDIR=pwd

# First, build dialog library
cd $ROOTDIR\MantidQt
qmake
nmake clean
nmake

# Now build qtiplot
cd $ROOTDIR\qtiplot
nmake clean
qmake
nmake
