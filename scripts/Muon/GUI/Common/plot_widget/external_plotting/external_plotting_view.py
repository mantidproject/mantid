# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List
from math import sqrt, ceil, floor

from distutils.version import LooseVersion

from qtpy import QT_VERSION, QtGui
from Muon.GUI.Common.plot_widget.external_plotting.external_plotting_model import PlotInformation
from mantid.plots.utility import legend_set_draggable


class ExternalPlottingView(object):

    def __init__(self):
        self.number_of_axes = 0

    def create_external_plot_window(self, internal_axes):
        """
        Handles the external_plot request and delegates appropriately to either the Workbench
        or Mantidplot plotting functions to create a plot window.
        :param internal_axes, The internal axes that will be replicated in the new figure
        :return A new figure window:
        """
        self.number_of_axes = len(internal_axes)
        if QT_VERSION < LooseVersion("5"):
            external_fig_window = self._create_external_mantidplot_fig_window()

        else:
            external_fig_window = self._create_external_workbench_fig_window()

        return external_fig_window

    def copy_axes_setup(self, fig_window, internal_axes):
        """
        Sets the axis setup of the new figure window to match the internal axis
        e.g axis limits and title
        :param fig_window, The new figure window
        :param internal_axes, The internal axes that will be replicated in the new figure
        """
        if QT_VERSION < LooseVersion("5"):
            self._copy_axes_setup_mantidplot(fig_window, internal_axes)
        else:
            self._copy_axes_setup_workbench(fig_window, internal_axes)

    def plot_data(self, fig_window, data: List[PlotInformation]):
        """
        Handles the plotting of the input data into the new fig window
        :param fig_window, The new figure window
        :param data, The data to be plotted in the new figure window
        """
        if QT_VERSION < LooseVersion("5"):
            self._plot_data_mantidplot(fig_window, data)
        else:
            self._plot_data_workbench(fig_window, data)

    def show(self, fig_window):
        """
        Raises the new figure window, for Mantidplot this function call does nothing
        :param fig_window, The new figure window
        """
        if QT_VERSION < LooseVersion("5"):
            pass  # do nothing
        else:
            fig_window.show()

    # private workbench and MantidPlot methods
    # private workbench methods
    def _plot_data_workbench(self, fig_window, data):
        external_axes = fig_window.axes
        for plot_info in data:
            external_axis = external_axes[plot_info.axis]
            external_axis.plot(plot_info.workspace, specNum=plot_info.specNum, autoscale_on_update=True,
                               distribution=not plot_info.normalised)
            legend_set_draggable(external_axis.legend(), True)
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

    # private Mantidplot methods
    def _plot_data_mantidplot(self, fig_window, data):
        from mantidplot import plotSpectrum
        for i, plot_info in enumerate(data):
            distr_state = self._get_distr_state_mantid_plot(plot_info.normalised)
            if self.number_of_axes == 1:
                plotSpectrum(plot_info.workspace, plot_info.specNum - 1,
                             distribution=distr_state, window=fig_window)
            else:
                lay = fig_window.layer(plot_info.axis + 1)
                fig_window.setActiveLayer(lay)
                plotSpectrum(plot_info.workspace, plot_info.specNum - 1,
                             distribution=distr_state, window=fig_window, type=0)
        if self.number_of_axes != 1:
            fig_window.arrangeLayers(False, False)

    def _create_external_mantidplot_fig_window(self):
        from mantidplot import newGraph
        if self.number_of_axes == 1:
            graph_window = newGraph(layers=1)
        else:
            graph_window = self._create_tiled_external_mantidplot_fig_window()

        return graph_window

    def _create_tiled_external_mantidplot_fig_window(self):
        from mantidplot import newGraph
        ncols = ceil(sqrt(self.number_of_axes))
        nrows = ceil(self.number_of_axes / ncols)
        graph_window = newGraph()
        graph_window.setCols(ncols)
        graph_window.setRows(nrows)
        graph_window.setLayerCanvasSize(100, 100)
        graph_window.setNumLayers(self.number_of_axes)

        return graph_window

    def _get_row_and_col_number(self, plot_number):
        ncols = ceil(sqrt(self.number_of_axes))
        row = floor(plot_number / ncols)
        col = plot_number - row * (ncols)
        return row, col

    def _copy_axes_setup_mantidplot(self, fig_window, internal_axes):
        if self.number_of_axes == 1:
            # remove old legend as we are going to make a new one
            fig_window.activeLayer().removeLegend()
            self._set_mantid_plot_axis_display(fig_window, internal_axes[0])
            return
        for i, internal_axis in enumerate(internal_axes):  # else tiled plot
            layer = fig_window.layer(i + 1)
            if i == 0:
                layer.removeLegend()
            fig_window.setActiveLayer(layer)
            self._set_mantid_plot_axis_display(fig_window, internal_axis)
        fig_window.arrangeLayers(False, False)

    @staticmethod
    def _set_mantid_plot_axis_display(window, internal_axis):
        from mantidplot import Layer
        xlim = internal_axis.get_xlim()
        title = internal_axis.get_title()
        ylim = internal_axis.get_ylim()
        active_layer = window.activeLayer()
        active_layer.setAxisTitleFont(0, QtGui.QFont("normal", 8))
        active_layer.setAxisTitleFont(2, QtGui.QFont("normal", 8))
        active_layer.setTitleFont(QtGui.QFont("normal", 8))
        active_layer.setAutoScale()
        active_layer.setAxisScale(Layer.Bottom, xlim[0], xlim[1])
        active_layer.setAxisScale(Layer.Left, ylim[0], ylim[1])
        active_layer.setTitle(title)
        active_layer.newLegend("")

    @staticmethod
    def _get_distr_state_mantid_plot(is_normalised):
        from mantidplot import DistrFlag
        if is_normalised:
            return DistrFlag.DistrTrue
        else:
            return DistrFlag.DistrFalse
