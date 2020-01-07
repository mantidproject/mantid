# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple, Dict

from sans.common.Containers.FloatRange import FloatRange
from sans.common.enums import ReductionMode, CorrectionType


class DetectorDetails(NamedTuple):
    reduction_mode: ReductionMode
    detector_adjustment: Dict[CorrectionType, float]

    merge_fitted_rescale: FloatRange
    merge_rescale: float

    merge_fitted_shift: FloatRange
    merge_shift: float
