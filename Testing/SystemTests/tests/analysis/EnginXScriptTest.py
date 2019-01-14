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


class FocusBothBanks(systemtesting.MantidSystemTest):

    def runTest(self):
        _setup_focus()
        test = EnginX(user="test", vanadium_run="236516",
                      directory=focus_directory)
        test.focus(run_number="299080")

    def validate(self):
        return("engg_focus_output_bank_1", "enggui_focusing_output_ws_bank_1.nxs",
               "engg_focus_output_bank_2", "enggui_focusing_output_ws_bank_2.nxs")

    def cleanup(self):
        simple.mtd.clear()
        _tear_down_focus()


class FocusCropped(systemtesting.MantidSystemTest):

    def runTest(self):
        _setup_focus()
        test = EnginX(user="test", vanadium_run="236516",
                      directory=focus_directory)
        test.focus(run_number="299080", cropped="spectra", spectra="1-20")

    def validate(self):
        return "engg_focus_output", "enggui_focusing_output_ws_bank_cropped.nxs"

    def cleanup(self):
        simple.mtd.clear()
        _tear_down_focus()


class FocusTextureMode(systemtesting.MantidSystemTest):

    def runTest(self):
        _setup_focus()
        csv_file = os.path.join(root_directory, "EnginX.csv")
        location = os.path.join(focus_directory, "User", "test", "Calibration")
        shutil.copy2(csv_file, location)
        csv_file = os.path.join(location, "EnginX.csv")
        test = EnginX(user="test", vanadium_run="236516",
                      directory=focus_directory)
        test.focus(run_number="299080", grouping_file=csv_file)
        output = "engg_focusing_output_ws_texture_bank_{}{}"
        group = ""

        for i in range(1, 11):
            group = group+output.format(i, ",")
        simple.GroupWorkspaces(InputWorkspaces=group, OutputWorkspace="test")

    def validate(self):
        outputlist=["engg_focusing_output_ws_texture_bank_{}".format(i) for i in range(1, 11)]
        filelist=["enggui_texture_Bank_{}.nxs".format(i) for i in range(1, 11)]
        validation_list = [x for t in zip(*[outputlist, filelist])for x in t]
        return validation_list

    def cleanup(self):
        simple.mtd.clear()
        _tear_down_focus()


def _try_delete(path):
    try:
        # Use this instead of os.remove as we could be passed a non-empty dir
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except OSError:
        print ("Could not delete output file at: ", path)


def _setup_focus():
    os.makedirs(focus_directory)
    test = EnginX(user="test", vanadium_run="236516",
                  directory=focus_directory)
    test.create_vanadium()
    test.create_calibration()
    test.create_calibration(cropped="spectra", spectra="1-20")
    simple.mtd.clear()


def _tear_down_focus():
        _try_delete(focus_directory)
