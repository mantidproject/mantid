# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)
import os
import systemtesting
import shutil
import mantid.simpleapi as simple
from mantid import config
from Engineering.EnginX import EnginX

DIRS = config['datasearch.directories'].split(';')

root_directory = os.path.join(DIRS[0], "ENGINX")
cal_directory = os.path.join(root_directory, "cal")
focus_directory = os.path.join(root_directory, "focus")
focus_tests = 0

class CreateVanadiumTest(systemtesting.MantidSystemTest):

    def runTest(self):
        os.makedirs(cal_directory)
        test = EnginX(user="test", vanadium_run="236516",
                      directory=cal_directory)
        test.create_vanadium()

    def validate(self):
        return "eng_vanadium_integration", "engggui_vanadium_integration.nxs"

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(cal_directory)


class CreateCalibrationWholeTest(systemtesting.MantidSystemTest):

    def runTest(self):
        os.makedirs(cal_directory)
        test = EnginX(user="test", vanadium_run="236516",
                      directory=cal_directory)
        test.create_vanadium()
        test.create_calibration()

    def validate(self):
        return ("engg_calibration_bank_1", "engggui_calibration_bank_1.nxs",
                "engg_calibration_bank_2", "engggui_calibration_bank_2.nxs",
                "engg_calibration_banks_parameters", "engggui_calibration_banks_parameters.nxs")

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(cal_directory)


class CreateCalibrationCroppedTest(systemtesting.MantidSystemTest):

    def runTest(self):
        os.makedirs(cal_directory)
        test = EnginX(user="test", vanadium_run="236516",
                      directory=cal_directory)
        test.create_vanadium()
        test.create_calibration(cropped="spectra", spectra="1-20")

    def validate(self):
        return ("cropped", "engggui_calibration_bank_cropped.nxs",
                "engg_calibration_banks_parameters", "engggui_calibration_bank_cropped_parameters.nxs")

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(cal_directory)


class CreateCalibrationBankTest(systemtesting.MantidSystemTest):

    def runTest(self):
        os.makedirs(cal_directory)
        test = EnginX(user="test", vanadium_run="236516",
                      directory=cal_directory)
        test.create_vanadium()
        test.create_calibration(cropped="banks", bank="South")

    def validate(self):
        return ("engg_calibration_bank_2", "engggui_calibration_bank_2.nxs",
                "engg_calibration_banks_parameters", "engggui_calibration_bank_south_parameters.nxs")

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(cal_directory)





def _try_delete(path):
    try:
        # Use this instead of os.remove as we could be passed a non-empty dir
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except OSError:
        print ("Could not delete output file at: ", path)


