from __future__ import (absolute_import, division, print_function)

import os
import stresstesting

import mantid.simpleapi as mantid
from mantid import config

from isis_powder import polaris

DIRS = config['datasearch.directories'].split(';')


class isis_powder_PolarisVanadiumCalTest(stresstesting.MantidStressTest):

    calibration_results = None

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        self.calibration_results = _run_vanadium_calibration()

    def validate(self):
        return _calibration_validation(self, self.calibration_results)

    def cleanup(self):
        # TODO clean up reference files properly
        _clean_up()


class isis_powder_PolarisFocusTest(stresstesting.MantidStressTest):

    focus_results = None

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        _run_vanadium_calibration()
        self.focus_results = _run_focus()

    def validation(self):
        return _calibration_validation(self, self.focus_results)

    def cleanup(self):
        pass
        # TODO


def _gen_required_files():
    input_files = ["POLARIS/POL78338.raw",
                   "POLARIS/POL78339.raw",
                   "POLARIS/POL79514.raw"]
    return input_files


def _run_vanadium_calibration():
    vanadium_run = 78338
    empty_run = 78339
    output_file_name = _get_calibration_output_name()
    gen_absorb = True

    polaris_obj = setup_polaris_instrument()
    # Try it without an output name
    polaris_obj.create_calibration_vanadium(vanadium_runs=vanadium_run, empty_runs=empty_run,
                                            gen_absorb_correction=gen_absorb)

    return polaris_obj.create_calibration_vanadium(vanadium_runs=vanadium_run, empty_runs=empty_run,
                                                   output_file_name=output_file_name, gen_absorb_correction=gen_absorb)


def _run_focus():
    run_number = 79514
    polaris_obj = setup_polaris_instrument()
    return polaris_obj.focus(run_number=run_number, focus_mode="")


def _calibration_validation(cls, results):
    _validation_setup(cls)
    output_full_path = os.path.join(_get_calibration_dir(), _get_calibration_output_name())
    ws_to_validate_output_name = "pearl_van_cal_output"
    mantid.LoadNexus(Filename=output_full_path, OutputWorkspace=ws_to_validate_output_name)

    reference_file_name = "POLARIS_PowderDiffVanCalibration.nxs"
    return ws_to_validate_output_name, reference_file_name


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

    calibration_dir = _get_calibration_dir()
    raw_data_dir = os.path.join(DIRS[0], "POLARIS")
    output_dir = _get_output_dir()

    polaris_obj = polaris.Polaris(user_name=user_name, calibration_dir=calibration_dir, raw_data_dir=raw_data_dir,
                                  output_dir=output_dir)
    return polaris_obj


def _get_calibration_output_name():
    return "system_test_polaris_van_cal.nxs"


def _get_output_dir():
    return os.path.join(DIRS[0], "POLARIS/DataOut")


def _get_calibration_dir():
    return os.path.join(DIRS[0], "POLARIS/test/Cycle_15_2/Calibration")
