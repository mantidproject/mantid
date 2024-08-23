# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from matplotlib.legend import Legend

from mantid.plots.mantidaxes import MantidAxes
from mantid.plots.utility import row_num, col_num
from mantidqt.widgets.plotconfigdialog import curve_in_ax
from workbench.config import DEFAULT_SCRIPT_CONTENT
from workbench.plotting.plotscriptgenerator.axes import (
    generate_axis_limit_commands,
    generate_axis_label_commands,
    generate_set_title_command,
    generate_axis_scale_commands,
    generate_axis_facecolor_commands,
    generate_tick_commands,
    generate_tick_formatter_commands,
)
from workbench.plotting.plotscriptgenerator.figure import generate_subplots_command
from workbench.plotting.plotscriptgenerator.lines import generate_plot_command
from workbench.plotting.plotscriptgenerator.legend import (
    generate_legend_commands,
    generate_title_font_commands,
    generate_label_font_commands,
    generate_visible_command,
)
from workbench.plotting.plotscriptgenerator.colorfills import generate_plot_2d_command
from workbench.plotting.plotscriptgenerator.utils import generate_workspace_retrieval_commands, sorted_lines_in
from workbench.plotting.plotscriptgenerator.fitting import get_fit_cmds
from mantidqt.plotting.figuretype import FigureType, axes_type

FIG_VARIABLE = "fig"
AXES_VARIABLE = "axes"
LEGEND_VARIABLE = "legend"
if hasattr(Legend, "set_draggable"):
    SET_DRAGGABLE_METHOD = "set_draggable(True)"
else:
    SET_DRAGGABLE_METHOD = "draggable()"
FIT_DOCUMENTATION_STRING = "# Fit definition, see https://docs.mantidproject.org/algorithms/Fit-v1.html for more " "details"
TICKER_FORMATTER_IMPORT = "from matplotlib.ticker import NullFormatter, ScalarFormatter, LogFormatterSciNotation"


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
        axes.set_facecolor()
        <Set axes major tick formatters if non-default>
        axes.legend().set_draggable(True)     (if legend present, or just draggable() for earlier matplotlib versions)
        <Legend title and label commands if non-default>

        fig.show()
        # Use plt.show() if running the script outside of Workbench
        #plt.show()

    :param fig: A matplotlib.pyplot.Figure object you want to create a script from
    :param exclude_headers: Boolean. Set to True to ignore imports/headers
    :return: A String. A script to recreate the given figure
    """
    plot_commands = []
    plot_headers = ["import matplotlib.pyplot as plt", "from mantid.plots.utility import MantidAxType"]

    for ax in fig.get_axes():
        if not isinstance(ax, MantidAxes):
            continue
        ax_object_var = get_axes_object_variable(ax)
        if axes_type(ax) in [FigureType.Image]:
            colormap_lines, colormap_headers = get_plot_2d_cmd(ax, ax_object_var)  # ax.imshow or pcolormesh
            plot_commands.extend(colormap_lines)
            plot_headers.extend(colormap_headers)
        else:
            if not curve_in_ax(ax):
                plot_commands.append(f"{ax_object_var}.axis('off')")
                plot_commands.append("")
                continue
            plot_commands.extend(get_plot_cmds(ax, ax_object_var))  # ax.plot

        plot_commands.extend(get_tick_commands(ax, ax_object_var))
        plot_commands.extend(get_title_cmds(ax, ax_object_var))  # ax.set_title
        plot_commands.extend(get_axis_label_cmds(ax, ax_object_var))  # ax.set_label
        plot_commands.extend(get_axis_limit_cmds(ax, ax_object_var))  # ax.set_lim
        plot_commands.extend(get_axis_scale_cmds(ax, ax_object_var))  # ax.set_scale
        plot_commands.extend(get_axis_facecolor_cmds(ax, ax_object_var))  # ax.set_facecolor

        # Only add the ticker import to headers if it's needed.
        formatter_commands = get_tick_formatter_commands(ax, ax_object_var)
        if len(formatter_commands) > 0:
            plot_commands.extend(formatter_commands)  # ax.{x,y}axis.set_major_formatter
            if TICKER_FORMATTER_IMPORT not in plot_headers:
                plot_headers.append(TICKER_FORMATTER_IMPORT)

        plot_commands.extend(get_legend_cmds(ax, ax_object_var))  # ax.legend
        plot_commands.append("")

    if not plot_commands:
        return

    fit_commands, fit_headers = get_fit_cmds(fig)

    cmds = [] if exclude_headers else [DEFAULT_SCRIPT_CONTENT]
    if exclude_headers and fit_headers:
        cmds.extend(fit_headers)
    if plot_headers:
        cmds.extend(plot_headers)
    if fit_commands:
        cmds.append(FIT_DOCUMENTATION_STRING)
        cmds.extend(fit_commands + [""])
    cmds.extend(generate_workspace_retrieval_commands(fig) + [""])
    cmds.append("{}, {} = {}".format(FIG_VARIABLE, AXES_VARIABLE, generate_subplots_command(fig)))
    cmds.extend(plot_commands)
    cmds.append("fig.show()")
    cmds.append("# Use plt.show() if running the script outside of Workbench")
    cmds.append("#plt.show()")
    cmds.append("# Scripting Plots in Mantid:")
    cmds.append("# https://docs.mantidproject.org/tutorials/python_in_mantid/plotting/02_scripting_plots.html")

    return "\n".join(cmds)


def get_plot_cmds(ax, ax_object_var):
    """Get commands such as axes.plot or axes.errorbar"""
    cmds = []
    for artist in sorted_lines_in(ax, ax.get_tracked_artists()):
        cmds.append("{ax_obj}.{cmd}".format(ax_obj=ax_object_var, cmd=generate_plot_command(artist)))
    return cmds


def get_plot_2d_cmd(ax, ax_object_var):
    """Get commands such as imshow or pcolormesh"""
    cmds = []
    for artist in ax.get_tracked_artists():
        cmds.extend(generate_plot_2d_command(artist, ax_object_var))
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


def get_axis_facecolor_cmds(ax, ax_object_var):
    """Get command ax.set_facecolor. Returns an empty list if the facecolor is default."""
    command = generate_axis_facecolor_commands(ax)
    if command is not None:
        return ["{ax_obj}.{cmd}".format(ax_obj=ax_object_var, cmd=command)]
    return []


def get_title_cmds(ax, ax_object_var):
    """Get command ax.set_title return an empty list if no title set"""
    if ax.get_title():
        return ["{ax_obj}.{cmd}".format(ax_obj=ax_object_var, cmd=generate_set_title_command(ax))]
    return []


def get_legend_cmds(ax, ax_object_var):
    """Get commands for setting legend properties"""
    cmds = []
    if ax.legend_:
        cmds.append(
            "{legend_object} = {ax_obj}.legend({legend_commands}).{draggable_method}.legend".format(
                legend_object=LEGEND_VARIABLE,
                ax_obj=ax_object_var,
                legend_commands=generate_legend_commands(ax.legend_),
                draggable_method=SET_DRAGGABLE_METHOD,
            )
        )
        cmds.extend(generate_title_font_commands(ax.legend_, LEGEND_VARIABLE))
        cmds.extend(generate_label_font_commands(ax.legend_, LEGEND_VARIABLE))
        cmds.extend(generate_visible_command(ax.legend_, LEGEND_VARIABLE))
    return cmds


def get_tick_commands(ax, ax_object_var):
    """Get ax.tick_params commands for setting properties of tick marks and grid lines."""
    tick_commands = generate_tick_commands(ax)
    return ["{ax_obj}.{cmd}".format(ax_obj=ax_object_var, cmd=cmd) for cmd in tick_commands]


def get_tick_formatter_commands(ax, ax_object_var):
    """Get commands for setting the format of the tick labels."""
    commands = generate_tick_formatter_commands(ax)
    return ["{ax_obj}.{cmd}".format(ax_obj=ax_object_var, cmd=cmd) for cmd in commands]


def get_axes_object_variable(ax):
    """Get a string that will return the axes object in the script"""
    # plt.subplots returns an Axes object if there's only one axes being
    # plotted otherwise it returns a list
    ax_object_var = AXES_VARIABLE

    if ax.get_gridspec() and hasattr(ax.get_gridspec(), "nrows") and hasattr(ax.get_gridspec(), "ncols"):
        num_rows = ax.get_gridspec().nrows
        num_cols = ax.get_gridspec().ncols
    else:
        num_rows = ax.numRows
        num_cols = ax.numCols

    if num_rows > 1:
        ax_object_var += "[{row_num}]".format(row_num=row_num(ax))
    if num_cols > 1:
        ax_object_var += "[{col_num}]".format(col_num=col_num(ax))

    return ax_object_var
