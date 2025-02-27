# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List

from mantidqtinterfaces.Muon.GUI.Common.plot_widget.external_plotting.external_plotting_model import PlotInformation


class ExternalPlottingView(object):
    def __init__(self):
        self.number_of_axes = 0

    def create_external_plot_window(self, internal_axes):
        """
        Handles the external_plot requests
        :param internal_axes, The internal axes that will be replicated in the new figure
        :return A new figure window:
        """
        self.number_of_axes = len(internal_axes)
        return self._create_external_workbench_fig_window()

    def copy_axes_setup(self, fig_window, internal_axes):
        """
        Sets the axis setup of the new figure window to match the internal axis
        e.g axis limits and title
        :param fig_window, The new figure window
        :param internal_axes, The internal axes that will be replicated in the new figure
        """
        self._copy_axes_setup_workbench(fig_window, internal_axes)

    def plot_data(self, fig_window, data: List[PlotInformation]):
        """
        Handles the plotting of the input data into the new fig window
        :param fig_window, The new figure window
        :param data, The data to be plotted in the new figure window
        """
        self._plot_data_workbench(fig_window, data)

    def show(self, fig_window):
        """
        Raises the new figure window
        :param fig_window, The new figure window
        """
        fig_window.show()

    # private workbench methods
    def _plot_data_workbench(self, fig_window, data):
        external_axes = fig_window.axes
        for plot_info in data:
            external_axis = external_axes[plot_info.axis]
            external_axis.plot(
                plot_info.workspace, specNum=plot_info.specNum, autoscale_on_update=True, distribution=not plot_info.normalised
            )
            external_axis.make_legend()
        fig_window.show()

    def _create_external_workbench_fig_window(self):
        from mantid.plots.plotfunctions import get_plot_fig

        external_fig, _ = get_plot_fig(axes_num=self.number_of_axes)
        return external_fig

    def _copy_axes_setup_workbench(self, fig_window, internal_axes):
        for internal_axis, external_axis in zip(internal_axes, fig_window.axes):
            xlim = internal_axis.get_xlim()
            ylim = internal_axis.get_ylim()
            external_axis.set_xlim(xlim[0], xlim[1])
            external_axis.set_ylim(ylim[0], ylim[1])

            external_axis.set_xticks(internal_axis.get_xticks())
            external_axis.set_xticklabels(internal_axis.get_xticklabels())

            external_axis.set_yticks(internal_axis.get_yticks())
            external_axis.set_yticklabels(internal_axis.get_yticklabels())
