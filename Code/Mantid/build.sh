#!/bin/sh
#
# Build the Mantid library using scons-local in the Third_Party directory
#
# Also Passes through command line arguments to Scons.
#
python release_version.py
#
# Third_Party is not part of rpm distribution, so look for local scons
#
sc=`which scons 2>/dev/null`
if test x$sc = x; then
    python ../Third_Party/src/scons-local/scons.py $* $MANTID_BUILD_FLAGS
else
    $sc $* $MANTID_BUILD_FLAGS
fi
