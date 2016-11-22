from __future__ import (absolute_import, division, print_function)

import os

import yaml

import isis_powder.routines.common


def get_calibration_dict(run_number):
    config_file = _open_yaml_file()

    # First check exceptions list as it should be shorter
    exception_key = _check_if_run_is_exception(config_handle=config_file, run_number=run_number)
    if exception_key:
        exceptions_dict = config_file["exceptions"]
        return exceptions_dict[exception_key]

    # Otherwise parse the entire YAML file
    run_key = _find_dictionary_key(dict_to_search=config_file, run_number=run_number)

    # If we have not found the run in either
    if not run_key:
        raise ValueError("Run number " + str(run_number) + " not recognised in calibration mapping")

    return config_file[run_key]


def _open_yaml_file():
    config_file_name = "polaris_calibration.yaml"
    config_file_path = os.path.join(os.path.dirname(__file__), config_file_name)

    read_config = None

    with open(config_file_path, 'r') as input_stream:
        try:
            read_config = yaml.load(input_stream)
        except yaml.YAMLError as exception:
            print(exception)
            raise RuntimeError("Failed to parse POLARIS calibration YAML file")

    return read_config


def _check_if_run_is_exception(config_handle, run_number):
    try:
        exceptions_dict = config_handle["exceptions"]
    except KeyError:
        return None

    return _find_dictionary_key(dict_to_search=exceptions_dict, run_number=run_number)


def _find_dictionary_key(dict_to_search, run_number):

    for key in dict_to_search:
        generated_runs = isis_powder.routines.common.generate_run_numbers(run_number_string=key)
        if run_number in generated_runs:
            return key

    return None
