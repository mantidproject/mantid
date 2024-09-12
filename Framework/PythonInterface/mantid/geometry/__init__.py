# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
mantid.geometry
===============

Defines Python objects that wrap the C++ Geometry namespace.

"""

###############################################################################
# Load the C++ library
###############################################################################
from mantid.utils import import_mantid_cext

import_mantid_cext("._geometry", "mantid.geometry", globals())

###############################################################################
# Make aliases accessible in this namespace
###############################################################################
from mantid.geometry._aliases import *
