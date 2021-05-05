# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from numpy import isclose

from matplotlib.ticker import NullLocator

from mantid.plots.utility import get_autoscale_limits
from workbench.plotting.plotscriptgenerator.utils import convert_value_to_arg_string

BASE_AXIS_LABEL_COMMAND = "set_{}label({})"
BASE_AXIS_LIM_COMMAND = "set_{}lim({})"
BASE_SET_TITLE_COMMAND = "set_title({})"
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
            commands.append(BASE_AXIS_LABEL_COMMAND.format(axis, repr(label)))
    return commands


def generate_set_title_command(ax):
    return BASE_SET_TITLE_COMMAND.format(repr(ax.get_title()))


def generate_axis_scale_commands(ax):
    commands = []
    for axis in ['x', 'y']:
        scale = getattr(ax, 'get_{}scale'.format(axis))()
        if scale != 'linear':
            commands.append(BASE_AXIS_SCALE_COMMAND.format(axis, scale))
    return commands


def generate_tick_params_kwargs(axis, tick_type="major"):
    return getattr(axis, f"_{tick_type}_tick_kw")


def generate_tick_commands(ax):
    commands = []

    for tick_type in ["minor", "major"]:
        if not isinstance(getattr(ax.xaxis, tick_type).locator, NullLocator):
            if tick_type == "minor":
                commands.append("axes.minorticks_on()")

            if isinstance(getattr(ax.xaxis, f"{tick_type}Ticks"), list) and \
                    len(getattr(ax.xaxis, f"{tick_type}Ticks")) > 0:
                commands.append(f"axes.tick_params(axis='x', which='{tick_type}', **"
                                f"{generate_tick_params_kwargs(ax.xaxis, tick_type)})")
                commands.append(f"axes.tick_params(axis='y', which='{tick_type}', **"
                                f"{generate_tick_params_kwargs(ax.yaxis, tick_type)})")

    return commands


def generate_grid_commands(ax):
    axes_grid_params = list()
    axes = ['xaxis', 'yaxis']
    for axis_name in axes:
        axis = getattr(ax, axis_name, None)
        if axis:
            name = getattr(axis, "axis_name", None)  # x or y
            is_gridOnMinor = getattr(axis, "_gridOnMinor", False)
            is_gridOnMajor = getattr(axis, "_gridOnMajor", False)
            if is_gridOnMinor and is_gridOnMajor:
                which = 'both'
            elif is_gridOnMinor:
                which = 'minor'
            elif is_gridOnMajor:
                which = 'major'
            else:
                continue  # in this case no grid lines for this axis, this is default so no point making a command
            axes_grid_params.append((name, which))
    commands = []
    if len(axes_grid_params) == 2:  # check 'which' equality - this lets us use only one command for both axes
        if axes_grid_params[0][1] == axes_grid_params[1][1]:
            commands.append(f"axes.grid(b=True, axis='both', which='{axes_grid_params[0][1]}')")
            return commands
    for params in axes_grid_params:  # in the very rare event that only one axis has a grid or the options differ
        commands.append(f"axes.grid(b=True, axis='{params[0]}', which='{params[1]}')")
    return commands
