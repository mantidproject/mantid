from __future__ import (absolute_import, division, print_function)

import os
import shutil
import stresstesting

from mantid import config

from isis_powder import HRPD

DIRS = config['datasearch.directories'].split(';')

# Setup various path details

inst_name = "HRPD"
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
spline_rel_path = os.path.join("16_5", "VanSplined_66031_hrpd_new_072_01_corr.cal.nxs")

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

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        self.assertTrue(False)

    def validate(self):
        raise NotImplementedError("HRPD Focus test")


def _compare_ws(reference_file_name, results):
    ref_ws = mantid.Load(Filename=reference_file_name)
    is_valid = len(results) > 0

    for (ws, ref) in zip(results, ref_ws):
        if not mantid.CompareWorkspaces(Workspace1=ws, Workspace2=ref):
            is_valid = False
            print(ws.getName() + " was not equal to: " + ref.getName())

    return is_valid


def _gen_required_files():
    required_run_numbers = ["66845", "66846"]
    input_files = [os.path.join(input_dir, (inst_name + number + ".nxs")) for number in required_run_numbers]
    input_files.append(calibration_map_path)
    return input_files


def _try_delete(path):
    try:
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except OSError:
        print("Could not delete output file at: ", path)


def calibration_validator(results):
    reference_file_name = "ISIS_Powder-HRPD-VanSplined_66031_hrpd_new_072_01_corr.cal.nxs"
    return _compare_ws(reference_file_name, results=results)


def run_vanadium_calibration():
    vanadium_run = 66829  # Choose arbitrary run from cycle 17_1
    inst_obj = setup_inst_object()
    inst_obj.create_vanadium(first_cycle_run_no=vanadium_run,
                             do_absorb_corrections=True,
                             multiple_scattering=False,
                             window="10-110")

    # Check the spline looks good and was saved
    if not os.path.exists(spline_path):
        raise RuntimeError("Could not find output spline at the following path: " + spline_path)
    splined_ws = mantid.Load(Filename=spline_path)

    return splined_ws


def setup_inst_object():
    user_name = "Test"
    inst_obj = HRPD(user_name=user_name, calibration_mapping_file=calibration_map_path,
                    calibration_directory=calibration_dir, output_directory=output_dir)
    return inst_obj

def setup_mantid_paths():
    config['datasearch.directories'] += ";" + input_dir