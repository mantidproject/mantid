from __future__ import (absolute_import, division, print_function)

import os
import yaml

import isis_powder.routines.common


def get_calibration_dict(run_number, file_path):
    config_file = _open_yaml_file(file_path)
    run_key = _find_dictionary_key(dict_to_search=config_file, run_number=run_number)

    if not run_key:
        raise ValueError("Run number " + str(run_number) + " not recognised in calibration mapping")

    return config_file[run_key]


def _open_yaml_file(file_path):
    if not os.path.isfile(file_path):
        raise ValueError("Calibration mapping file not found at user specified path of:\n" + str(file_path) + '\n')

    read_config = None

    with open(file_path, 'r') as input_stream:
        try:
            read_config = yaml.load(input_stream)
        except yaml.YAMLError as exception:
            print(exception)
            raise RuntimeError("Failed to parse POLARIS calibration YAML file")

    return read_config


def _find_dictionary_key(dict_to_search, run_number):

    for key in dict_to_search:
        generated_runs = isis_powder.routines.common.generate_run_numbers(run_number_string=key)
        if run_number in generated_runs:
            return key

    return None
