# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple, List, Tuple

from sans.common.Containers.FloatRange import FloatRange
from sans.common.Containers.MonitorID import MonitorID


class BackgroundDetails(NamedTuple):
    monitors_with_background_off: List[MonitorID]
    transmission_tof_range: FloatRange  # This does not appear to be used in user docs
    tof_window_all_monitors: FloatRange
    tof_window_single_monitor: List[Tuple[MonitorID, FloatRange]]
