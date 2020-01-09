# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple

from sans.common.enums import ReductionDimensionality
from sans.user_file.settings_tags import event_binning_string_values


class SampleDetails(NamedTuple):
    event_slices : event_binning_string_values
    selected_dimension: ReductionDimensionality
    wide_angle_corrections: bool  # Also known as sample path on/off in .txt user files
    z_offset: float
