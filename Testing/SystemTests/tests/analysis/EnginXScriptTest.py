# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)
import os
import platform
import shutil
import systemtesting

import mantid.simpleapi as simple

from mantid import config
from Engineering.EnginX import main

DIRS = config['datasearch.directories'].split(';')
ref_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(DIRS[0]))),
                       "SystemTests", "tests", "analysis", "reference")
root_directory = os.path.join(DIRS[0], "ENGINX")
cal_directory = os.path.join(root_directory, "cal")
focus_directory = os.path.join(root_directory, "focus")


class CreateVanadiumTest(systemtesting.MantidSystemTest):

    def runTest(self):
        os.makedirs(cal_directory)
        main(vanadium_run="236516", user="test", focus_run=None, force_vanadium=True, directory=cal_directory)

    def validate(self):
        return "eng_vanadium_integration", "engggui_vanadium_integration.nxs"

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(cal_directory)


class CreateCalibrationWholeTest(systemtesting.MantidSystemTest):

    def runTest(self):
        os.makedirs(cal_directory)
        main(vanadium_run="236516", user="test", focus_run=None, do_cal=True, directory=cal_directory)

    def validate(self):
        self.tolerance_is_rel_err = True
        self.tolerance = 5e-2

        # this is neccesary due to appendspectra creating spectrum numbers of 0
        self.disableChecking.append('SpectraMap')
        if _current_os_has_gsl_lvl2():
            return ("engg_calibration_bank_1", "engggui_calibration_bank_1.nxs",
                    "engg_calibration_bank_2", "engggui_calibration_bank_2.nxs",
                    "engg_calibration_banks_parameters", "engggui_calibration_banks_parameters.nxs",
                    "Engg difc Zero Peaks Bank 1", "engggui_difc_zero_peaks_bank_1.nxs",
                    "Engg difc Zero Peaks Bank 2", "engggui_difc_zero_peaks_bank_2.nxs"
                    )
        return ("engg_calibration_bank_1", "engggui_calibration_bank_1_gsl1.nxs",
                "engg_calibration_bank_2", "engggui_calibration_bank_2_gsl1.nxs",
                "engg_calibration_banks_parameters", "engggui_calibration_banks_parameters_gsl1.nxs",
                "Engg difc Zero Peaks Bank 1", "engggui_difc_zero_peaks_bank_1.nxs",
                "Engg difc Zero Peaks Bank 2", "engggui_difc_zero_peaks_bank_2.nxs")

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(cal_directory)


class CreateCalibrationCroppedTest(systemtesting.MantidSystemTest):

    def runTest(self):
        os.makedirs(cal_directory)
        main(vanadium_run="236516", user="test", focus_run=None, do_cal=True, directory=cal_directory,
             crop_type="spectra", crop_on="1-20")

    def validate(self):
        self.tolerance_is_rel_err = True
        self.tolerance = 1e-2

        # this is neccesary due to appendspectra creating spectrum numbers of 0
        self.disableChecking.append('SpectraMap')
        if _current_os_has_gsl_lvl2():
            return ("cropped", "engggui_calibration_bank_cropped.nxs",
                    "engg_calibration_banks_parameters", "engggui_calibration_bank_cropped_parameters.nxs",
                    "Engg difc Zero Peaks Bank cropped", "engggui_difc_zero_peaks_bank_cropped.nxs")
        return ("cropped", "engggui_calibration_bank_cropped.nxs_gsl1.nxs",
                "engg_calibration_banks_parameters", "engggui_calibration_cropped_parameters_gsl1.nxs",
                "Engg difc Zero Peaks Bank cropped", "engggui_difc_zero_peaks_bank_cropped.nxs")

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(cal_directory)


class CreateCalibrationBankTest(systemtesting.MantidSystemTest):

    def runTest(self):
        os.makedirs(cal_directory)
        main(vanadium_run="236516", user="test", focus_run=None, do_cal=True, directory=cal_directory,
             crop_type="banks", crop_on="South")

    def validate(self):
        self.tolerance_is_rel_err = True
        self.tolerance = 1e-2

        # this is neccesary due to appendspectra creating spectrum numbers of 0
        self.disableChecking.append('SpectraMap')
        if _current_os_has_gsl_lvl2():
            return ("engg_calibration_bank_2", "engggui_calibration_bank_2.nxs",
                    "engg_calibration_banks_parameters", "engggui_calibration_bank_south_parameters.nxs",
                    "Engg difc Zero Peaks Bank 2", "engggui_difc_zero_peaks_bank_2.nxs")
        return ("engg_calibration_bank_2", "engggui_calibration_bank_2_gsl1.nxs",
                "engg_calibration_banks_parameters", "engggui_calibration_bank_south_parameters_gsl1.nxs",
                "Engg difc Zero Peaks Bank 2", "engggui_difc_zero_peaks_bank_2.nxs")

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(cal_directory)


class FocusBothBanks(systemtesting.MantidSystemTest):

    def runTest(self):
        os.makedirs(focus_directory)
        main(vanadium_run="236516", user="test", focus_run="299080", do_cal=True, directory=focus_directory)

    def validate(self):
        return ("engg_focus_output_bank_1", "enggui_focusing_output_ws_bank_1.nxs",
                "engg_focus_output_bank_2", "enggui_focusing_output_ws_bank_2.nxs")

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(focus_directory)


class FocusCropped(systemtesting.MantidSystemTest):

    def runTest(self):
        os.makedirs(focus_directory)
        main(vanadium_run="236516", user="test", focus_run="299080", directory=focus_directory,
             crop_type="spectra", crop_on="1-20")

    def validate(self):
        return "engg_focus_output", "enggui_focusing_output_ws_bank_cropped.nxs"

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(focus_directory)


class FocusTextureMode(systemtesting.MantidSystemTest):

    def runTest(self):
        os.makedirs(focus_directory)
        main(vanadium_run="236516", user="test", focus_run=None, do_cal=True, directory=focus_directory)
        simple.mtd.clear()
        csv_file = os.path.join(root_directory, "EnginX.csv")
        location = os.path.join(focus_directory, "User", "test", "Calibration")
        shutil.copy2(csv_file, location)
        csv_file = os.path.join(location, "EnginX.csv")
        main(vanadium_run="236516", user="test", focus_run="299080", do_cal=True, directory=focus_directory,
             grouping_file=csv_file)
        output = "engg_focusing_output_ws_texture_bank_{}{}"
        group = ""

        for i in range(1, 11):
            group = group + output.format(i, ",")
        simple.GroupWorkspaces(InputWorkspaces=group, OutputWorkspace="test")

    def validate(self):
        outputlist = ["engg_focusing_output_ws_texture_bank_{}".format(i) for i in range(1, 11)]
        filelist = ["enggui_texture_Bank_{}.nxs".format(i) for i in range(1, 11)]
        validation_list = [x for t in zip(*[outputlist, filelist]) for x in t]
        return validation_list

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(focus_directory)


def _try_delete(path):
    try:
        # Use this instead of os.remove as we could be passed a non-empty dir
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except OSError:
        print("Could not delete output file at: ", path)


def _current_os_has_gsl_lvl2():
    """ Check whether the current OS should be running GSLv2 """
    return platform.linux_distribution()[0].lower() == "ubuntu" or platform.mac_ver()[0] != ''
