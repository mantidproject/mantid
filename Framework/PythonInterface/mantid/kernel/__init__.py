# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
kernel
=============

Defines Python objects that wrap the C++ Kernel namespace.

"""

###############################################################################
# Make modules available in this namespace
###############################################################################
# Imports boost.mpi if applicable
from mantid.kernel import funcinspect

# module alias for backwards-compatibility in user scripts
funcreturns = funcinspect

###############################################################################
# Do site-specific setup for packages
###############################################################################
from mantid.kernel import packagesetup as _mantidsite

_mantidsite.set_NEXUSLIB_var()

###############################################################################
# Load the C++ library
###############################################################################
from mantid.utils import import_mantid_cext

# insert all the classes from _kernel in the mantid.kernel namespace
import_mantid_cext("._kernel", "mantid.kernel", globals())

from mantid.kernel._aliases import *
from mantid.kernel.AmendConfig import amend_config  # noqa: F401
