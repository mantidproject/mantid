# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from numpy import ndarray

BASE_CREATE_FIG_COMMAND = "plt.figure({})"


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
        arg_string = "{"
        for key, val in value.items():
            arg_string += "'{}': {}, ".format(key, convert_value_to_arg_string(val))
        return arg_string[:-2] + "}"
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


class PlotScriptGenerator:

    def __init__(self):
        pass

    @staticmethod
    def get_figure_command_kwargs(fig):
        kwargs = {
            'figsize': (fig.get_figwidth(), fig.get_figheight()),
            'dpi': fig.dpi
        }
        return kwargs

    @staticmethod
    def generate_figure_command(fig):
        """Generate command to create figure"""
        kwargs = PlotScriptGenerator.get_figure_command_kwargs(fig)
        return BASE_CREATE_FIG_COMMAND.format(convert_args_to_string(None, kwargs))
