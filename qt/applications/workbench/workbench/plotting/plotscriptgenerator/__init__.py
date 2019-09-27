# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantid.plots import MantidAxes

from mantidqt.widgets.plotconfigdialog import curve_in_ax
from workbench.plugins.editor import DEFAULT_CONTENT
from workbench.plotting.plotscriptgenerator.axes import (generate_axis_limit_commands,
                                                         generate_axis_label_commands,
                                                         generate_set_title_command,
                                                         generate_axis_scale_commands)
from workbench.plotting.plotscriptgenerator.figure import generate_subplots_command
from workbench.plotting.plotscriptgenerator.lines import generate_plot_command
from workbench.plotting.plotscriptgenerator.utils import generate_workspace_retrieval_commands, sorted_lines_in

FIG_VARIABLE = "fig"
AXES_VARIABLE = "axes"


def generate_script(fig, exclude_headers=False):
    """
    Generate a script to recreate a figure.

    This currently only supports recreating artists that were plotted
    from a Workspace. The format of the outputted script is as follows:

        <Default Workbench script contents (imports)>
        <Workspace retrieval from ADS>
        fig, axes = plt.subplots()
        axes.plot() or axes.errorbar()
        axes.set_title()
        axes.set_xlabel and axes.set_ylabel()
        axes.set_xlim() and axes.set_ylim()
        axes.set_xscale() and axes.set_yscale()
        axes.legend().draggable()     (if legend present)
        plt.show()

    :param fig: A matplotlib.pyplot.Figure object you want to create a script from
    :param exclude_headers: Boolean. Set to True to ignore imports/headers
    :return: A String. A script to recreate the given figure
    """
    plot_commands = []
    for ax in fig.get_axes():
        if not isinstance(ax, MantidAxes) or not curve_in_ax(ax):
            continue
        ax_object_var = get_axes_object_variable(ax)
        plot_commands.extend(get_plot_cmds(ax, ax_object_var))  # ax.plot
        plot_commands.extend(get_title_cmds(ax, ax_object_var))  # ax.set_title
        plot_commands.extend(get_axis_label_cmds(ax, ax_object_var))  # ax.set_label
        plot_commands.extend(get_axis_limit_cmds(ax, ax_object_var))  # ax.set_lim
        plot_commands.extend(get_axis_scale_cmds(ax, ax_object_var))  # ax.set_scale
        plot_commands.extend(get_legend_cmds(ax, ax_object_var))  # ax.legend
        plot_commands.append('')

    if not plot_commands:
        return

    cmds = [] if exclude_headers else [DEFAULT_CONTENT]
    cmds.extend(generate_workspace_retrieval_commands(fig) + [''])
    cmds.append("{}, {} = {}".format(FIG_VARIABLE, AXES_VARIABLE, generate_subplots_command(fig)))
    cmds.extend(plot_commands)
    cmds.append("plt.show()")
    return '\n'.join(cmds)


def get_plot_cmds(ax, ax_object_var):
    """Get commands such as axes.plot or axes.errorbar"""
    cmds = []
    for artist in sorted_lines_in(ax, ax.get_tracked_artists()):
        cmds.append("{ax_obj}.{cmd}".format(ax_obj=ax_object_var,
                                            cmd=generate_plot_command(artist)))
    return cmds


def get_axis_limit_cmds(ax, ax_object_var):
    """Get commands such as axes.set_xlim and axes.set_ylim"""
    axis_limit_cmds = generate_axis_limit_commands(ax)
    return ["{ax_obj}.{cmd}".format(ax_obj=ax_object_var, cmd=cmd) for cmd in axis_limit_cmds]


def get_axis_label_cmds(ax, ax_object_var):
    """Get commands such as axes.set_xlabel and axes.set_ylabel"""
    axis_label_cmds = generate_axis_label_commands(ax)
    return ["{ax_obj}.{cmd}".format(ax_obj=ax_object_var, cmd=cmd) for cmd in axis_label_cmds]


def get_axis_scale_cmds(ax, ax_object_var):
    """Get commands such as axes.set_xscale and axes.set_yscale"""
    axis_scale_cmds = generate_axis_scale_commands(ax)
    return ["{ax_obj}.{cmd}".format(ax_obj=ax_object_var, cmd=cmd) for cmd in axis_scale_cmds]


def get_title_cmds(ax, ax_object_var):
    """Get command ax.set_title return an empty list if no title set"""
    if ax.get_title():
        return ["{ax_obj}.{cmd}".format(ax_obj=ax_object_var, cmd=generate_set_title_command(ax))]
    return []


def get_legend_cmds(ax, ax_object_var):
    """Get command axes.set_legend"""
    if ax.legend_:
        return ["{ax_obj}.legend().draggable()".format(ax_obj=ax_object_var)]
    return []


def get_axes_object_variable(ax):
    """Get a string that will return the axeses object in the script"""
    # plt.subplots returns an Axes object if there's only one axes being
    # plotted otherwise it returns a list
    ax_object_var = AXES_VARIABLE
    if ax.numRows > 1:
        ax_object_var += "[{row_num}]".format(row_num=ax.rowNum)
    if ax.numCols > 1:
        ax_object_var += "[{col_num}]".format(col_num=ax.colNum)
    return ax_object_var
