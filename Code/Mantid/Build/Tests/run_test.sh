#!/bin/sh
if test -z "$1"; then
    echo "Usage: $0 test_executable"
    exit
fi
# export LD_LIBRARY_PATH=/home/ansell/Mantid2/mantid/Code:/home/ansell/Mantid2/Bin/Shared
./$*
