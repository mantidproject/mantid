# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
mantid.geometry
===============

Defines Python objects that wrap the C++ Geometry namespace.

"""
from __future__ import (absolute_import, division,
                        print_function)

###############################################################################
# Load the C++ library
###############################################################################
from ..kernel import _shared_cextension

with _shared_cextension():
    from _geometry import *

###############################################################################
# Make aliases accessible in this namespace
###############################################################################
from ._aliases import *