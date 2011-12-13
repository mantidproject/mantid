"""
mantid.geometry
===============

Defines Python objects that wrap the C++ Geometry namespace.

"""

###############################################################################
# The _api C extension depends on exports defined in the _kernel extension
###############################################################################
from mantid.kernel import dlopen as _dlopen
flags = _dlopen.setup_dlopen() # Ensure the library is open with the correct flags
from _geometry import *
_dlopen.restore_flags(flags)
