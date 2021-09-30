# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List

from matplotlib.container import ErrorbarContainer
from qtpy import QtWidgets, QtCore
from Muon.GUI.Common.plot_widget.plotting_canvas.plot_toolbar import PlotToolbar
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model import WorkspacePlotInformation
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_view_interface import PlottingCanvasViewInterface
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_utils import (_do_single_plot,
                                                                               get_y_min_max_between_x_range,
                                                                               get_num_row_and_col,
                                                                               convert_index_to_row_and_col)
from Muon.GUI.Common.plot_widget.plotting_canvas.plot_color_queue import ColorQueue
from mantid import AnalysisDataService
from mantid.plots import legend_set_draggable
from mantid.plots.plotfunctions import get_plot_fig
from matplotlib.figure import Figure
import matplotlib.pyplot as plt
import numpy as np
from textwrap import wrap

from mantidqt.MPLwidgets import FigureCanvas

# Default color cycle using Matplotlib color codes C0, C1...ect
NUMBER_OF_COLOURS = 10
DEFAULT_COLOR_CYCLE = ["C" + str(index) for index in range(NUMBER_OF_COLOURS)]


class PlottingCanvasView(QtWidgets.QWidget, PlottingCanvasViewInterface):

    def __init__(self, quick_edit, settings, parent=None):
        super().__init__(parent)
        # later we will allow these to be changed in the settings
        self._settings = settings
        self._min_y_range = settings.min_y_range
        self._y_axis_margin = settings.y_axis_margin
        self._x_tick_labels = None
        self._y_tick_labels = None
        # create the figure
        self.fig = Figure()
        self.fig.canvas = FigureCanvas(self.fig)
        self.fig.canvas.setMinimumHeight(500)
        self.toolBar = PlotToolbar(self.fig.canvas, self)

        # Create a set of Mantid axis for the figure
        self.fig, axes = get_plot_fig(overplot=False, ax_properties=None, axes_num=1,
                                      fig=self.fig)
        self._number_of_axes = 1
        self._color_queue = [ColorQueue(DEFAULT_COLOR_CYCLE)]

        # Add a splitter for the plotting canvas and quick edit toolbar
        splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)
        splitter.addWidget(self.fig.canvas)
        self._quick_edit = quick_edit
        splitter.addWidget(self._quick_edit)
        splitter.setChildrenCollapsible(False)

        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.toolBar)
        layout.addWidget(splitter)
        self.setLayout(layout)

        self._plot_information_list = []  # type : List[PlotInformation}

    @property
    def autoscale_state(self):
        return self._quick_edit.autoscale_state

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

    def set_x_ticks(self, x_ticks=None):
        self._x_tick_labels = x_ticks

    def set_y_ticks(self, y_ticks=None):
        self._y_tick_labels = y_ticks

    def create_new_plot_canvas(self, num_axes):
        """Creates a new blank plotting canvas"""
        self.toolBar.reset_gridline_flags()
        self._plot_information_list = []
        self._number_of_axes = num_axes
        self._color_queue = [ColorQueue(DEFAULT_COLOR_CYCLE) for _ in range(num_axes)]
        self.fig.clf()
        self.fig, axes = get_plot_fig(overplot=False, ax_properties=None, axes_num=num_axes,
                                      fig=self.fig)
        if self._settings.is_condensed:
            self.fig.subplots_adjust(wspace=0, hspace=0)
        else:
            self.fig.tight_layout()
        self.fig.canvas.draw()

    def clear_all_workspaces_from_plot(self):
        """Clears all workspaces from the plot"""
        for ax in self.fig.axes:
            ax.cla()
            ax.tracked_workspaces.clear()
            ax.set_prop_cycle(None)

        for color_queue in self._color_queue:
            color_queue.reset()

        self._plot_information_list = []

    def _make_plot(self, workspace_plot_info: WorkspacePlotInformation):
        workspace_name = workspace_plot_info.workspace_name
        try:
            workspace = AnalysisDataService.Instance().retrieve(workspace_name)
        except (RuntimeError, KeyError):
            return -1
        self._plot_information_list.append(workspace_plot_info)
        errors = workspace_plot_info.errors
        ws_index = workspace_plot_info.index
        axis_number = workspace_plot_info.axis
        ax = self.fig.axes[axis_number]
        plot_kwargs = self._get_plot_kwargs(workspace_plot_info)
        plot_kwargs['color'] = self._color_queue[axis_number]()
        _do_single_plot(ax, workspace, ws_index, errors=errors,
                        plot_kwargs=plot_kwargs)
        return axis_number

    def add_workspaces_to_plot(self, workspace_plot_info_list: List[WorkspacePlotInformation]):
        """Add a list of workspaces to the plot - The workspaces are contained in a list PlotInformation
        The PlotInformation contains the workspace name, workspace index and target axis."""
        nrows, ncols = get_num_row_and_col(self._number_of_axes)
        for workspace_plot_info in workspace_plot_info_list:
            axis_number = self._make_plot(workspace_plot_info)
            if axis_number < 0:
                continue
            self._set_text_tick_labels(axis_number)
            if self._settings.is_condensed:
                self.hide_axis(axis_number, nrows, ncols)
        #remove labels from empty plots
        if self._settings.is_condensed:
            for axis_number in range(int(self._number_of_axes), int(nrows*ncols)):
                self.hide_axis(axis_number, nrows, ncols)

    def _wrap_labels(self, labels: list) -> list:
        """Wraps a list of labels so that every line is at most self._settings.wrap_width characters long."""
        return ["\n".join(wrap(label, self._settings.wrap_width)) for label in labels]

    def _set_text_tick_labels(self, axis_number):
        ax = self.fig.axes[axis_number]
        if self._x_tick_labels:
            ax.set_xticks(range(len(self._x_tick_labels)))
            labels = self._wrap_labels(self._x_tick_labels)
            ax.set_xticklabels(labels, fontsize = self._settings.font_size, rotation = self._settings.rotation, ha = "right")
        if self._y_tick_labels:
            ax.set_yticks(range(len(self._y_tick_labels)))
            labels = self._wrap_labels(self._y_tick_labels)
            ax.set_yticklabels(labels, fontsize = self._settings.font_size)

    def hide_axis(self, axis_number, nrows, ncols):
        row, col = convert_index_to_row_and_col(axis_number,  nrows, ncols)
        ax = self.fig.axes[axis_number]
        if row != nrows-1:
            labels = ["" for item in ax.get_xticks().tolist()]
            ax.set_xticklabels(labels)
            ax.xaxis.label.set_visible(False)
        if col != 0 and col != ncols-1:
            labels = ["" for item in ax.get_yticks().tolist()]
            ax.set_yticklabels(labels)
            ax.yaxis.label.set_visible(False)
        elif col == ncols-1 and ncols>1:
            ax.yaxis.set_label_position('right')
            ax.yaxis.tick_right()

    def remove_workspace_info_from_plot(self, workspace_plot_info_list: List[WorkspacePlotInformation]):
        # We reverse the workspace info list so that we can maintain a unique color queue
        # See _update_color_queue_on_workspace_removal for more
        workspace_plot_info_list.reverse()
        for workspace_plot_info in workspace_plot_info_list:
            workspace_name = workspace_plot_info.workspace_name
            if not AnalysisDataService.Instance().doesExist(workspace_name):
                continue

            workspace = AnalysisDataService.Instance().retrieve(workspace_name)
            for plotted_information in self._plot_information_list.copy():
                if workspace_plot_info.workspace_name == plotted_information.workspace_name and \
                        workspace_plot_info.axis == plotted_information.axis:
                    self._update_color_queue_on_workspace_removal(workspace_plot_info.axis, workspace_name)
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
                self._update_color_queue_on_workspace_removal(workspace_plot_info.axis, workspace_name)
                axis = self.fig.axes[workspace_plot_info.axis]
                axis.remove_workspace_artists(workspace)
                self._plot_information_list.remove(workspace_plot_info)

    def _update_color_queue_on_workspace_removal(self, axis_number, workspace_name):
        try:
            artist_info = self.fig.axes[axis_number].tracked_workspaces[workspace_name]
        except KeyError:
            return
        for ws_artist in artist_info:
            for artist in ws_artist._artists:
                if isinstance(artist, ErrorbarContainer):
                    color = artist[0].get_color()
                else:
                    color = artist.get_color()
                # When we repeat colors we don't want to add colors to the queue if they are already plotted.
                # We know we are repeating colors if we have more lines than colors, then we check if the color
                # removed is already the color of an existing line. If it is we don't manually re-add the color
                # to the queue. This ensures we only plot lines of the same colour if we have more lines
                # plotted than colours
                lines = self.fig.axes[axis_number].get_lines()
                if len(lines) > NUMBER_OF_COLOURS:
                    current_colors = [line.get_c() for line in lines]
                    if color in current_colors:
                        return
                self._color_queue[axis_number] += color

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
            ymin_i, ymax_i = self._get_y_axis_autoscale_limits(axis)
            if ymin_i < ymin:
                ymin = ymin_i
            if ymax_i > ymax:
                ymax = ymax_i
        plt.setp(self.fig.axes, ylim=[ymin, ymax])

    @property
    def get_xlim_list(self):
        xlim_list=[]
        for axis in self.fig.axes:
            min, max = axis.get_xlim()
            xlim_list.append([min,max])
        return xlim_list

    @property
    def get_ylim_list(self):
        ylim_list=[]
        for axis in self.fig.axes:
            min, max = axis.get_ylim()
            ylim_list.append([min,max])
        return ylim_list

    def autoscale_selected_y_axis(self, axis_number):
        if axis_number >= len(self.fig.axes):
            return
        axis = self.fig.axes[axis_number]
        bottom, top, = self._get_y_axis_autoscale_limits(axis)
        axis.set_ylim(bottom, top)

    def set_title(self, axis_number, title):
        if axis_number >= self.number_of_axes or self._settings.is_condensed:
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
        if not self._settings.is_condensed:
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
        plot_kwargs["marker"] = self._settings.get_marker(workspace_info.workspace_name)
        plot_kwargs["linestyle"] = self._settings.get_linestyle(workspace_info.workspace_name)
        return plot_kwargs

    def _get_y_axis_autoscale_limits(self, axis):
        x_min, x_max = sorted(axis.get_xlim())
        y_min, y_max = np.inf, -np.inf
        for line in axis.lines:
            y_min, y_max = get_y_min_max_between_x_range(line, x_min, x_max, y_min, y_max)
        if y_min == np.inf:
            y_min = -self._min_y_range
        if y_max == -np.inf:
            y_max = self._min_y_range
        if y_min == y_max:
            y_min -= self._min_y_range
            y_max += self._min_y_range
        y_margin = abs(y_max - y_min) * self._y_axis_margin

        return y_min - y_margin, y_max + y_margin

    def _reset_color_cycle(self):
        for i, ax in enumerate(self.fig.axes):
            ax.cla()
            ax.tracked_workspaces.clear()

    def resizeEvent(self, event):
        if self._settings.is_condensed:
            return
        self.fig.tight_layout()

    def add_uncheck_autoscale_subscriber(self, observer):
        self.toolBar.uncheck_autoscale_notifier.add_subscriber(observer)

    def add_enable_autoscale_subscriber(self, observer):
        self.toolBar.enable_autoscale_notifier.add_subscriber(observer)

    def add_disable_autoscale_subscriber(self, observer):
        self.toolBar.uncheck_autoscale_notifier.add_subscriber(observer)

    def add_range_changed_subscriber(self, observer):
        self.toolBar.range_changed_notifier.add_subscriber(observer)
