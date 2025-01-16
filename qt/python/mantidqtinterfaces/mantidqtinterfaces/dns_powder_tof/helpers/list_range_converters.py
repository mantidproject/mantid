# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


def list_to_range(value_list):
    """
    Converting a list of file numbers to a string with a python range command.
    """
    if len(value_list) > 3:
        increment = value_list[1] - value_list[0]
        my_range = range(value_list[0], value_list[-1] + increment, increment)
        if value_list == list(my_range):
            return f"[*range({value_list[0]}, {value_list[-1] + increment}, {increment})]"
    return str(value_list)


def list_to_multirange(value_list):
    """
    Creating a string with python range commands from list of file numbers.
    """
    range_string = ""
    if len(value_list) > 5:
        start = 0
        increment = value_list[start + 1] - value_list[start]
        for i, value in enumerate(value_list):
            if i == len(value_list) - 1 or value + increment != value_list[i + 1]:
                end = i + 1
                if range_string:
                    range_string = " + ".join((range_string, list_to_range(value_list[start:end])))
                else:
                    range_string = list_to_range(value_list[start:end])
                start = end
                if start < len(value_list) - 1:
                    increment = value_list[start + 1] - value_list[start]
    if not range_string:
        range_string = str(value_list)
    elif range_string.count("+") == 0:
        range_string = range_string[2:-1]
    return range_string


def get_normalisation(options):
    if options.get("norm_monitor", False):
        return "monitor"
    return "time"
