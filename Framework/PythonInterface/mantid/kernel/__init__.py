"""
kernel
=============

Defines Python objects that wrap the C++ Kernel namespace.

"""
from __future__ import (absolute_import, division,
                        print_function, unicode_literals)

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
from . import funcinspect
from ._aliases import *
