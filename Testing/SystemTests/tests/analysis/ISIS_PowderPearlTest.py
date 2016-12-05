from __future__ import (absolute_import, division, print_function)

import os
import stresstesting

import mantid.simpleapi as mantid
from mantid import config

from isis_powder.pearl import Pearl

DIRS = config['datasearch.directories'].split(';')

class VanadiumCalibrationTest(stresstesting.MantidStressTest):

    calibration_results = None
    existing_config = config['datasearch.directories']

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


def _calibration_validation(cls, results):
    _validation_setup(cls)
    results_group_name = results.name()
    reference_file_name = "ISIS_PowderPEARL95634-95647_Van_Cal.nxs"
    return results_group_name, reference_file_name


def _validation_setup(cls):
    cls.disableChecking.append('Instrument')
    cls.disableChecking.append('Sample')
    cls.disableChecking.append('SpectraMap')


def _gen_required_files():
    input_file_dir = _get_input_dir()
    van_files_names = ["PEARL000" + str(num) for num in range(95634, 95647)]
    empty_file_names = ["PEARL000" + str(num) for num in range(95648, 95654)]

    required_files = []
    for van_name, empty_name in zip(van_files_names, empty_file_names):
        required_files.append(os.path.join(input_file_dir, van_name))
        required_files.append(os.path.join(input_file_dir, empty_name))

    return required_files


def _get_calibration_dir():
    return os.path.join(DIRS[0], "PEARL", "Calibration")


def _get_input_dir():
    return os.path.join(DIRS[0], "PEARL", "InputData")


def _get_output_dir():
    return os.path.join(DIRS[0], "PEARL", "SystemTestOutput")


def _run_vanadium_calibration():
    vanadium_runs = "95634_95647"
    empty_runs = "95648_95654"

    pearl_obj = _setup_pearl_instrument(tt_mode=None)
    results = pearl_obj.create_calibration_vanadium(vanadium_runs=vanadium_runs, empty_runs=empty_runs,
                                                    do_absorb_corrections=True)
    return results


def _setup_pearl_instrument(tt_mode):
    user_name = "Test"
    config['datasearch.directories'] += ";" + _get_input_dir()
    pearl_obj = Pearl(user_name=user_name, tt_mode=tt_mode,
                      calibration_dir=_get_calibration_dir(), output_dir=_get_output_dir())
    return pearl_obj

