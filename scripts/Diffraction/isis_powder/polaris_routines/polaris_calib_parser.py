from __future__ import (absolute_import, division, print_function)

import os
import yaml

import isis_powder.routines.common


def get_calibration_dict(run_number, file_path):
    config_file = _open_yaml_file(file_path)
    _calibration_sanity_check(config_file)
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
        if _key_is_unbounded(key):  # Have an unbounded run don't generate numbers
            split_key = str(key).split('-')
            lower_key_bound = int(split_key[-2])
            if run_number > lower_key_bound:
                return key

        generated_runs = isis_powder.routines.common.generate_run_numbers(run_number_string=key)
        if run_number in generated_runs:
            return key

    return None


def _key_is_unbounded(key):
    split_key = str(key).split('-')
    return True if split_key[-1] == '' else False


def _calibration_sanity_check(yaml_dict):
    # Check that we only have one unbounded range at maximum
    unbound_key_exists = _does_single_unbound_key_exist(yaml_dict)
    if unbound_key_exists:
        _is_unbound_key_sane(yaml_dict)


def _does_single_unbound_key_exist(keys):
    seen_unbounded_key = None

    for key in keys:
        key_string = str(key)
        # The user can have multiple ranges but the last one is the only one which can be unbounded
        if _key_is_unbounded(key_string) and not seen_unbounded_key:
            seen_unbounded_key = key_string
        elif _key_is_unbounded(key_string) and seen_unbounded_key:
            raise ValueError("Seen multiple unbounded keys in mapping file: " + key_string + " and " +
                             seen_unbounded_key)

    return True if seen_unbounded_key else False


def _is_unbound_key_sane(keys):
    unbounded_lower_value = 0
    largest_seen_bound_value = 0
    for key in keys:
        split_key = str(key).split('-')
        if _key_is_unbounded(key):
            unbounded_lower_value = int(split_key[-2])  # Get the second to last element which is the lower bounds
        else:
            # This isn't perfect but it avoid us generating loads of lists of ints.
            for value in split_key:
                if value.isdigit() and int(value) > largest_seen_bound_value:
                    largest_seen_bound_value = int(value)

    if unbounded_lower_value < largest_seen_bound_value:
        # We have a bounded value in an unbounded range
        raise ValueError("Found a run range in calibration mapping overlaps an unbounded run range." +
                         "\nThe value " + str(largest_seen_bound_value) + " was found and is greater than " +
                         str(unbounded_lower_value))
