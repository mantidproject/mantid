# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from enum import Enum


class PlotType(Enum):
    LINEAR = 1
    SCATTER = 2
    LINE_AND_SYMBOL = 3
    LINEAR_WITH_ERR = 4
    SCATTER_WITH_ERR = 5
