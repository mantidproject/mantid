# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import re
from numpy import ndarray

from mantid.py3compat import is_text_string


def convert_value_to_arg_string(value):
    """
    Converts a given object into a string representation of that object
    which can be passed to a function. It is recursive so works on objects
    such as lists.
    """
    if is_text_string(value):
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


def get_plotted_workspaces_names(fig):
    plotted_workspaces = []
    for ax in fig.get_axes():
        try:
            plotted_workspaces += list(ax.tracked_workspaces.keys())
        except AttributeError:  # Scripted plots have no tracked workspaces
            pass
    return plotted_workspaces


def generate_workspace_retrieval_commands(fig):
    workspace_names = get_plotted_workspaces_names(fig)
    commands = ["from mantid.api import AnalysisDataService as ADS\n"]
    for name in workspace_names:
        variable_name = clean_variable_name(name)
        commands.append("{} = ADS.retrieve('{}')".format(variable_name, name))
    return commands


def clean_variable_name(name):
    """Converts a string into a valid Python variable name"""
    return re.sub('\W|^(?=\d)', '_', name)
