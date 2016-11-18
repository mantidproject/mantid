from __future__ import (absolute_import, division, print_function)
from six.moves import xrange

import os
import yaml


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
        run_generator = _parse_number_key(input_string=key)
        for run_list in run_generator:
            if run_number in run_list:
                return key

    return None


def _parse_number_key(input_string):
    # Expands run numbers of the form 1-10, 12, 14-20, 23 to 1,2,3,..,8,9,10,12,14,15,16...,19,20,23

    string_to_parse = str(input_string).strip()

    for entry in string_to_parse.split(','):
        # Split between comma separated values
        numbers = entry.split('-')
        # Check if we are using a dash separator and return the range between those values
        if len(numbers) == 1:
            yield numbers
        elif len(numbers) == 2:
            # Add 1 so it includes the final number '-' range
            yield xrange(int(numbers[0]), int(numbers[-1]) + 1)
        else:
            raise ValueError("The run number " + str(entry) + " is incorrect in calibration mapping")
