# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantid.plots.utility import get_errorbar_containers
from workbench.plotting.plotscriptgenerator.axes import generate_add_subplot_command
from workbench.plotting.plotscriptgenerator.figure import generate_figure_command
from workbench.plotting.plotscriptgenerator.lines import generate_plot_command


def generate_script(fig):
    fig_var = "fig"
    ax_var = "ax"
    cmds = list()
    cmds.append("{} = {}".format(fig_var, generate_figure_command(fig)))
    for ax in fig.get_axes():
        cmds.append("{} = {}.{}"
                    "".format(ax_var, fig_var,
                              generate_add_subplot_command(ax)))
        for artist in ax.lines + get_errorbar_containers(ax):
            if artist in ax.get_tracked_artists():
                cmds.append("{}.{}".format(
                    ax_var, generate_plot_command(artist)))
    cmds.append("plt.show()")
    return '\n'.join(cmds)
