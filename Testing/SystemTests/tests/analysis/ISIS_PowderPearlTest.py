from __future__ import (absolute_import, division, print_function)

import os
import stresstesting
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


class CreateVanadiumTest(stresstesting.MantidStressTest):

    calibration_results = None
    existing_config = config['datasearch.directories']

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        self.calibration_results = run_vanadium_calibration()

    def validate(self):
        return calibration_validator(self.calibration_results)

    def cleanup(self):
        try:
            _try_delete(output_dir)
            _try_delete(spline_path)
        finally:
            mantid.mtd.clear()
            config['datasearch.directories'] = self.existing_config


class FocusTest(stresstesting.MantidStressTest):

    focus_results = None
    existing_config = config['datasearch.directories']

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        self.focus_results = run_focus()

    def validate(self):
        return focus_validation(self.focus_results)

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            config['datasearch.directories'] = self.existing_config
            mantid.mtd.clear()


def _gen_required_files():
    required_run_numbers = ["98472", "98485",  # create_van
                            "98507", "98472_splined"]  # Focus (Si)

    # Generate file names of form "INSTxxxxx.nxs" - PEARL requires 000 padding
    input_files = [os.path.join(input_dir, (inst_name + "000" + number + ".nxs")) for number in required_run_numbers]
    input_files.append(calibration_map_path)
    return input_files


def run_vanadium_calibration():
    vanadium_run = 98507  # Choose arbitrary run in the cycle 17_1

    inst_obj = setup_inst_object(mode="tt70")

    # Run create vanadium twice to ensure we get two different output splines / files
    inst_obj.create_vanadium(run_in_cycle=vanadium_run, do_absorb_corrections=True)

    # Check the spline looks good and was saved
    if not os.path.exists(spline_path):
        raise RuntimeError("Could not find output spline at the following path: " + spline_path)
    splined_ws = mantid.Load(Filename=spline_path)

    return splined_ws


def run_focus():
    run_number = 98507
    attenuation_file_name = "PRL112_DC25_10MM_FF.OUT"

    # Copy the required splined file into place first (instead of relying on generated one)
    splined_file_name = "PEARL00098472_splined.nxs"

    attenuation_path = os.path.join(calibration_dir, attenuation_file_name)
    original_splined_path = os.path.join(input_dir, splined_file_name)
    shutil.copy(original_splined_path, spline_path)

    inst_object = setup_inst_object(mode="tt70")
    return inst_object.focus(run_number=run_number, vanadium_normalisation=True,
                             perform_attenuation=True, attenuation_file_path=attenuation_path,
                             focus_mode="Trans")


def calibration_validator(results):
    # Get the name of the grouped workspace list
    reference_file_name = "ISIS_Powder-PEARL00098472_splined.nxs"
    return _compare_ws(reference_file_name=reference_file_name, results=results)


def focus_validation(results):
    reference_file_name = "ISIS_Powder-PEARL00098507_tt70Atten.nxs"
    return _compare_ws(reference_file_name=reference_file_name, results=results)


def _compare_ws(reference_file_name, results):
    ref_ws = mantid.Load(Filename=reference_file_name)

    is_valid = True if len(results) > 0 else False

    for ws, ref in zip(results, ref_ws):
        if not (mantid.CompareWorkspaces(Workspace1=ws, Workspace2=ref)):
            is_valid = False
            print (ws.getName() + " was not equal to: " + ref.getName())

    return is_valid


def setup_mantid_paths():
    config['datasearch.directories'] += ";" + input_dir


def setup_inst_object(mode):
    user_name = "Test"

    inst_obj = Pearl(user_name=user_name, calibration_mapping_file=calibration_map_path, long_mode=False,
                     calibration_directory=calibration_dir, output_directory=output_dir, tt_mode=mode)
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
