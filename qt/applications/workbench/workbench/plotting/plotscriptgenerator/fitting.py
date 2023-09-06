# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import json

BASE_FIT_COMMAND = "Fit"
BASE_FIT_INCLUDE = "from mantid.simpleapi import Fit"

FIT_COMMANDS_ORDER = {"Function": 1, "InputWorkspace": 2, "WorkspaceIndex": 3, "Output": 4, "StartX": 5, "EndX": 6}


def get_fit_cmds(fig):
    if not hasattr(fig.canvas.manager, "fit_browser"):
        return [], []
    cmds = []
    fit_headers = [BASE_FIT_INCLUDE]
    fit_browser = fig.canvas.manager.fit_browser
    if fit_browser.fit_result_ws_name:
        fit_parameters = json.loads(fit_browser.getFitAlgorithmParameters())
        # Fit parameters is a Json object with the following entries
        # {name: 'algorithm', properties: {json object containing all the properties of the alg'}
        cmds.extend(_get_fit_variable_assigment_commands(fit_parameters["properties"]))
        cmds.append(_get_fit_call_command(fit_parameters["properties"]))
        return cmds, fit_headers
    else:
        return [], []


def _get_fit_variable_assignment_command(fit_property, property_value):
    if isinstance(property_value, str):
        return '{}="{}"'.format(fit_property, property_value)
    elif isinstance(property_value, float):
        return "{0}={1:.5f}".format(fit_property, property_value)
    else:
        return "{}={}".format(fit_property, property_value)


def _get_fit_variable_assigment_commands(fit_properties):
    cmds = []
    for fit_property in fit_properties:
        cmds.append(_get_fit_variable_assignment_command(fit_property, fit_properties[fit_property]))
    cmds.sort(key=lambda x: FIT_COMMANDS_ORDER[x.split("=")[0]] if x.split("=")[0] in FIT_COMMANDS_ORDER else 999)
    return cmds


def _get_fit_call_command(fit_properties):
    fit_argument_list = []
    for fit_property in fit_properties:
        fit_argument_list.append("{}={}".format(fit_property, fit_property))
    fit_command = "{}({})".format(BASE_FIT_COMMAND, ",".join(fit_argument_list))
    return fit_command
