"""
api
===

Defines Python objects that wrap the C++ API namespace.

"""

###############################################################################
# The _api C extension depends on exports defined in the _kernel extension
###############################################################################
# The fully-qualified package path allows it to be found without path manipulation
from mantid.kernel import dlopen as _pydlopen
import os as _os
clib = _os.path.join(_os.path.dirname(__file__), '_api.so')
flags = _pydlopen.setup_dlopen(clib, ['libMantidKernel', 'libMantidGeometry', 'libMantidAPI']) # Ensure the library is open with the correct flags
from mantid.kernel import _kernel
import _api
from _api import *
_pydlopen.restore_flags(flags)
###############################################################################

###############################################################################
# Start the framework (if not embedded in other application)
###############################################################################
FrameworkManagerImpl.Instance()
# Declare any additional C++ algorithms defined in this package
_api._declareCPPAlgorithms()

###############################################################################
# Make aliases accessible in this namespace
###############################################################################
from _aliases import *

###############################################################################
# Add importAll member to ADS
###############################################################################
import _adsimports

###############################################################################
# Attach additional operators to workspaces
###############################################################################
import _workspaceops
_workspaceops.attach_binary_operators_to_workspace()
_workspaceops.attach_unary_operators_to_workspace()
_workspaceops.attach_tableworkspaceiterator()