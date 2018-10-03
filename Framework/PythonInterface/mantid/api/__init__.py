# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
api
===

Defines Python objects that wrap the C++ API namespace.

"""
from __future__ import (absolute_import, division,
                        print_function)

# Load the C++ library
from . import _api
from ._api import *

# stdlib imports
import atexit as _atexit

###############################################################################
# Start the framework
###############################################################################
FrameworkManagerImpl.Instance()
_atexit.register(FrameworkManagerImpl.Instance().shutdown)

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
