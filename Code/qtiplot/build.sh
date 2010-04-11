#!/bin/sh
#
# Build the Mantidplot user interface
#
#
CLEAN="0"
if [ $# -ge 1 ]; then 
    if [ $1 = "clean" ]; then CLEAN="1"; fi
fi
if [ $# -ge 2 ]; then 
    VERSION=$2
fi
if [ $# -ge 3 ]; then 
    SVN_VERSION=$3
fi

python release_date.py $VERSION $SVN_VERSION

# The top-level directory 
ROOTDIR=`pwd`

# First, build dialog library
cd $ROOTDIR/MantidQt
if [ $CLEAN = "1" ]; then 
    make clean 
    qmake-qt4 QMAKE_CC="${CC:-gcc}" QMAKE_CXX="${CXX:-g++}"
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
    qmake-qt4 QMAKE_CC="${CC:-gcc}" QMAKE_CXX="${CXX:-g++}"
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
    qmake-qt4 QMAKE_CC="${CC:-gcc}" QMAKE_CXX="${CXX:-g++}"
fi
make -j2
ERRORCODE=$?
if [ $ERRORCODE != 0 ]; then
    echo "Error: MantidPlot build failed with error code $ERRORCODE"
    exit $ERRORCODE
fi

# If we reached this point then the build was successful
echo "MantidQt and MantidPlot build succeeded"
