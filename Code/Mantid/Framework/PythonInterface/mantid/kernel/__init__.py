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

###############################################################################
# Make the singleton objects available as named variables 
###############################################################################
config = ConfigService.Instance()

###############################################################################
# Set up a general Python logger. Others can be created as they are required
# if a user wishes to be more specific
###############################################################################
logger = Logger.get("Python")