# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import shutil
import systemtesting

import mantid.simpleapi as simple

from mantid import config
from Engineering.EnginX import main

DIRS = config['datasearch.directories'].split(';')
ref_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(DIRS[0]))),
                       "SystemTests", "tests", "framework", "reference")
root_directory = os.path.join(DIRS[0], "ENGINX")
cal_directory = os.path.join(root_directory, "cal")
focus_directory = os.path.join(root_directory, "focus")
WHOLE_INST_CALIB = os.path.join(root_directory, "ENGINX_whole_inst_calib.nxs")


class FocusBothBanks(systemtesting.MantidSystemTest):

    def runTest(self):
        _make_test_directories()
        main(vanadium_run="236516", user="test", focus_run="299080", force_cal=True, directory=focus_directory,
             full_inst_calib_path=WHOLE_INST_CALIB)

    def validate(self):
        self.tolerance = 3
        self.checkInstrument = False
        if systemtesting.using_gsl_v1():
            return ("engg_focus_output_bank_1", "engg_focusing_output_ws_bank_1_gsl1.nxs",
                    "engg_focus_output_bank_2", "engg_focusing_output_ws_bank_2_gsl1.nxs")
        else:
            return ("engg_focus_output_bank_1", "engg_focusing_output_ws_bank_1.nxs",
                    "engg_focus_output_bank_2", "engg_focusing_output_ws_bank_2.nxs")

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(focus_directory)


class FocusCropped(systemtesting.MantidSystemTest):

    def runTest(self):
        _make_test_directories()
        main(vanadium_run="236516", user="test", focus_run="299080", directory=focus_directory,
             crop_type="spectra", crop_on="1-20", full_inst_calib_path=WHOLE_INST_CALIB)

    def validate(self):
        self.tolerance = 1e-3
        return "engg_focus_output_cropped", "engg_focusing_output_ws_bank_cropped.nxs"

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(focus_directory)


class FocusTextureMode(systemtesting.MantidSystemTest):

    def runTest(self):
        _make_test_directories()
        main(vanadium_run="236516", user="test", focus_run=None, force_cal=True, directory=focus_directory,
             full_inst_calib_path=WHOLE_INST_CALIB)
        simple.mtd.clear()
        csv_file = os.path.join(root_directory, "EnginX.csv")
        location = os.path.join(focus_directory, "User", "test", "Calibration")
        shutil.copy2(csv_file, location)
        csv_file = os.path.join(location, "EnginX.csv")
        main(vanadium_run="236516", user="test", focus_run="299080", force_cal=True, directory=focus_directory,
             grouping_file=csv_file, full_inst_calib_path=WHOLE_INST_CALIB)
        output = "engg_focusing_output_ws_texture_bank_{}{}"
        group = ""

        for i in range(1, 11):
            group = group + output.format(i, ",")
        simple.GroupWorkspaces(InputWorkspaces=group, OutputWorkspace="test")

    def validate(self):
        self.tolerance = 1e-3
        outputlist = ["engg_focusing_output_ws_texture_bank_{}".format(i) for i in range(1, 11)]
        filelist = ["engg_texture_bank_{}.nxs".format(i) for i in range(1, 11)]
        validation_list = [x for t in zip(*[outputlist, filelist]) for x in t]
        return validation_list

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(focus_directory)


def _make_test_directories():
    """Attempts to make the input directory for the tests"""
    if not os.path.exists(cal_directory):
        os.makedirs(cal_directory)


def _try_delete(path):
    try:
        # Use this instead of os.remove as we could be passed a non-empty dir
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except OSError:
        print("Could not delete output file at: ", path)
