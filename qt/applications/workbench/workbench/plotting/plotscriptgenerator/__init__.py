# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantid.plots import MantidAxes

from workbench.plugins.editor import DEFAULT_CONTENT
from workbench.plotting.plotscriptgenerator.axes import generate_add_subplot_command
from workbench.plotting.plotscriptgenerator.figure import generate_figure_command
from workbench.plotting.plotscriptgenerator.lines import generate_plot_command
from workbench.plotting.plotscriptgenerator.utils import get_workspace_history_commands

FIG_VARIABLE = "fig"
AXES_VARIABLE = "ax"


def generate_script(fig):
    """
    Generate a script to recreate a figure.

    This currently only supports recreating artists that were plotted
    from a Workspace. The format of the outputted script is as follows:

        <Default Workbench script contents (imports)>
        <Workspace history to recreate Workspaces>
        fig = plt.figure()
        ax = fig.add_subplot()
        ax.plot() or ax.errorbar()
        ax.legend().draggable()     (if legend present)
        plt.show()

    :param fig: A matplotlib.pyplot.Figure object you want to create a script from
    :return: A String. A script to recreate the given figure
    """
    plot_commands = []
    for ax in fig.get_axes():
        if not isinstance(ax, MantidAxes):
            continue
        plot_commands.append("{} = {}.{}"
                             "".format(AXES_VARIABLE, FIG_VARIABLE,
                                       generate_add_subplot_command(ax)))
        for artist in ax.get_tracked_artists():
            plot_commands.append("{}.{}".format(AXES_VARIABLE, generate_plot_command(artist)))
        if ax.legend_:
            plot_commands.append("{}.legend().draggable()".format(AXES_VARIABLE))
    if not plot_commands:
        return
    cmds = [DEFAULT_CONTENT] + get_workspace_history_commands(fig)
    cmds.append("{} = {}".format(FIG_VARIABLE, generate_figure_command(fig)))
    cmds += plot_commands
    cmds.append("plt.show()")
    return '\n'.join(cmds)
