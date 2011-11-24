"""
kernel
=============

Defines Python objects that wrap the C++ Kernel namespace.

"""

###############################################################################
# The _api C extension depends on exports defined in the _kernel extension
###############################################################################
import dlopen as _dlopen
flags = _dlopen.setup_dlopen() # Ensure the library is open with the correct flags
from _kernel import *
dlopen.restore_flags(flags)
