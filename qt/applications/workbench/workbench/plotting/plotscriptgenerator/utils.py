# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from numpy import ndarray


def convert_value_to_arg_string(value):
    """
    Converts a given object into a string representation of that object
    which can be passed to a function. It is recursive so works on objects
    such as lists.
    """
    if isinstance(value, str) or isinstance(value, unicode):
        return "'{}'".format(value)
    if isinstance(value, list) or isinstance(value, ndarray):
        return "[{}]".format(', '.join([convert_value_to_arg_string(v) for v in value]))
    if isinstance(value, dict):
        kv_pairs = []
        for key, val in value.items():
            kv_pairs.append("{}: {}".format(convert_value_to_arg_string(key),
                                            convert_value_to_arg_string(val)))
        return "{{{}}}".format(', '.join(kv_pairs))
    return str(value)


def convert_args_to_string(args, kwargs):
    """
    Given list of args and dict of kwargs, constructs a string that
    would be valid code to pass into a Python function
    """
    arg_strings = [str(arg) for arg in args] if args else []
    for kwarg, value in sorted(kwargs.items()):  # sorting makes this testable
        arg_strings.append("{}={}".format(kwarg, convert_value_to_arg_string(value)))
    return ', '.join(arg_strings)
