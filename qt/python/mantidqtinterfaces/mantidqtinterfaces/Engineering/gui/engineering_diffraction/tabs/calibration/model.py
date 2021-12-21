# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import Load, AnalysisDataService as ADS, logger
import Engineering.EnggUtils as EnggUtils
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting


class CalibrationModel(object):
    # model shares code with the EnginX auto reduction script - code is kept in EnggUtils.py
    @staticmethod
    def create_new_calibration(calibration, rb_num, plot_output, save_dir=output_settings.get_output_path()):
        full_calib = load_full_instrument_calibration()
        EnggUtils.create_new_calibration(calibration, rb_num, plot_output, save_dir, full_calib)

    @staticmethod
    def load_existing_calibration_files(calibration):
        load_full_instrument_calibration()
        EnggUtils.load_existing_calibration_files(calibration)


def load_full_instrument_calibration():
    if ADS.doesExist("full_inst_calib"):
        full_calib = ADS.retrieve("full_inst_calib")
    else:
        full_calib_path = get_setting(output_settings.INTERFACES_SETTINGS_GROUP,
                                      output_settings.ENGINEERING_PREFIX, "full_calibration")
        try:
            full_calib = Load(full_calib_path, OutputWorkspace="full_inst_calib")
        except ValueError:
            logger.error("Error loading Full instrument calibration - this is set in the interface settings.")
            return
    return full_calib
