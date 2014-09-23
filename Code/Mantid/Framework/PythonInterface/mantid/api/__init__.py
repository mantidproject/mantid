"""
api
===

Defines Python objects that wrap the C++ API namespace.

"""
from __future__ import absolute_import

###############################################################################
# Load the C++ library
###############################################################################
from . import _api
from ._api import *

###############################################################################
# Start the framework (if not embedded in other application)
###############################################################################
FrameworkManagerImpl.Instance()
# Declare any additional C++ algorithms defined in this package
_api._declareCPPAlgorithms()

###############################################################################
# Make aliases accessible in this namespace
###############################################################################
from ._aliases import *

###############################################################################
# Add importAll member to ADS
###############################################################################
from . import _adsimports

###############################################################################
# Attach additional operators to workspaces
###############################################################################
from . import _workspaceops
_workspaceops.attach_binary_operators_to_workspace()
_workspaceops.attach_unary_operators_to_workspace()
_workspaceops.attach_tableworkspaceiterator()
