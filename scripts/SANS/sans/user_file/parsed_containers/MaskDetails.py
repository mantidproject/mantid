# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple, List, Tuple

from sans.common.enums import DetectorType
from sans.user_file.settings_tags import mask_block, mask_block_cross, mask_line, range_entry_with_detector


class MaskDetails(NamedTuple):
    block_masks: List[mask_block]
    block_crosses: List[mask_block_cross]
    line_masks: List[mask_line]
    time_masks: List[range_entry_with_detector]
    time_masks_with_detector: List[range_entry_with_detector]

    remove_detector_masks: bool
    remove_time_masks: bool

    mask_horizontal_strips: List[Tuple[DetectorType, int]]
    mask_vertical_strips: List[Tuple[DetectorType, int]]
    mask_spectra_nums: List[int]

    mask_filenames: List[str]
