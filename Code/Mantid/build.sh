#!/bin/bash
#
# Build the Mantid library using scons-local in the Third_Party directory
#
# Also Passes through command line arguments to Scons.
#
#
# Third_Party is not part of rpm distribution, so look for local scons
#
#sc=`which scons 2>/dev/null`
#if test x$sc = x; then
#    python ../Third_Party/src/scons-local/scons.py $* $MANTID_BUILD_FLAGS
#else
#    $sc $* $MANTID_BUILD_FLAGS
#fi

## Perform the build and exit if there were errors
scons $* $MANTID_BUILD_FLAGS
if [ $? -ne 0 ]; then
    exit
fi

## Move the libraries around to the correct place
# PLUGINS="Algorithms CurveFitting DataObjects DataHandling Nexus"
# SHAREDDIR=Bin/Shared
# PLUGINDIR=Bin/Plugins

# for i in $PLUGINS; do
#     lib=${SHAREDDIR}/libMantid${i}.so
#     if [ -e $lib ]; then
# 	echo "Moving ${lib} to $PLUGINDIR"
# 	mv {${SHAREDDIR},$PLUGINDIR}/libMantid${i}.so
#     fi
# done
