# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from numpy import isclose

from mantid.plots.utility import get_autoscale_limits
from workbench.plotting.plotscriptgenerator.utils import convert_value_to_arg_string

BASE_AXIS_LABEL_COMMAND = "set_{}label('{}')"
BASE_AXIS_LIM_COMMAND = "set_{}lim({})"
BASE_SET_TITLE_COMMAND = "set_title('{}')"
BASE_AXIS_SCALE_COMMAND = "set_{}scale('{}')"


def generate_axis_limit_commands(ax):
    """Generate commands to set the axes' limits"""
    commands = []
    for axis in ['x', 'y']:
        current_lims = getattr(ax, "get_{}lim".format(axis))()
        default_lims = get_autoscale_limits(ax, axis)
        if not isclose(current_lims, default_lims, rtol=0.01).all():
            arg_string = convert_value_to_arg_string(current_lims)
            commands.append(BASE_AXIS_LIM_COMMAND.format(axis, arg_string))
    return commands


def generate_axis_label_commands(ax):
    commands = []
    for axis in ['x', 'y']:
        label = getattr(ax, 'get_{}label'.format(axis))()
        if label:
            commands.append(BASE_AXIS_LABEL_COMMAND.format(axis, label))
    return commands


def generate_set_title_command(ax):
    return BASE_SET_TITLE_COMMAND.format(ax.get_title())


def generate_axis_scale_commands(ax):
    commands = []
    for axis in ['x', 'y']:
        scale = getattr(ax, 'get_{}scale'.format(axis))()
        if scale != 'linear':
            commands.append(BASE_AXIS_SCALE_COMMAND.format(axis, scale))
    return commands
