# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantid.plots.utility import get_axes_index
from workbench.plotting.plotscriptgenerator.utils import convert_args_to_string, convert_value_to_arg_string

BASE_CREATE_AX_COMMAND = "add_subplot({})"
ADD_SUBPLOT_KWARGS = [  # kwargs passed to the "add_subplot" command
    'frame_on', 'label', 'title', 'visible', 'xlabel', 'xscale', 'ylabel', 'yscale']


def get_add_subplot_pos_args(ax):
    """Get list of positional args to recreate an axes"""
    return [ax.numRows, ax.numCols, get_axes_index(ax)]


def get_add_subplot_kwargs(ax):
    """Get kwargs for recreating an axes"""
    props = {key: ax.properties()[key] for key in ADD_SUBPLOT_KWARGS}
    props['projection'] = 'mantid'
    props['sharex'] = True if ax.get_shared_x_axes()._mapping else None
    props['sharey'] = True if ax.get_shared_y_axes()._mapping else None
    return props


def generate_add_subplot_command(ax):
    """Generate command to create an axes"""
    command = BASE_CREATE_AX_COMMAND.format(
        convert_args_to_string(get_add_subplot_pos_args(ax),
                               get_add_subplot_kwargs(ax)))
    return command


def generate_axis_limit_commands(ax):
    """Generate commands to set the axes' limits"""
    commands = []
    for axis in ['x', 'y']:
        lims = getattr(ax, "get_{}lim".format(axis))()
        commands.append("set_{}lim({})".format(axis, convert_value_to_arg_string(lims)))
    return commands
