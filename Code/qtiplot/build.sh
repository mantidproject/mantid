#!/bin/sh
#
# Build the Mantidplot user interface
#
build_library () {
    if [ "$2" = "clean" -o ! -f Makefile ]; then
	qmake-qt4 QMAKE_CC="${CC:-gcc}" QMAKE_CXX="${CXX:-g++}" QMAKE_LINK="${CXX:-g++}"
    fi
    if [ "$2" = "clean" ]; then
	make clean
    fi
    make
    ERRORCODE=$?
    if [ $ERRORCODE != 0 ]; then
	echo "Error: $1 build failed with error code $ERRORCODE"
	exit $ERRORCODE
    fi
}

CLEAN=""
if [ $# -ge 1 ]; then 
    if [ $1 = "clean" ]; then CLEAN=$1; fi
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

# QtPropertyBrowser library
cd $ROOTDIR/QtPropertyBrowser
build_library "QtPropertyBrowser" $CLEAN

# Build dialog library
cd $ROOTDIR/MantidQt
build_library "MantidQt" $CLEAN

# Now build qtiplot
cd $ROOTDIR/qtiplot
build_library "MantidPlot" $CLEAN

# If we reached this point then the build was successful
echo "MantidQt and MantidPlot build succeeded"
cd $ROOTDIR
