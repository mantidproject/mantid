"""
kernel
=============

Defines Python objects that wrap the C++ Kernel namespace.

"""

###############################################################################
# The _api C extension depends on exports defined in the _kernel extension
###############################################################################
import dlopen as _dlopen
import os as _os
clib = _os.path.join(_os.path.dirname(__file__), '_kernel.so')
flags = _dlopen.setup_dlopen(clib, ['libMantidKernel']) # Ensure the library is open with the correct flags
from _kernel import *
dlopen.restore_flags(flags)
###############################################################################

from _aliases import *
