from __future__ import (absolute_import, division, print_function)

import os
import stresstesting
import shutil

import mantid.simpleapi as mantid
from mantid import config

from isis_powder import Gem

DIRS = config['datasearch.directories'].split(';')

# Setup various path details

inst_name = "GEM"
# Relative to system data folder
working_folder_name = "ISIS_Powder"

# Relative to working folder
input_folder_name = "input"
output_folder_name = "output"

# Relative to input folder
calibration_folder_name = os.path.join("calibration", inst_name.lower())
calibration_map_rel_path = os.path.join("yaml_files", "gem_system_test_mapping.yaml")
spline_rel_path = os.path.join("17_1", "VanSplined_83608_offsets_2011_cycle111b.cal.nxs")

# Generate paths for the tests
# This implies DIRS[0] is the system test data folder
working_folder = os.path.join(DIRS[0], working_folder_name)

input_folder = os.path.join(working_folder, input_folder_name)
output_folder = os.path.join(working_folder, output_folder_name)

calibration_map_path = os.path.join(input_folder, calibration_map_rel_path)
calibration_folder = os.path.join(input_folder, calibration_folder_name)
spline_path = os.path.join(calibration_folder, spline_rel_path)


class CreateVanadiumTest(stresstesting.MantidStressTest):

    calibration_results = None
    existing_config = config['datasearch.directories']

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        self.calibration_results = run_vanadium_calibration()

    def validate(self):
        return calibration_validator(self, self.calibration_results)

    def cleanup(self):
        try:
            spline_folder = os.path.join(calibration_folder, "17_1")
            _try_delete(output_folder)
            _try_delete(os.path.join(spline_folder, "VanSplined_83608_offsets_2011_cycle111b.cal.nxs"))
            _try_delete(os.path.join(spline_folder, "VanSplined_83664_offsets_2011_cycle111b.cal.nxs"))
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
        return focus_validation(self, self.focus_results)

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_folder)
        finally:
            config['datasearch.directories'] = self.existing_config
            mantid.mtd.clear()


def _gen_required_files():
    required_run_numbers = ["83605", "83607", "83608",  # create_van : PDF mode
                            "83664", "83665", "83666",  # create_van : Rietveld mode
                            "83605", "83608_splined"]  # File to focus (Si)

    # Generate file names of form "INSTxxxxx.nxs"
    input_files = [os.path.join(input_folder, (inst_name + number + ".nxs")) for number in required_run_numbers]
    input_files.append(calibration_map_path)
    return input_files


def run_vanadium_calibration():
    vanadium_run = 83605  # Choose arbitrary run in the cycle 17_1

    pdf_inst_obj = setup_inst_object(mode="PDF")
    rietveld_inst_obj = setup_inst_object(mode="Rietveld")

    # Run create vanadium twice to ensure we get two different output splines / files
    pdf_inst_obj.create_vanadium(first_cycle_run_no=vanadium_run,
                                 do_absorb_corrections=True, multiple_scattering=False)
    rietveld_inst_obj.create_vanadium(first_cycle_run_no=vanadium_run,
                                      do_absorb_corrections=True, multiple_scattering=False)

    # Check the spline looks good and was saved
    if not os.path.exists(spline_path):
        raise RuntimeError("Could not find output spline at the following path: " + spline_path)
    splined_ws = mantid.Load(Filename=spline_path)

    return splined_ws


def run_focus():
    run_number = 83605
    sample_empty = 83608  # Use the vanadium empty again to make it obvious
    sample_empty_scale = 0.5  # Set it to 50% scale

    # Copy the required splined file into place first (instead of relying on generated one)
    splined_file_name = "GEM83608_splined.nxs"

    original_splined_path = os.path.join(input_folder, splined_file_name)
    shutil.copy(original_splined_path, spline_path)

    inst_object = setup_inst_object(mode="PDF")
    return inst_object.focus(run_number=run_number, input_mode="Individual", vanadium_normalisation=True,
                             do_absorb_corrections=False, sample_empty=sample_empty,
                             sample_empty_scale=sample_empty_scale)


def calibration_validator(cls, results):
    _validation_setup(cls)

    # Get the name of the grouped workspace list
    reference_file_name = "ISIS_Powder-GEM-VanSplined_83608_offsets_2011_cycle111b.cal.nxs"
    return _compare_ws(reference_file_name=reference_file_name, results=results)


def focus_validation(cls, results):
    _validation_setup(cls)

    reference_file_name = "ISIS_Powder-GEM83605_FocusSempty.nxs"
    return _compare_ws(reference_file_name=reference_file_name, results=results)


def _compare_ws(reference_file_name, results):
    ref_ws = mantid.Load(Filename=reference_file_name)

    is_valid = True if len(results) > 0 else False

    for ws, ref in zip(results, ref_ws):
        if not (mantid.CompareWorkspaces(Workspace1=ws, Workspace2=ref)):
            is_valid = False
            print (ws.getName() + " was not equal to: " + ref.getName())

    return is_valid


def _validation_setup(cls):
    cls.disableChecking.append('Instrument')
    cls.disableChecking.append('Sample')
    cls.disableChecking.append('SpectraMap')


def setup_mantid_paths():
    config['datasearch.directories'] += ";" + input_folder


def setup_inst_object(mode):
    user_name = "Test"

    inst_obj = Gem(user_name=user_name, calibration_mapping_file=calibration_map_path,
                   calibration_directory=calibration_folder, output_directory=output_folder, mode=mode)
    return inst_obj


def _try_delete(path):
    try:
        # Use this instead of os.remove as we could be passed a non-empty dir
        shutil.rmtree(path)
    except OSError as err:
        print ("Could not delete output file at: ", path)
