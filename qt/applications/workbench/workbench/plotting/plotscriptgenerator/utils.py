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

from mantid.api import AlgorithmManager, AnalysisDataService as ads
from mantid.kernel import UsageService
from workbench.projectrecovery.projectrecoverysaver import ALGS_TO_IGNORE, ALG_PROPERTIES_TO_IGNORE


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


def get_plotted_workspaces_names(fig):
    plotted_workspaces = []
    for ax in fig.get_axes():
        try:
            plotted_workspaces += list(ax.tracked_workspaces.keys())
        except AttributeError:  # Scripted plots have no tracked workspaces
            pass
    return plotted_workspaces


def get_workspace_history_list(workspace):
    """Return a list of commands that will recover the state of a workspace."""
    alg_name = "GeneratePythonScript"
    alg = AlgorithmManager.createUnmanaged(alg_name, 1)
    alg.setChild(True)
    alg.setLogging(False)
    alg.initialize()
    alg.setProperty("InputWorkspace", workspace)
    alg.setProperty("IgnoreTheseAlgs", ALGS_TO_IGNORE)
    alg.setProperty("IgnoreTheseAlgProperties", ALG_PROPERTIES_TO_IGNORE)
    alg.setPropertyValue("StartTimestamp", UsageService.getStartTime().toISO8601String())
    alg.execute()
    history = alg.getPropertyValue("ScriptText")
    return history.split('\n')[5:]  # trim the header and import


def get_workspace_history_commands(fig):
    """
    Get a list of commands that will recover the state of all the
    Workspaces tracked by a figure. If the final command in a Workspace's
    history is a Load command then we only append that command.
    """
    plotted_workspaces = get_plotted_workspaces_names(fig)
    history_commands = []
    for ws_name in plotted_workspaces:
        try:
            workspace = ads.retrieve(ws_name)
            ws_var_name = clean_variable_name(ws_name)
            ws_history = get_workspace_history_list(workspace)
            if ws_history[-1].startswith('Load('):
                ws_history = [ws_history[-1]]
            history_commands += ['{} = {}'.format(ws_var_name, cmd) for cmd in ws_history]
            history_commands.append('')  # Blank line to separate each workspace's history
        except KeyError:  # Raised if workspace is not in ADS
            pass
    return history_commands


def clean_variable_name(name):
    """Converts a string into a valid Python variable name"""
    return re.sub('\W|^(?=\d)', '_', name)
