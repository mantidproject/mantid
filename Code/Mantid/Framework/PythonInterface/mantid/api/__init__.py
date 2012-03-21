"""
api
===

Defines Python objects that wrap the C++ API namespace.

"""

###############################################################################
# The _api C extension depends on exports defined in the _kernel extension
###############################################################################
# The fully-qualified package path allows it to be found with path manipulation
from mantid.kernel import dlopen as _dlopen
import os as _os
clib = _os.path.join(_os.path.dirname(__file__), '_api.so')
flags = _dlopen.setup_dlopen(clib, ['libMantidKernel', 'libMantidGeometry', 'libMantidAPI']) # Ensure the library is open with the correct flags
from mantid.kernel import _kernel
from _api import *
_dlopen.restore_flags(flags)
###############################################################################

from _aliases import *

###############################################################################
# Add importAll member to ADS 
###############################################################################
import _adsimports

###############################################################################
# Attach operators to workspaces 
###############################################################################
import _workspaceops

###############################################################################
# Starting the FrameworkManager loads the C++ plugin libraries
# we need to load in the Python plugins as well
###############################################################################
import mantid.kernel.plugins as _plugins
# Algorithms
from mantid.kernel import config as _cfg
# Disabled for the time being as all algorithms are of the old kind
#_plugins.load(_cfg['pythonalgorithm.directories']) 
