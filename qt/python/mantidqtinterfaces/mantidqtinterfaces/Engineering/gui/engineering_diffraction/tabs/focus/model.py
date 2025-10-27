# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Optional

import Engineering.EnggUtils as EnggUtils
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from Engineering.common.calibration_info import CalibrationInfo
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.calibration.model import load_full_instrument_calibration
import os


class FocusModel(object):
    # model shares code with the EnginX auto reduction script - code is kept in EnggUtils.py
    def __init__(self):
        self._last_focused_files = []
        self._last_focused_files_gsas2 = []
        self._last_focused_files_combined = []

    def get_last_focused_files(self):
        return self._last_focused_files

    def get_last_focused_files_gsas2(self):
        return self._last_focused_files_gsas2

    def get_last_focused_files_texture(self):
        return self._last_focused_files_combined

    @staticmethod
    def get_last_directory(filepaths):
        directories = set()
        directory = None
        for filepath in filepaths:
            directory, discard = os.path.split(filepath)
            directories.add(directory)
        if len(directories) == 1:
            return directory

    def focus_run(
        self,
        sample_paths: list,
        vanadium_path: str,
        plot_output: bool,
        rb_num: str,
        calibration: CalibrationInfo,
        save_dir: Optional[str] = None,
    ) -> None:
        if save_dir is None:
            save_dir = output_settings.get_output_path()
        full_calib = load_full_instrument_calibration()
        focused_files, focused_files_gsas2, focused_files_combined = EnggUtils.focus_run(
            sample_paths, vanadium_path, plot_output, rb_num, calibration, save_dir, full_calib
        )
        self._last_focused_files = focused_files
        self._last_focused_files_gsas2 = focused_files_gsas2
        self._last_focused_files_combined = focused_files_combined
