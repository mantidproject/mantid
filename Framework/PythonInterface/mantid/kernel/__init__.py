"""
kernel
=============

Defines Python objects that wrap the C++ Kernel namespace.

"""
from __future__ import (absolute_import, division,
                        print_function)

# Imports boost.mpi if applicable
from . import mpisetup

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

# module alias for backwards-compatability in user scripts
funcreturns = funcinspect

