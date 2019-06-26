# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
from __future__ import absolute_import

from mantid.py3compat import Enum


# Any changes here must be reflected in the definition in
# the C++ MplCpp/Plot.h header. See the comment in that file
# for the reason for duplication.
class MantidAxType(Enum):
    """Define an list of possible axis types for plotting"""

    BIN = 0
    SPECTRUM = 1
