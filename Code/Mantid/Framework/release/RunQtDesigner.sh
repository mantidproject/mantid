#!/bin/bash
#
# Sets the correct environmental variables for Qt designer to pick up the Mantid plugins 
#

export QT_PLUGIN_PATH=$PWD
export LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH

designer