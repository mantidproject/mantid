# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple


class XYPosition(NamedTuple):
    X: float
    Y: float


class XYZPosition(NamedTuple):
    X: float
    Y: float
    Z: float
