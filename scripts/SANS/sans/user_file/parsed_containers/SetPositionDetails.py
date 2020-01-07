# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple, Dict

from sans.common.Containers.Position import XYPosition
from sans.common.enums import DetectorType
from sans.user_file.settings_tags import set_scales_entry


class SetPositionDetails(NamedTuple):
    centre: Dict[DetectorType, XYPosition]
    scales: set_scales_entry
