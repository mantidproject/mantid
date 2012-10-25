"""
kernel
=============

Defines Python objects that wrap the C++ Kernel namespace.

"""

###############################################################################
# Loads the C library with the correct flags
###############################################################################
import dlopen as _dlopen
import os as _os
clib = _os.path.join(_os.path.dirname(__file__), '_kernel.so')
flags = _dlopen.setup_dlopen(clib, ['libMantidKernel']) 
from _kernel import *
_dlopen.restore_flags(flags)
###############################################################################

###############################################################################
# Set the path to the NeXus C library. It is required by the nxs package so
# make it a little bit easier for our users
###############################################################################
import nexuslib as _nexuslib
_nexuslib.set_NEXUSLIB_var()

###############################################################################
# Make modules available in this namespace
###############################################################################
import environment
import funcreturns
from _aliases import *
