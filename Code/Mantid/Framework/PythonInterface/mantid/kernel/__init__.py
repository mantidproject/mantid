"""
kernel
=============

Defines Python objects that wrap the C++ Kernel namespace.

"""

###############################################################################
# Loads the C library with the correct flags
###############################################################################
import dlopen as _pydlopen
import os as _os
clib = _os.path.join(_os.path.dirname(__file__), '_kernel.so')
flags = _pydlopen.setup_dlopen(clib, ['libMantidKernel'])
from _kernel import *
_pydlopen.restore_flags(flags)
###############################################################################

###############################################################################
# Do any site-specific setup for packages
###############################################################################
import packagesetup as _packagesetup

###############################################################################
# Make modules available in this namespace
###############################################################################
import environment
import funcreturns
from _aliases import *
