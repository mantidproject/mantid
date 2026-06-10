# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Engineering.common.calibration_info import CalibrationInfo
from mantid.simpleapi import Load, AnalysisDataService as ADS, logger
import Engineering.EnggUtils as EnggUtils
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from mantid.dataobjects import Workspace2D


class CalibrationModel(object):
    # model shares code with the EnginX auto reduction script - code is kept in EnggUtils.py
    def __init__(self):
        self._saved_prm_file = ""

    def get_last_prm_file_gsas2(self) -> str:
        return self._saved_prm_file

    def create_new_calibration(
        self, calibration: CalibrationInfo, rb_num: str | None, plot_output: bool, instrument: str, save_dir: str | None = None
    ) -> None:
        if save_dir is None:
            save_dir = output_settings.get_output_path()
        full_calib = load_full_instrument_calibration(instrument)
        self._saved_prm_file = EnggUtils.create_new_calibration(calibration, rb_num, plot_output, save_dir, full_calib, False)

    def load_existing_calibration_files(self, calibration: CalibrationInfo, instrument: str) -> None:
        load_full_instrument_calibration(instrument)
        self._saved_prm_file = EnggUtils.load_existing_calibration_files(calibration)


def load_full_instrument_calibration(instrument: str) -> Workspace2D | None:
    if ADS.doesExist(f"full_inst_calib_{instrument}"):
        full_calib = ADS.retrieve(f"full_inst_calib_{instrument}")
    else:
        full_calib_path = get_setting(
            output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, f"full_calibration_{instrument}"
        )
        try:
            full_calib = Load(full_calib_path, OutputWorkspace=f"full_inst_calib_{instrument}")
        except ValueError:
            logger.error(
                f"Error loading Full instrument calibration from path: `{full_calib_path}` - this is set in the interface settings."
            )
            return
    return full_calib
