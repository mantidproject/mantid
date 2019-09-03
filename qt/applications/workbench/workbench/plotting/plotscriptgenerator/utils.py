# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import re

import numpy as np
from matplotlib.container import ErrorbarContainer

from mantid.py3compat import is_text_string


def round_to_sig_figs(number, sig_figs):
    if np.isclose(number, 0):
        return 0.0
    return round(number, -int(np.floor(np.log10(np.abs(number)))) + (sig_figs - 1))


def convert_value_to_arg_string(value):
    """
    Converts a given object into a string representation of that object
    which can be passed to a function. It is recursive so works on objects
    such as lists.
    """
    if is_text_string(value):
        return "'{}'".format(value)
    if isinstance(value, (list, np.ndarray, tuple)):
        return "[{}]".format(', '.join([convert_value_to_arg_string(v) for v in value]))
    if isinstance(value, dict):
        kv_pairs = []
        for key, val in value.items():
            kv_pairs.append("{}: {}".format(convert_value_to_arg_string(key),
                                            convert_value_to_arg_string(val)))
        return "{{{}}}".format(', '.join(kv_pairs))
    if isinstance(value, (float, np.float)):
        return str(round_to_sig_figs(value, 5))
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
    for name in set(workspace_names):
        variable_name = clean_variable_name(name)
        commands.append("{} = ADS.retrieve('{}')".format(variable_name, name))
    return commands


def clean_variable_name(name):
    """Converts a string into a valid Python variable name"""
    return re.sub('\W|^(?=\d)', '_', name)


def sorted_lines_in(ax, artists):
    lines = ax.get_lines()
    err_containers = [cont for cont in ax.containers
                      if isinstance(cont, ErrorbarContainer)]
    sorted_lines = []
    for line in lines + err_containers:
        if line in artists:
            sorted_lines.append(line)
    return sorted_lines
