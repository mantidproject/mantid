# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple, List, Dict

from sans.common.enums import DetectorType
from sans.user_file.settings_tags import monitor_spectrum, monitor_length


class MonitorDetails(NamedTuple):
    direct_filename: Dict[DetectorType, str]
    flood_source_filename: Dict[DetectorType, str]  # Previously known as flat in .txt file
    monitor_pos_from_moderator: List[monitor_length]
    selected_spectrum: monitor_spectrum
