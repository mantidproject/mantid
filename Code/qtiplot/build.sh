#!/bin/sh
#
# Build the Mantidplot user interface
#
#
python release_date.py

# The top-level directory 
ROOTDIR=`pwd`

# First, build dialog library
cd $ROOTDIR/MantidQt
qmake
make clean
make
ERRORCODE=$?
if [ $ERRORCODE != 0 ]; then
    echo "Error: MantidQt build failed with error code $ERRORCODE"
    exit $ERRORCODE
fi

# Now build qtiplot
cd $ROOTDIR/qtiplot
qmake
make clean
make
ERRORCODE=$?
if [ $ERRORCODE? != 0 ]; then
    echo "Error: MantidPlot build failed with error code $ERRORCODE"
    exit $ERRORCODE
fi

# If we reached this point then the build was successful
echo "MantidQt and MantidPlot build succeeded"
