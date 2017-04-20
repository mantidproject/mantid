from __future__ import (absolute_import, division, print_function)


def calibration_file_sanity_check(yaml_dict, file_path):
    # Check that the dictionary has data
    if not yaml_dict:
        raise ValueError("YAML dictionary appear to be empty at:\n" + str(file_path))

    # Check that we only have one unbounded range at maximum
    unbound_key_exists = _does_single_unbound_key_exist(yaml_dict)
    if unbound_key_exists:
        _is_unbound_key_sane(yaml_dict)


def _does_single_unbound_key_exist(keys):
    seen_unbounded_key = None

    for key in keys:
        key_string = str(key)
        # The user can have multiple ranges but the last one is the only one which can be unbounded
        if _is_run_range_key_unbounded(key_string):
            if not seen_unbounded_key:
                seen_unbounded_key = key_string
            else:
                raise ValueError("Seen multiple unbounded keys in mapping file: " + key_string + " and " +
                                 seen_unbounded_key)
    return True if seen_unbounded_key else False


def _is_unbound_key_sane(keys):
    unbounded_lower_value = 0
    largest_seen_bound_value = 0
    for key in keys:
        split_key = str(key).split('-')
        if _is_run_range_key_unbounded(key):
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


def _is_run_range_key_unbounded(key):
    split_key = str(key).split('-')
    return True if split_key[-1] == '' else False
