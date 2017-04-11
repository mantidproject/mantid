from __future__ import (absolute_import, division, print_function)

import os
import yaml
from isis_powder.routines import common as common
from isis_powder.routines import yaml_sanity


def get_run_dictionary(run_number_string, file_path):
    if isinstance(run_number_string, str):
        run_number_list = common.generate_run_numbers(run_number_string=run_number_string)
        run_number_string = run_number_list[0]

    config_file = open_yaml_file_as_dictionary(file_path)
    yaml_sanity.calibration_file_sanity_check(config_file, file_path)
    run_key = _find_dictionary_key(dict_to_search=config_file, run_number=run_number_string)

    if not run_key:
        raise ValueError("Run number " + str(run_number_string) + " not recognised in calibration mapping")

    return config_file[run_key]


def is_run_range_key_unbounded(key):
    split_key = str(key).split('-')
    return True if split_key[-1] == '' else False


def open_yaml_file_as_dictionary(file_path):
    if not file_path:
        return None
    elif not os.path.isfile(file_path):
        raise ValueError("Config file not found at path of:\n" + str(file_path) + '\n ')

    read_config = None

    with open(file_path, 'r') as input_stream:
        try:
            read_config = yaml.load(input_stream)
        except yaml.YAMLError as exception:
            print(exception)
            raise RuntimeError("Failed to parse YAML file: " + str(file_path))

    return read_config


def _find_dictionary_key(dict_to_search, run_number):
    for key in dict_to_search:
        if is_run_range_key_unbounded(key):  # Have an unbounded run don't generate numbers
            split_key = str(key).split('-')
            lower_key_bound = int(split_key[-2])
            if run_number >= lower_key_bound:
                return key
        else:
            try:
                generated_runs = common.generate_run_numbers(run_number_string=key)
            except RuntimeError:
                raise ValueError("Could not parse '" + str(key) + "'\n"
                                 "This should be a range of runs in this cycle in the mapping file."
                                 " Please check your indentation if this should be within a cycle.")
            if run_number in generated_runs:
                return key

    return None
