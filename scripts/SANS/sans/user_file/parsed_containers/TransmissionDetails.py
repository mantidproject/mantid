# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple, List, Tuple

from sans.common.Containers.MonitorID import MonitorID


class TransmissionDetails(NamedTuple):
    selected_spectrum: int
    spectrum_shifts: List[Tuple[MonitorID, float]]

    selected_radius: float
    selected_mask_filenames: List[str]  # List of filenames - TODO add some typing around filenames
    selected_roi_filenames: List[str]  # List of filenames - TODO add some typing around filenames

    selected_can_workspace: str
    selected_sample_workspace: str
