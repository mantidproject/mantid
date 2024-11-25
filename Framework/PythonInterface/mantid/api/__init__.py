# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# ruff: noqa: E402,F403   # Allow module import not at top and wild imports
"""
api
===

Defines Python objects that wrap the C++ API namespace.

"""

###############################################################################
# Load the C++ library
###############################################################################
from mantid.utils import import_mantid_cext

# insert all the classes from _api in the mantid.api namespace
import_mantid_cext("._api", "mantid.api", globals())

###############################################################################
# Attach additional operators to workspaces
###############################################################################
from mantid.api import _workspaceops

_workspaceops.attach_binary_operators_to_workspace()
_workspaceops.attach_unary_operators_to_workspace()
_workspaceops.attach_tableworkspaceiterator()
###############################################################################
# Add importAll member to ADS.
#
# Must be imported AFTER all the api members
# have been added to the mantid.api namespace above!
###############################################################################
from mantid.api import _adsimports  # noqa: F401


###############################################################################
# Make aliases accessible in this namespace
###############################################################################
from mantid.api._aliases import *
