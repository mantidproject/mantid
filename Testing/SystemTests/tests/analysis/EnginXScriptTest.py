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
import mantid.kernel
import mantid.simpleapi as simple
from mantid import config
from Engineering.EnginX import main

DIRS = config['datasearch.directories'].split(';')
ref_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(DIRS[0]))),
                       "SystemTests", "tests", "analysis", "reference")
root_directory = os.path.join(DIRS[0], "ENGINX")
cal_directory = os.path.join(root_directory, "cal")
focus_directory = os.path.join(root_directory, "focus")
param_deltas = [0.1, 0.1, 1, 2]
cal_deltas = [0.1, 90000, 0.8, 52000, 1, 0.1, 4, 3, 250, 9, 800, 1.5, 10, 0.5, 5, 0.5]


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
        return_list = [_compare_tableworkspaces("engg_calibration_bank_1", "engggui_calibration_bank_1.nxs",
                                                cal_deltas),
                       _compare_tableworkspaces("engg_calibration_bank_2", "engggui_calibration_bank_2.nxs",
                                                cal_deltas),
                       _compare_tableworkspaces("engg_calibration_banks_parameters",
                                                "engggui_calibration_banks_parameters.nxs", param_deltas)]

        return all(return_list)

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(cal_directory)


class CreateCalibrationCroppedTest(systemtesting.MantidSystemTest):

    def runTest(self):
        os.makedirs(cal_directory)
        main(vanadium_run="236516", user="test", focus_run=None, do_cal=True, directory=cal_directory,
             crop_type="spectra", crop_on="1-20")

    def validate(self):
        return_list = [_compare_tableworkspaces("cropped", "engggui_calibration_bank_cropped.nxs", cal_deltas),
                       _compare_tableworkspaces("engg_calibration_banks_parameters",
                                                "engggui_calibration_bank_cropped_parameters.nxs", param_deltas)]
        return all(return_list)

    def cleanup(self):
        simple.mtd.clear()
        _try_delete(cal_directory)


class CreateCalibrationBankTest(systemtesting.MantidSystemTest):

    def runTest(self):
        os.makedirs(cal_directory)
        main(vanadium_run="236516", user="test", focus_run=None, do_cal=True, directory=cal_directory,
             crop_type="banks", crop_on="South")

    def validate(self):
        return_list = [_compare_tableworkspaces("engg_calibration_bank_2", "engggui_calibration_bank_2.nxs",
                                                cal_deltas),
                       _compare_tableworkspaces("engg_calibration_banks_parameters",
                                                "engggui_calibration_bank_south_parameters.nxs", param_deltas)]
        return all(return_list)

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


def _setup_focus():
    os.makedirs(focus_directory)
    main(vanadium_run="236516", user="test", focus_run=None, do_cal=True, directory=focus_directory,
         crop_type="spectra", crop_on="1-20")
    simple.mtd.clear()


#  substitute for using table workspace as ref-files as tolerance doesn't work on compareworkspace for tableworkspaces
def _compare_tableworkspaces(workspace, ref_file, delta):
    loaded = simple.Load(os.path.join(ref_dir, ref_file))
    ws = simple.mtd[workspace]
    passed = True
    if not (ws.rowCount() == loaded.rowCount()):
        mantid.kernel.logger.warning("number of rows in: " + workspace + " and " + ref_file + " did not match")
        passed = False
    if not (ws.columnCount() == loaded.columnCount()):
        mantid.kernel.logger.warning("number of columns in: " + workspace + " and " + ref_file + " did not match")
        passed = False
    if passed:

        for i in range(ws.columnCount()):
            newcolumn = [rel_err_less_delta(a, b, delta[i]) for a, b in zip(loaded.column(i), ws.column(i))]
            mantid.kernel.logger.warning("newcolumn = {}".format(all(newcolumn)))
            if not all(newcolumn):
                passed = False
                mantid.kernel.logger.warning("data in: " + workspace + " and " + ref_file + " did not match")

    if not (ws.getColumnNames() == loaded.getColumnNames()):
        mantid.kernel.logger.warning("Column names in: " + workspace + " and " + ref_file + " did not match")
        passed = False

    return passed


# borrowed from EnggCalibrationTest
def rel_err_less_delta(val, ref, epsilon):
    """
    Checks that a value 'val' does not differ from a reference value 'ref' by 'epsilon'
    or more. This method compares the relative error. An epsilon of 0.1 means a relative
    difference of 10 % = 100*0.1 %

    @param val :: value obtained from a calculation or algorithm
    @param ref :: (expected) reference value
    @param epsilon :: acceptable relative error (error tolerance)

    @returns if val differs in relative terms from ref by less than epsilon
    """
    if 0 == ref:
        return abs(ref-val) < epsilon
    check = (abs((ref - val) / ref) < epsilon)

    mantid.kernel.logger.warning("check value = {}".format((abs((ref - val) / ref))))
    mantid.kernel.logger.warning("ref = {}".format(epsilon))
    if not check:
        mantid.kernel.logger.warning("Value '{0}' differs from reference '{1}' by more than required epsilon '{2}' "
                                     "(relative)".format(val, ref, epsilon))

    return check
