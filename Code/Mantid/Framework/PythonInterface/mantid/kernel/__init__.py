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
dlopen.restore_flags(flags)
###############################################################################

###############################################################################
# Make modules available in this namespace
###############################################################################
from _aliases import *
