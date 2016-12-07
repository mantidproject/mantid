from __future__ import (absolute_import, division, print_function)

import os
import warnings
import yaml
from isis_powder.routines import common as common
from isis_powder.routines import yaml_sanity


def get_run_dictionary(run_number, file_path):
    config_file = _open_yaml_file_as_dictionary(file_path)
    yaml_sanity.calibration_file_sanity_check(config_file)
    run_key = _find_dictionary_key(dict_to_search=config_file, run_number=run_number)

    if not run_key:
        raise ValueError("Run number " + str(run_number) + " not recognised in calibration mapping")

    return config_file[run_key]


def is_run_range_key_unbounded(key):
    split_key = str(key).split('-')
    return True if split_key[-1] == '' else False


def set_kwargs_from_config_file(config_path, kwargs, keys_to_find):
    if config_path:
        basic_config_dict = _open_yaml_file_as_dictionary(file_path=config_path)
    else:
        # Create an empty dictionary so we still get error checking below and nicer error messages
        basic_config_dict = {}

    # Set any unset properties:
    for key in keys_to_find:
        _get_kwarg_key_from_dict(config_dictionary=basic_config_dict, kwargs=kwargs, key=key)


def _open_yaml_file_as_dictionary(file_path):
    if not file_path or not os.path.isfile(file_path):
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
            if run_number > lower_key_bound:
                return key
        else:
            generated_runs = common.generate_run_numbers(run_number_string=key)
            if run_number in generated_runs:
                return key

    return None


def _get_kwarg_key_from_dict(config_dictionary, kwargs, key):
    error_first = "Setting with name: '"
    error_last = "' was not passed in the call or set in the basic config."
    kwarg_value = kwargs.get(key, None)
    if kwarg_value is None:
        # Only try to parse it if it wasn't passed in
        value = common.dictionary_key_helper(dictionary=config_dictionary, key=key, throws=True,
                                             exception_msg=(error_first + key + error_last))
        kwargs[key] = value
