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
from ..kernel import _shared_cextension
with _shared_cextension():
    from _api import *

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
