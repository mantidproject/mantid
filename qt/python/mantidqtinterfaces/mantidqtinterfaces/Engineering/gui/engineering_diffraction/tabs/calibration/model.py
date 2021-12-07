# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import Engineering.EnggUtils as EnggUtils
from Engineering.common import output_settings


class CalibrationModel(object):
    @staticmethod
    def create_new_calibration(calibration, rb_num, plot_output, save_dir=output_settings.get_output_path()):
        EnggUtils.create_new_calibration(calibration, rb_num, plot_output, save_dir)

    @staticmethod
    def load_existing_calibration_files(calibration):
        EnggUtils.load_existing_calibration_files(calibration)
