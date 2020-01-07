# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple, Tuple, List

from sans.common.Containers.FloatRange import FloatRange
from sans.user_file.settings_tags import mask_angle_entry, LimitsId, q_rebin_values, simple_range


class LimitDetails(NamedTuple):
    angle_limit: mask_angle_entry
    event_binning: str

    cut_limit: List[Tuple[LimitsId, float]]
    radius_range: FloatRange
    q_limits: q_rebin_values
    qxy_limit: simple_range
    wavelength_limit: simple_range
