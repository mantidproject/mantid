# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import os
import systemtesting
import shutil

import mantid.simpleapi as mantid
from mantid import config

from isis_powder import Pearl

DIRS = config['datasearch.directories'].split(';')

# Setup various path details

inst_name = "PEARL"
# Relative to system data folder
working_folder_name = "ISIS_Powder"

# Relative to working folder
input_folder_name = "input"
output_folder_name = "output"

# Relative to input folder
calibration_folder_name = os.path.join("calibration", inst_name.lower())
calibration_map_rel_path = os.path.join("yaml_files", "pearl_system_test_mapping.yaml")
spline_rel_path = os.path.join("17_1", "VanSplined_98472_tt70_pearl_offset_16_4.cal.nxs")

# Generate paths for the tests
# This implies DIRS[0] is the system test data folder
working_dir = os.path.join(DIRS[0], working_folder_name)

input_dir = os.path.join(working_dir, input_folder_name)
output_dir = os.path.join(working_dir, output_folder_name)

calibration_map_path = os.path.join(input_dir, calibration_map_rel_path)
calibration_dir = os.path.join(input_dir, calibration_folder_name)
spline_path = os.path.join(calibration_dir, spline_rel_path)


class _CreateVanadiumTest(systemtesting.MantidSystemTest):

    existing_config = config['datasearch.directories']
    focus_mode = None

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        inst_obj = setup_inst_object(tt_mode="tt70", focus_mode="trans")
        run_vanadium_calibration(inst_obj, focus_mode=self.focus_mode)

        # Make sure that inst settings reverted to the default after create_vanadium
        self.assertEqual(inst_obj._inst_settings.focus_mode, "trans")

    def skipTests(self):
        # Don't actually run this test, as it is a dummy for the focus-mode-specific tests
        return True

    def validate(self):
        self.tolerance = 0.05  # Required for difference in spline data between operating systems
        return "PEARL98472_tt70-Results-D-Grp", "ISIS_Powder_PRL98472_tt70_{}.nxs".format(self.focus_mode), \
               "Van_spline_data_tt70", "ISIS_Powder-PEARL00098472_splined.nxs"

    def cleanup(self):
        try:
            _try_delete(output_dir)
            _try_delete(spline_path)
        finally:
            mantid.mtd.clear()
            config['datasearch.directories'] = self.existing_config


class CreateVanadiumAllTest(_CreateVanadiumTest):
    focus_mode = "all"

    def skipTests(self):
        return False


class CreateVanadiumTransTest(_CreateVanadiumTest):
    focus_mode = "trans"

    def skipTests(self):
        return False


class CreateVanadiumGroupsTest(_CreateVanadiumTest):
    focus_mode = "groups"

    def skipTests(self):
        return False


class CreateVanadiumModsTest(_CreateVanadiumTest):
    focus_mode = "mods"

    def skipTests(self):
        return False


class FocusTest(systemtesting.MantidSystemTest):

    focus_results = None
    existing_config = config['datasearch.directories']

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        inst_object = setup_inst_object(tt_mode="tt88", focus_mode="Trans")
        self.focus_results = run_focus(inst_object, tt_mode="tt70")

        # Make sure that inst settings reverted to the default after focus
        self.assertEqual(inst_object._inst_settings.tt_mode, "tt88")

    def validate(self):
        self.tolerance = 5e-9  # Required for difference in spline data between operating systems
        return "PEARL98507_tt70-Results-D-Grp", "ISIS_Powder-PEARL00098507_tt70Atten.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            config['datasearch.directories'] = self.existing_config
            mantid.mtd.clear()

class FocusLongThenShortTest(systemtesting.MantidSystemTest):

    focus_results = None
    existing_config = config['datasearch.directories']

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        inst_object = setup_inst_object(tt_mode="tt88", focus_mode="Trans")
        inst_object.focus(run_number=98507, vanadium_normalisation=False, do_absorb_corrections=False,
                          long_mode=True, perform_attenuation=False, tt_mode="tt70")
        self.focus_results = run_focus(inst_object, tt_mode="tt70")

        # Make sure that inst settings reverted to the default after focus
        self.assertEqual(inst_object._inst_settings.tt_mode, "tt88")
        self.assertFalse(inst_object._inst_settings.long_mode)

    def validate(self):
        self.tolerance = 5e-9  # Required for difference in spline data between operating systems
        return "PEARL98507_tt70-Results-D-Grp", "ISIS_Powder-PEARL00098507_tt70Atten.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            config['datasearch.directories'] = self.existing_config
            mantid.mtd.clear()


class FocusWithAbsorbCorrectionsTest(systemtesting.MantidSystemTest):

    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        self.focus_results = run_focus_with_absorb_corrections()

    def validate(self):
        return "PEARL98507_tt70-Results-D-Grp", "ISIS_Powder-PEARL00098507_tt70_absorb.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            config['datasearch.directories'] = self.existing_config
            mantid.mtd.clear()


class CreateCalTest(systemtesting.MantidSystemTest):

    calibration_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        inst_object = setup_inst_object(tt_mode="tt88", focus_mode="trans")
        self.calibration_results = run_create_cal(inst_object, focus_mode="all")

        # Make sure that inst_settings reverted to the default after create_cal
        self.assertEqual(inst_object._inst_settings.focus_mode, "trans")

    def validate(self):
        self.tolerance = 1e-5
        return "PRL98494_tt88_grouped", "ISIS_Powder-PEARL98494_grouped.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            config['datasearch.directories'] = self.existing_config
            mantid.mtd.clear()


def _gen_required_files():
    required_run_numbers = ["98472", "98485",  # create_van
                            "98507", "98472_splined",  # Focus (Si)
                            "98494"]  # create_cal (Ce)

    # Generate file names of form "INSTxxxxx.nxs" - PEARL requires 000 padding
    input_files = [os.path.join(input_dir, (inst_name + "000" + number + ".nxs")) for number in required_run_numbers]
    input_files.append(calibration_map_path)
    return input_files


def run_create_cal(inst_object, focus_mode):
    ceria_run = 98494
    return inst_object.create_cal(run_number=ceria_run, focus_mode=focus_mode)


def run_vanadium_calibration(inst_object, focus_mode):
    vanadium_run = 98507  # Choose arbitrary run in the cycle 17_1

    # Run create vanadium twice to ensure we get two different output splines / files
    inst_object.create_vanadium(run_in_cycle=vanadium_run, do_absorb_corrections=True, focus_mode=focus_mode)


def run_focus(inst_object, tt_mode):
    run_number = 98507
    attenuation_file_name = "PRL112_DC25_10MM_FF.OUT"

    # Copy the required splined file into place first (instead of relying on generated one)
    splined_file_name = "PEARL00098472_splined.nxs"

    attenuation_path = os.path.join(calibration_dir, attenuation_file_name)
    original_splined_path = os.path.join(input_dir, splined_file_name)
    shutil.copy(original_splined_path, spline_path)

    return inst_object.focus(run_number=run_number, vanadium_normalisation=True, do_absorb_corrections=False,
                             perform_attenuation=True, attenuation_file_path=attenuation_path, tt_mode=tt_mode)


def run_focus_with_absorb_corrections():
    run_number = 98507
    inst_object = setup_inst_object(tt_mode="tt70", focus_mode="Trans")
    return inst_object.focus(run_number=run_number, vanadium_normalisation=False, perform_attenuation=False,
                             do_absorb_corrections=True, long_mode=False)


def setup_mantid_paths():
    config['datasearch.directories'] += ";" + input_dir


def setup_inst_object(tt_mode, focus_mode):
    user_name = "Test"

    inst_obj = Pearl(user_name=user_name, calibration_mapping_file=calibration_map_path, long_mode=False,
                     calibration_directory=calibration_dir, output_directory=output_dir, tt_mode=tt_mode,
                     focus_mode=focus_mode)
    return inst_obj


def _try_delete(path):
    try:
        # Use this instead of os.remove as we could be passed a non-empty dir
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except OSError:
        print ("Could not delete output file at: ", path)
