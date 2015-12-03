"""
mantid.experimental
===================

Defines Python objects that wrap the C++ Experimental namespace.

"""
from __future__ import absolute_import

###############################################################################
# Load the C++ library
###############################################################################
from ._experimental import *

###############################################################################
# Make aliases accessible in this namespace
###############################################################################
from ._aliases import *
