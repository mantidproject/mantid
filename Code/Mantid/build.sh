#!/bin/sh
#
# Build the Mantid library using scons-local in the Third_Party directory
# if scons is not installed on the local machine.
#
# Also Passes through command line arguments to Scons.
#
sc=`which scons 2>/dev/null`
if test x$sc = x; then 
    python ../Third_Party/src/scons-local/scons.py $*
else
    $sc $*	# use installed scone
fi
