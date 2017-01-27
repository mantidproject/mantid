from __future__ import (absolute_import, division, print_function)

import os
import stresstesting

import mantid.simpleapi as mantid
from mantid import config

from isis_powder import polaris

DIRS = config['datasearch.directories'].split(';')


class VanadiumCalibrationTest(stresstesting.MantidStressTest):

    calibration_results = None
    existing_config = config['datasearch.directories']

    # TODO
    # Test disabled whilst in development as we were having to update the reference file on a daily basis
    def skipTests(self):
        return True

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        self.calibration_results = _run_vanadium_calibration()

    def validate(self):
        return _calibration_validation(self, self.calibration_results)

    def cleanup(self):
        # TODO clean up reference files properly
        config['datasearch.directories'] = self.existing_config
        # _clean_up()


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
    input_files = ["POLARIS/POL78338.raw",
                   "POLARIS/POL78339.raw",
                   "POLARIS/POL79514.raw"]
    return input_files


def _run_vanadium_calibration():
    vanadium_run = 95598

    polaris_obj = setup_polaris_instrument()
    # Try it without an output name

    return polaris_obj.create_calibration_vanadium(run_in_range=vanadium_run, do_absorb_corrections=True,
                                                   generate_absorb_corrections=True)


def _run_focus():
    run_number = 95599
    polaris_obj = setup_polaris_instrument()
    return polaris_obj.focus(run_number=run_number, input_mode="Individual", do_van_normalisation=True)


def _calibration_validation(cls, results):
    _validation_setup(cls)
    results_name = results[0].name()
    reference_file_name = "ISIS_Powder-POLARIS78338_Van_Cal.nxs"
    return results_name, reference_file_name


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


def _clean_up():
    output_file_path = _get_calibration_dir() + _get_calibration_output_name()
    try:
        os.remove(output_file_path)
    except OSError:
        print ("Could not delete output file at: ", output_file_path)


def setup_polaris_instrument():
    user_name = "Test"
    calibration_mapping_file_name = "polaris_calibration.yaml"

    calibration_dir = _get_calibration_dir()
    calibration_mapping_path = os.path.join(calibration_dir, calibration_mapping_file_name)
    output_dir = _get_output_dir()

    path_to_add = os.path.join(DIRS[0], "POLARIS")
    config['datasearch.directories'] += ";" + path_to_add
    polaris_obj = polaris.Polaris(user_name=user_name, chopper_on=True, apply_solid_angle=False,
                                  calibration_directory=calibration_dir, output_directory=output_dir,
                                  calibration_mapping_file=calibration_mapping_path)
    return polaris_obj


def _get_calibration_output_name():
    return "system_test_polaris_van_cal.nxs"


def _get_output_dir():
    return os.path.join(DIRS[0], "POLARIS/DataOut")


def _get_calibration_dir():
    return os.path.join(DIRS[0], "POLARIS/Calibration")
