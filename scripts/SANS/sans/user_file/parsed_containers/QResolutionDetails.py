# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple


class QResolutionDetails(NamedTuple):
    use_q_resolution_calculation: bool

    a1: float
    a2: float
    h1: float
    h2: float
    w1: float
    w2: float

    collimation_length: float
    delta_r: float

    moderator_filename: str
