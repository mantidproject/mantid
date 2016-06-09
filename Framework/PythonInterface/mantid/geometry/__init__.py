"""
mantid.geometry
===============

Defines Python objects that wrap the C++ Geometry namespace.

"""
from __future__ import (absolute_import, division,
                        print_function, unicode_literals)

###############################################################################
# Load the C++ library
###############################################################################
from ._geometry import *

###############################################################################
# Make aliases accessible in this namespace
###############################################################################
from ._aliases import *