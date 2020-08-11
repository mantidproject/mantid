# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List

from matplotlib.container import ErrorbarContainer
from qtpy import QtWidgets
from Muon.GUI.Common.plot_widget.plotting_canvas.plot_toolbar import PlotToolbar
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model import WorkspacePlotInformation
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_view_interface import PlottingCanvasViewInterface
from mantid import AnalysisDataService
from mantid.plots import legend_set_draggable
from mantid.plots.plotfunctions import get_plot_fig
from matplotlib.figure import Figure
import matplotlib.pyplot as plt
import numpy as np

from matplotlib.backends.qt_compat import is_pyqt5

if is_pyqt5():
    from matplotlib.backends.backend_qt5agg import FigureCanvas
else:
    from matplotlib.backends.backend_qt4agg import FigureCanvas

DEFAULT_X_LIMITS = [0, 15]


def _do_single_plot(ax, workspace, index, errors, plot_kwargs):
    plot_fn = ax.errorbar if errors else ax.plot
    plot_kwargs['wkspIndex'] = index
    plot_fn(workspace, **plot_kwargs)


class PlottingCanvasView(QtWidgets.QWidget, PlottingCanvasViewInterface):

    def __init__(self, parent=None):
        super().__init__(parent)

        # create the figure
        self.fig = Figure()
        self.fig.canvas = FigureCanvas(self.fig)
        self.toolBar = PlotToolbar(self.fig.canvas, self)

        # Create a set of Mantid axis for the figure
        self.fig, axes = get_plot_fig(overplot=False, ax_properties=None, axes_num=1,
                                      fig=self.fig)
        self._number_of_axes = 1

        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.toolBar)
        layout.addWidget(self.fig.canvas)
        self.setLayout(layout)

        self._plot_information_list = []  # type : List[PlotInformation}

    @property
    def plotted_workspace_information(self):
        return self._plot_information_list

    @property
    def plotted_workspaces_and_indices(self):
        plotted_workspaces = []
        plotted_indices = []
        for plot_info in self._plot_information_list:
            plotted_workspaces.append(plot_info.workspace_name)
            plotted_indices.append(plot_info.index)

        return plotted_workspaces, plotted_indices

    @property
    def num_plotted_workspaces(self):
        return len(self._plot_information_list)

    @property
    def number_of_axes(self):
        return self._number_of_axes

    def add_widget(self, widget):
        self.layout().addWidget(widget)

    def create_new_plot_canvas(self, num_axes):
        """Creates a new blank plotting canvas"""
        self._plot_information_list = []
        self._number_of_axes = num_axes
        self.fig.clf()
        self.fig, axes = get_plot_fig(overplot=False, ax_properties=None, axes_num=num_axes,
                                      fig=self.fig)
        self.fig.tight_layout()
        self.fig.canvas.draw()

    def clear_all_workspaces_from_plot(self):
        """Clears all workspaces from the plot"""
        for ax in self.fig.axes:
            ax.cla()
            ax.tracked_workspaces.clear()
            ax.set_prop_cycle(None)
        self._plot_information_list = []

    def add_workspaces_to_plot(self, workspace_plot_info_list: List[WorkspacePlotInformation]):
        """Add a list of workspaces to the plot - The workspaces are contained in a list PlotInformation
        The PlotInformation contains the workspace name, workspace index and target axis."""
        for workspace_plot_info in workspace_plot_info_list:
            workspace_name = workspace_plot_info.workspace_name
            try:
                workspace = AnalysisDataService.Instance().retrieve(workspace_name)
            except RuntimeError:
                continue
            self._plot_information_list.append(workspace_plot_info)
            errors = workspace_plot_info.errors
            ws_index = workspace_plot_info.index
            axis_number = workspace_plot_info.axis
            ax = self.fig.axes[axis_number]
            _do_single_plot(ax, workspace, ws_index, errors=errors,
                            plot_kwargs=self._get_plot_kwargs(workspace_plot_info))

    def remove_workspace_info_from_plot(self, workspace_plot_info_list: List[WorkspacePlotInformation]):
        for workspace_plot_info in workspace_plot_info_list:
            workspace_name = workspace_plot_info.workspace_name
            try:
                workspace = AnalysisDataService.Instance().retrieve(workspace_name)
            except RuntimeError:
                continue
            for plotted_information in self._plot_information_list.copy():
                if workspace_plot_info.workspace_name == plotted_information.workspace_name and \
                        workspace_plot_info.axis == plotted_information.axis:
                    axis = self.fig.axes[workspace_plot_info.axis]
                    axis.remove_workspace_artists(workspace)
                    self._plot_information_list.remove(plotted_information)

        # If we have no plotted lines, reset the color cycle
        if self.num_plotted_workspaces == 0:
            self._reset_color_cycle()

    def remove_workspace_from_plot(self, workspace):
        """Remove all references to a workspaces from the plot """
        for workspace_plot_info in self._plot_information_list.copy():
            workspace_name = workspace_plot_info.workspace_name
            if workspace_name == workspace.name():
                axis = self.fig.axes[workspace_plot_info.axis]
                axis.remove_workspace_artists(workspace)
                self._plot_information_list.remove(workspace_plot_info)

    # Ads observer functions
    def replace_specified_workspace_in_plot(self, workspace):
        """Replace specified workspace in the plot with a new and presumably updated instance"""
        for workspace_plot_info in self._plot_information_list:
            plotted_workspace_name = workspace_plot_info.workspace_name
            workspace_name = workspace.name()
            if workspace_name == plotted_workspace_name:
                axis = self.fig.axes[workspace_plot_info.axis]
                axis.replace_workspace_artists(workspace)
        self.redraw_figure()

    def replot_workspace_with_error_state(self, workspace_name, with_errors: bool):
        for plot_info in self.plotted_workspace_information:
            if plot_info.workspace_name == workspace_name:
                axis = self.fig.axes[plot_info.axis]
                workspace_name = plot_info.workspace_name
                artist_info = axis.tracked_workspaces[workspace_name]
                for ws_artist in artist_info:
                    for artist in ws_artist._artists:
                        if isinstance(artist, ErrorbarContainer):
                            color = artist[0].get_color()
                        else:
                            color = artist.get_color()
                        plot_kwargs = self._get_plot_kwargs(plot_info)
                        plot_kwargs["color"] = color
                        axis.replot_artist(artist, with_errors, **plot_kwargs)
        self.redraw_figure()

    def set_axis_xlimits(self, axis_number, xlims):
        ax = self.fig.axes[axis_number]
        ax.set_xlim(xlims[0], xlims[1])

    def set_axis_ylimits(self, axis_number, ylims):
        ax = self.fig.axes[axis_number]
        ax.set_ylim(ylims[0], ylims[1])

    def set_axes_limits(self, xlim, ylim):
        plt.setp(self.fig.axes, xlim=xlim, ylim=ylim)

    def autoscale_y_axes(self):
        ymin = 1e9
        ymax = -1e9
        for axis in self.fig.axes:
            ymin_i, ymax_i = self._get_y_axis_autoscale_limts(axis)
            if ymin_i < ymin:
                ymin = ymin_i
            if ymax_i > ymax:
                ymax = ymax_i

        plt.setp(self.fig.axes, ylim=[ymin, ymax])

    def autoscale_selected_y_axis(self, axis_number):
        if axis_number >= len(self.fig.axes):
            return
        axis = self.fig.axes[axis_number]
        bottom, top, = self._get_y_axis_autoscale_limts(axis)
        axis.set_ylim(bottom, top)

    def set_title(self, axis_number, title):
        if axis_number >= self.number_of_axes:
            return
        axis = self.fig.axes[axis_number]
        axis.set_title(title)

    def get_axis_limits(self, axis_number):
        xmin, xmax = self.fig.axes[axis_number].get_xlim()
        ymin, ymax = self.fig.axes[axis_number].get_ylim()

        return xmin, xmax, ymin, ymax

    def redraw_figure(self):
        self.fig.canvas.toolbar.update()
        self._redraw_legend()
        self.fig.tight_layout()
        self.fig.canvas.draw()

    def _redraw_legend(self):
        for ax in self.fig.axes:
            if ax.get_legend_handles_labels()[0]:
                legend = ax.legend(prop=dict(size=5))
                legend_set_draggable(legend, True)

    def _get_plot_kwargs(self, workspace_info: WorkspacePlotInformation):
        label = workspace_info.label
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': label}
        return plot_kwargs

    @staticmethod
    def _get_y_axis_autoscale_limts(axis):
        bottom = 1e9
        top = -1e9
        ylim = np.inf, -np.inf
        xmin, xmax = axis.get_xlim()
        for line in axis.lines:
            x, y = line.get_data()
            start, stop = np.searchsorted(x, tuple([xmin, xmax]))
            y_within_range = y[max(start - 1, 0):(stop + 1)]
            ylim = min(ylim[0], np.nanmin(y_within_range)), max(ylim[1], np.nanmax(y_within_range))
            bottom_i = ylim[0] * 1.3 if ylim[0] < 0.0 else ylim[0] * 0.7
            top_i = ylim[1] * 1.3 if ylim[1] > 0.0 else ylim[1] * 0.7
            if bottom_i < bottom:
                bottom = bottom_i
            if top_i > top:
                top = top_i
        return bottom, top

    def _reset_color_cycle(self):
        for ax in self.fig.axes:
            ax.cla()
            ax.tracked_workspaces.clear()
            ax.set_prop_cycle(None)

    def resizeEvent(self, event):
        self.fig.tight_layout()
