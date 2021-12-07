# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Optional

import Engineering.EnggUtils as EnggUtils
from Engineering.common import output_settings
from Engineering.common.calibration_info import CalibrationInfo


class FocusModel(object):

    def __init__(self):
        self._last_focused_files = []

    def get_last_focused_files(self):
        return self._last_focused_files

    def focus_run(self, sample_paths: list, vanadium_path: str, plot_output: bool, rb_num: str,
                  calibration: CalibrationInfo, save_dir: Optional[str] = output_settings.get_output_path()) -> None:
        focused_files = EnggUtils.focus_run(sample_paths, vanadium_path, plot_output, rb_num, calibration, save_dir)
        self._last_focused_files.extend(focused_files)
