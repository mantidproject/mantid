from __future__ import (absolute_import, division, print_function)

from itertools import groupby
from operator import itemgetter
import re

delimiter = ","
range_separator = "-"
run_string_regex = "^[0-9]*([0-9]+[,-]{0,1})*[0-9]+$"
max_run_list_size = 100


def _remove_duplicates_from_list(run_list):
    return list(set(run_list))


def run_list_to_string(run_list):
    """
    Converts a list of runs into a formatted string using a delimiter/range separator
    :param run_list: list of integers
    :return: string representation
    """
    run_list = _remove_duplicates_from_list(run_list)
    run_list = [i for i in run_list if i >= 0]
    run_list.sort()
    if len(run_list) > max_run_list_size:
        raise IndexError("Too many runs (" + str(len(run_list)) + ") must be <" + str(max_run_list_size))

    range_list = []
    for k, g in groupby(enumerate(run_list), lambda (i, x): i - x):
        concurrent_range = map(itemgetter(1), g)
        if len(concurrent_range) > 1:
            range_list += [str(concurrent_range[0]) + range_separator + str(concurrent_range[-1])]
        else:
            range_list += [str(concurrent_range[0])]
    return delimiter.join(range_list)


def validate_run_string(run_string):
    """
    Use a regular expression to check if a string represents a series of runs
    :param run_string: string representation of a series of runs
    :return: bool
    """
    if run_string == "":
        return True
    if re.match(run_string_regex, run_string) is not None:
        return True
    return False


def run_string_to_list(run_string):
    """
    Does the opposite of run_list_to_string(), taking a string representation of a series of runs
    and producing an ordered list of unique runs. Calls validate_run_string().
    :param run_string: string, a series of runs
    :return: list of integers
    """
    if not validate_run_string(run_string):
        raise IndexError(run_string + " is not a valid run string")
    run_list = []
    run_string_list = run_string.split(delimiter)
    for runs in run_string_list:
        split_runs = runs.split(range_separator)
        if len(runs) == 1:
            run_list += [int(runs)]
        else:
            range_max = int(split_runs[-1])
            range_min = int(split_runs[0])
            if (range_max - range_min) > max_run_list_size:
                raise IndexError(
                    "Too many runs (" + str(range_max - range_min) + ") must be <" + str(max_run_list_size))
            else:
                run_list += [range_min + i for i in range(range_max - range_min + 1)]
    run_list = _remove_duplicates_from_list(run_list)
    run_list.sort()
    return run_list


def increment_run_list(run_list, increment_by=1):
    """
    Takes a list of runs, and increments the list by adding a given number of runs, starting at one
    after the highest run in the list.
    :param run_list: list of integers
    :param increment_by: integer, the number of runs to increase the list by
    :return: modified list of integers
    """
    if increment_by <= 0:
        return run_list
    highest_run = max(run_list)
    run_list.append(highest_run + 1)
    return increment_run_list(run_list, increment_by - 1)


def decrement_run_list(run_list, decrement_by=1):
    """
    Takes a list of runs, and decrements the list by adding a given number of runs, starting at one
    before the lowest run in the list. Stops at 0.
    :param run_list: list of integers
    :param decrement_by: integer, the number of runs to increase the list by
    :return: modified list of integers
    """
    if decrement_by <= 0:
        return run_list
    lowest_run = min(run_list)
    if lowest_run < 1:
        return run_list
    run_list.append(lowest_run - 1)
    return decrement_run_list(run_list, decrement_by - 1)


def increment_run(run, increment_by=1):
    return run + increment_by


def increment_run_string(run_string, increment_by=1):
    """
    Takes a string representation of runs, and adds a number of runs starting at one after the highest.
    :param run_string:
    :param increment_by: integer, number of sequential runs to add
    :return: modified string of runs (reformatted according to run_list_to_string())
    """
    run_list = run_string_to_list(run_string)
    run_list = increment_run_list(run_list, increment_by)
    return run_list_to_string(run_list)


def decrement_run(run, decrement_by=1):
    return max(0, run - decrement_by)


def decrement_run_string(run_string, decrement_by=1):
    """
    Takes a string representation of runs, and adds a number of runs starting at one before the lowest and decrementing.
    :param run_string:
    :param decrement_by: integer, number of sequential runs to add
    :return: modified string of runs (reformatted according to run_list_to_string())
    """
    run_list = run_string_to_list(run_string)
    run_list = decrement_run_list(run_list, decrement_by)
    return run_list_to_string(run_list)
