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

# Generate paths for the tests
# This implies DIRS[0] is the system test data folder
working_folder = os.path.join(DIRS[0], working_folder_name)

input_folder = os.path.join(working_folder, input_folder_name)
output_folder = os.path.join(working_folder, output_folder_name)

calibration_map_path = os.path.join(input_folder, calibration_map_rel_path)
calibration_folder = os.path.join(input_folder, calibration_folder_name)


class VanadiumCalibrationTest(stresstesting.MantidStressTest):

    calibration_results = None
    existing_config = config['datasearch.directories']

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        self.calibration_results = _run_vanadium_calibration()

    def validate(self):
        return calibration_validator(self, self.calibration_results)

    def cleanup(self):
        clean_up()
        config['datasearch.directories'] = self.existing_config


class FocusTest(stresstesting.MantidStressTest):

    focus_results = None
    existing_config = config['datasearch.directories']

    # TODO
    # Test disabled whilst in development as we were having to update the reference file on a daily basis
    def skipTests(self):
        return True

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        _run_vanadium_calibration()
        self.focus_results = _run_focus()

    def validation(self):
        return _focus_validation(self, self.focus_results)

    def cleanup(self):
        config['datasearch.directories'] = self.existing_config
        # TODO


def _gen_required_files():
    required_run_numbers = ["83605", "83607", "83608",  # create_van : PDF mode
                            "83664", "83665", "83666"]  # create_van : Rietveld mode

    # Generate file names of form "INSTxxxxx.nxs"
    input_files = [os.path.join(input_folder, (inst_name + number + ".nxs")) for number in required_run_numbers]
    input_files.append(calibration_map_path)
    return input_files


def _run_vanadium_calibration():
    vanadium_run = 83605  # Choose arbitrary run in the cycle 17_1

    pdf_inst_obj = setup_inst_object(mode="PDF")
    rietveld_inst_obj = setup_inst_object(mode="Rietveld")

    # Run create vanadium twice to ensure we get two different output splines / files
    pdf_inst_obj.create_vanadium(first_cycle_run_no=vanadium_run,
                                 do_absorb_corrections=True, multiple_scattering=False)
    rietveld_inst_obj.create_vanadium(first_cycle_run_no=vanadium_run,
                                      do_absorb_corrections=True, multiple_scattering=False)

    # Check the spline looks good
    spline_path = os.path.join(calibration_folder, "17_1", "VanSplined_83608_offsets_2011_cycle111b.cal.nxs")
    if not os.path.exists(spline_path):
        raise RuntimeError("Could not find output spline at the following path: " + spline_path)
    splined_ws = mantid.Load(Filename=spline_path)

    return splined_ws


def _run_focus():
    run_number = 95599
    polaris_obj = setup_inst_object()
    return polaris_obj.focus(run_number=run_number, input_mode="Individual", do_van_normalisation=True)


def calibration_validator(cls, results):
    _validation_setup(cls)

    # Get the name of the grouped workspace list
    reference_file_name = "ISIS_Powder-GEM-VanSplined_83608_offsets_2011_cycle111b.cal.nxs"
    ref_ws = mantid.Load(Filename=reference_file_name)

    is_valid = True if len(results) > 0 else False

    for ws, ref in zip(results, ref_ws):
        if not (mantid.CompareWorkspaces(Workspace1=ws, Workspace2=ref)):
            is_valid = False
            print (ws.getName() + " was not equal to: " + ref.getName())

    return is_valid


def _focus_validation(cls, results):
    _validation_setup(cls)

    reference_file_name = "POLARIS_PowderFocus79514.nxs"
    focus_output_name = "Focus_results"
    mantid.GroupWorkspaces(InputWorkspaces=results, OutputWorkspace=focus_output_name)

    return focus_output_name, reference_file_name


def _validation_setup(cls):
    cls.disableChecking.append('Instrument')
    cls.disableChecking.append('Sample')
    cls.disableChecking.append('SpectraMap')


def clean_up():
    mantid.mtd.clear()

    spline_folder = os.path.join(calibration_folder, "17_1")
    try:
        shutil.rmtree(output_folder)
    except OSError:
        print("Could not delete output file at: ", output_folder)

    _try_delete(os.path.join(spline_folder, "VanSplined_83608_offsets_2011_cycle111b.cal.nxs"))
    _try_delete(os.path.join(spline_folder, "VanSplined_83664_offsets_2011_cycle111b.cal.nxs"))


def setup_mantid_paths():
    config['datasearch.directories'] += ";" + input_folder


def setup_inst_object(mode):
    user_name = "Test"

    inst_obj = Gem(user_name=user_name, calibration_mapping_file=calibration_map_path,
                   calibration_directory=calibration_folder, output_directory=output_folder, mode=mode)
    return inst_obj


def _try_delete(path):
    try:
        os.remove(path)
    except OSError:
        print ("Could not delete output file at: ", path)
