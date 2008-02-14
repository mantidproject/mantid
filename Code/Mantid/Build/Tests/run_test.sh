#!/bin/sh
if test -z "$1"; then
    echo "Usage: $0 test_executable"
    exit
fi
export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/poco-1.3.1/lib/:../../Bin/Shared
./$*
