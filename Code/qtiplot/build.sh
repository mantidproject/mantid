#!/bin/sh
#
# Build the Mantidplot user interface
#
#
python release_date.py

CLEAN="0"
if [ $# -eq 1 ]; then 
    if [ $1 = "clean" ]; then CLEAN="1";fi
fi

# The top-level directory 
ROOTDIR=`pwd`

# First, build dialog library
cd $ROOTDIR/MantidQt
if [ $CLEAN = "1" ]; then 
    make clean 
    qmake 
fi
make
ERRORCODE=$?
if [ $ERRORCODE != 0 ]; then
    echo "Error: MantidQt build failed with error code $ERRORCODE"
    exit $ERRORCODE
fi

# QtPropertyBrowser library
cd $ROOTDIR/QtPropertyBrowser
if [ $CLEAN = "1" ]; then 
    make clean
    qmake
fi
make -j2
ERRORCODE=$?
if [ $ERRORCODE != 0 ]; then
    echo "Error: QtPropertyBrowser build failed with error code $ERRORCODE"
    exit $ERRORCODE
fi

# Now build qtiplot
cd $ROOTDIR/qtiplot
if [ $CLEAN = "1" ]; then 
    make clean
    qmake
fi
make -j2
ERRORCODE=$?
if [ $ERRORCODE != 0 ]; then
    echo "Error: MantidPlot build failed with error code $ERRORCODE"
    exit $ERRORCODE
fi

# If we reached this point then the build was successful
echo "MantidQt and MantidPlot build succeeded"
