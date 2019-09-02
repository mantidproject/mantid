# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from workbench.plotting.plotscriptgenerator.utils import convert_value_to_arg_string

BASE_AXIS_LABEL_COMMAND = "set_{}label('{}')"
BASE_AXIS_SCALE_COMMAND = "set_{}lim({})"


def generate_axis_limit_commands(ax):
    """Generate commands to set the axes' limits"""
    commands = []
    for axis in ['x', 'y']:
        lims = getattr(ax, "get_{}lim".format(axis))()
        commands.append(BASE_AXIS_SCALE_COMMAND.format(axis, convert_value_to_arg_string(lims)))
    return commands


def generate_axis_label_commands(ax):
    commands = []
    for axis in ['x', 'y']:
        label = getattr(ax, 'get_{}label'.format(axis))()
        if label:
            commands.append(BASE_AXIS_LABEL_COMMAND.format(axis, label))
    return commands
