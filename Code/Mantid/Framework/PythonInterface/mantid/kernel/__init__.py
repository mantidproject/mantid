"""
kernel
=============

Defines Python objects that wrap the C++ Kernel namespace.

"""
from __future__ import absolute_import

###############################################################################
# Load the C++ library
###############################################################################
from ._kernel import *

###############################################################################
# Do any site-specific setup for packages
###############################################################################
from . import packagesetup as _packagesetup

###############################################################################
# Make modules available in this namespace
###############################################################################
from . import environment
from . import funcreturns
from ._aliases import *
