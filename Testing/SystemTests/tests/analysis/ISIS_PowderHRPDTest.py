# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import os
import platform
import shutil
import systemtesting

import mantid.simpleapi as mantid
from mantid import config

from isis_powder import HRPD, SampleDetails

DIRS = config['datasearch.directories'].split(';')
user_name = "Test"
cycle_number = "16_5"

# Setup various path details

inst_name = "HRP"
# Relative to system data folder
working_folder_name = "ISIS_Powder"

# Relative to working folder
input_folder_name = "input"
output_folder_name = "output"

# Generate paths for the tests
# This implies DIRS[0] is the system test data folder
working_dir = os.path.join(DIRS[0], working_folder_name)

input_dir = os.path.join(working_dir, input_folder_name)
output_dir = os.path.join(working_dir, output_folder_name)

# Relative to input folder
calibration_folder_name = os.path.join("calibration", inst_name.lower())
calibration_map_rel_path = os.path.join("yaml_files", "hrpd_system_test_mapping.yaml")
spline_rel_path = os.path.join(cycle_number, "VanSplined_66031_hrpd_new_072_01_corr.cal.nxs")

calibration_map_path = os.path.join(input_dir, calibration_map_rel_path)
calibration_dir = os.path.join(input_dir, calibration_folder_name)
spline_path = os.path.join(calibration_dir, spline_rel_path)


class CreateVanadiumTest(systemtesting.MantidSystemTest):

    calibration_results = None
    existing_config = config['datasearch.directories']

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        self.calibration_results = run_vanadium_calibration()

    def validate(self):
        self.tolerance = 0.05  # Required for difference in spline data between operating systems
        return self.calibration_results.name(), "ISIS_Powder-HRPD-VanSplined_66031_hrpd_new_072_01_corr.cal.nxs"

    def cleanup(self):
        try:
            _try_delete(output_dir)
            _try_delete(spline_path)
        finally:
            mantid.mtd.clear()
            config['datasearch.directories'] = self.existing_config


class FocusTest(systemtesting.MantidSystemTest):

    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        self.focus_results = run_focus()

    def validate(self):
        if platform.system() == "Darwin":  # OSX requires higher tolerance for splines
            self.tolerance = 0.4
        else:
            self.tolerance = 0.05
        return self.focus_results.name(), "HRPD66063_focused.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            config["datasearch.directories"] = self.existing_config
            mantid.mtd.clear()


def _gen_required_files():
    required_run_numbers = gen_required_run_numbers()
    input_files = [os.path.join(input_dir, (inst_name + number + ".raw")) for number in required_run_numbers]
    input_files.append(calibration_map_path)
    return input_files


def gen_required_run_numbers():
    return ["66028",  # Sample empty
            "66031",  # Vanadium
            "66063"]  # Run to focus


def run_vanadium_calibration():
    vanadium_run = 66031  # Choose arbitrary run from cycle 16_5
    inst_obj = setup_inst_object()
    inst_obj.create_vanadium(first_cycle_run_no=vanadium_run,
                             do_absorb_corrections=True,
                             multiple_scattering=False,
                             window="10-110")

    # Check the spline looks good and was saved
    if not os.path.exists(spline_path):
        raise RuntimeError("Could not find output spline at the following path: {}".format(spline_path))
    splined_ws = mantid.Load(Filename=spline_path)

    return splined_ws


def run_focus():
    [sample_empty, _, run_number] = gen_required_run_numbers()
    sample_empty_scale = 1

    # Copy the required spline file into place first (instead of relying on the generated one)
    splined_file_name = "HRPD66031_splined.nxs"

    original_splined_path = os.path.join(input_dir, splined_file_name)
    shutil.copy(original_splined_path, spline_path)

    inst_object = setup_inst_object()

    sample = SampleDetails(shape="cylinder", center=[1, 5, 1], height=1, radius=1)
    sample.set_material(chemical_formula="Si")
    inst_object.set_sample_details(sample=sample)

    return inst_object.focus(run_number=run_number, window="10-110", sample_empty=sample_empty,
                             sample_empty_scale=sample_empty_scale, vanadium_normalisation=True,
                             do_absorb_corrections=True, multiple_scattering=False)


def setup_inst_object():
    inst_obj = HRPD(user_name=user_name, calibration_mapping_file=calibration_map_path,
                    calibration_directory=calibration_dir, output_directory=output_dir)
    return inst_obj


def setup_mantid_paths():
    config['datasearch.directories'] += ";" + input_dir
    config['datasearch.directories'] += ";" + os.path.join(calibration_dir, cycle_number)


def _try_delete(path):
    try:
        # Use this instead of os.remove as we could be passed a non-empty dir
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except OSError:
        print("Could not delete output file at: ", path)
