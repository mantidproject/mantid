# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import numpy as np

from mantid.api import AnalysisDataService
from matplotlib.container import ErrorbarContainer
import matplotlib.pyplot as plt

legend_text_size = 7


class PlotWidgetModel(object):

    def __init__(self):
        self._plotted_workspaces = []
        self._plotted_workspaces_inverse_binning = {}
        self._plotted_fit_workspaces = []
        self.tiled_plot_positions = {}
        self._number_of_axes = 1

    # ------------------------------------------------------------------------------------------------------------------
    # Properties
    # ------------------------------------------------------------------------------------------------------------------

    @property
    def number_of_axes(self):
        """
        This property is needed as the view will generate an even number of axes, which can exceed the number of axes
        required by the model
        """
        return self._number_of_axes

    @property
    def plotted_workspaces(self):
        """
        A list of the plotted workspaces in the figure
        """
        return self._plotted_workspaces

    @property
    def plotted_workspaces_inverse_binning(self):
        return self._plotted_workspaces_inverse_binning

    @property
    def plotted_fit_workspaces(self):
        """
        A list of the plotted fit workspaces in the figure
        """
        return self._plotted_fit_workspaces

    # ------------------------------------------------------------------------------------------------------------------
    # Setters
    # ------------------------------------------------------------------------------------------------------------------

    @number_of_axes.setter
    def number_of_axes(self, number_of_axes):
        if number_of_axes > 1:
            self._number_of_axes = number_of_axes
        else:
            self._number_of_axes = 1

    @plotted_workspaces.setter
    def plotted_workspaces(self, workspaces):
        self._plotted_workspaces = workspaces

    @plotted_workspaces_inverse_binning.setter
    def plotted_workspaces_inverse_binning(self, value):
        self._plotted_workspaces_inverse_binning = value

    @plotted_fit_workspaces.setter
    def plotted_fit_workspaces(self, workspaces):
        self._plotted_fit_workspaces = workspaces

    # ------------------------------------------------------------------------------------------------------------------
    # Helper to add and remove from stored workspace lists
    # ------------------------------------------------------------------------------------------------------------------

    def add_workspace_to_plotted_workspaces(self, workspace_name):
        if workspace_name not in self.plotted_workspaces:
            self._plotted_workspaces.append(workspace_name)

    def remove_workspaces_from_plotted_workspaces(self, workspace_name):
        if workspace_name in self.plotted_workspaces:
            self._plotted_workspaces.remove(workspace_name)

    def add_workspace_to_plotted_fit_workspaces(self, workspace_name):
        if workspace_name not in self.plotted_fit_workspaces:
            self._plotted_fit_workspaces.append(workspace_name)

    def remove_workspaces_from_plotted_fit_workspaces(self, workspace_name):
        if workspace_name in self.plotted_fit_workspaces:
            self._plotted_fit_workspaces.remove(workspace_name)

    # ------------------------------------------------------------------------------------------------------------------
    # Plotting
    # ------------------------------------------------------------------------------------------------------------------

    def add_workspace_to_plot(self, ax, workspace_name, workspace_indices, errors, plot_kwargs):
        """
            Adds a plot line to the specified axis
            :param ax: Axis to plot the workspace on
            :param workspace_name: Name of workspace to get plot data from
            :param workspace_indices: workspace indices to plot from workspace
            :param errors: Plotting workspace errors
            :param plot_kwargs, arguments to Mantid axis plotting
            :return:
            """
        # check workspace exists -
        # retrieveWorkspaces expects a list of workspace names
        try:
            workspace = AnalysisDataService.Instance().retrieveWorkspaces([workspace_name], unrollGroups=True)
        except RuntimeError:
            return

        self._do_single_plot(ax, workspace, workspace_indices, errors,
                             plot_kwargs)
        self._update_legend(ax)

    def remove_workspace_from_plot(self, workspace_name, axes):
        """
        Remove workspace from plot
        :param workspace_name: Name of workspace to remove from plot
        :param axes: the axes which may contain the workspace
        :return:
        """
        if workspace_name not in self.plotted_workspaces + self.plotted_fit_workspaces:
            return

        try:
            workspace = AnalysisDataService.Instance().retrieve(workspace_name)
        except RuntimeError:
            return

        for i in range(self.number_of_axes):
            ax = axes[i]
            ax.remove_workspace_artists(workspace)
            self._update_legend(ax)

        self.plotted_workspaces = [item for item in self.plotted_workspaces if item != workspace_name]
        self.plotted_fit_workspaces = [item for item in self.plotted_fit_workspaces if item != workspace_name]
        if workspace_name in self.plotted_workspaces_inverse_binning:
            self.plotted_workspaces_inverse_binning.pop(workspace_name)

    def workspace_deleted_from_ads(self, workspace, axes):
        """
        Remove a workspace which was deleted in the ads from the plot
        :param workspace: Workspace object to remove from plot
        :param axes: the axes which may contain the workspace
        :return:
        """
        workspace_name = workspace.name()

        if workspace_name not in self.plotted_workspaces + self.plotted_fit_workspaces:
            return

        for i in range(self.number_of_axes):
            ax = axes[i]
            ax.remove_workspace_artists(workspace)
            self._update_legend(ax)

        self.plotted_workspaces = [item for item in self.plotted_workspaces if item != workspace_name]
        self.plotted_fit_workspaces = [item for item in self.plotted_fit_workspaces if item != workspace_name]
        if workspace_name in self.plotted_workspaces_inverse_binning:
            self.plotted_workspaces_inverse_binning.pop(workspace_name)

    def _do_single_plot(self, ax, workspaces, indices, errors, plot_kwargs):
        plot_fn = ax.errorbar if errors else ax.plot
        for ws in workspaces:
            for index in indices:
                plot_kwargs['wkspIndex'] = index
                plot_fn(ws, **plot_kwargs)

    def replace_workspace_plot(self, workspace_name, axis):
        """
        Replace workspace from plot
        :param workspace_name: Name of workspace to update in plot
        :param axis: the axis that contains the workspace
        :return:
        """
        try:
            workspace = AnalysisDataService.Instance().retrieve(workspace_name)
        except RuntimeError:
            return

        axis.replace_workspace_artists(workspace)

    def replot_workspace(self, workspace_name, axis, errors, plot_kwargs):
        """
        Replot a workspace with different kwargs, and error flag
        with the intention of keeping the rest of the model unchanged
        :param workspace_name: Name of workspace to update in plot
        :param axis: the axis that contains the workspace
        :param errors: Plotting with errors
        :param plot_kwargs: kwargs to the plotting
        :return:
        """
        artist_info = axis.tracked_workspaces[workspace_name]
        for ws_artist in artist_info:
            for artist in ws_artist._artists:
                # if the artist in an errorbarCotainer, the first object
                # in the tuple will contain the matplotlib line object
                if isinstance(artist, ErrorbarContainer):
                    color = artist[0].get_color()
                else:
                    color = artist.get_color()

                plot_kwargs["color"] = color
                axis.replot_artist(artist, errors, **plot_kwargs)

        self._update_legend(axis)

    def clear_plot_model(self, axes):
        self._remove_all_data_workspaces_from_plot(axes)
        self.plotted_workspaces = []
        self.plotted_workspaces_inverse_binning = {}
        self.plotted_fit_workspaces = []

    def remove_fit_workspace(self, fit_workspace, axes):
        if fit_workspace in self.plotted_fit_workspaces:
            self.remove_workspace_from_plot(fit_workspace, axes)

    def _remove_all_data_workspaces_from_plot(self, axes):
        workspaces_to_remove = self.plotted_workspaces
        for workspace in workspaces_to_remove:
            self.remove_workspace_from_plot(workspace, axes)
        workspaces_to_remove = self.plotted_fit_workspaces
        for workspace in workspaces_to_remove:
            self.remove_workspace_from_plot(workspace, axes)



    def autoscale_axes(self, axes, xlimits):
        ymin, ymax = self._get_autoscale_y_limits(axes, xlimits[0], xlimits[1])
        plt.setp(axes, xlim=xlimits, ylim=[ymin, ymax])
        return ymin, ymax

    def set_axis_xlim(self, axis, xlims):
        axis.set_xlim(left=xlims[0], right=xlims[1])

    def set_axis_ylim(self, axis, ylims):
        axis.set_ylim(ylims[0], ylims[1])

    def _get_autoscale_y_limits(self, axes, xmin, xmax):
        new_bottom = 1e9
        new_top = -1e9
        for axis in axes:
            axis.set_xlim(left=xmin, right=xmax)
            xlim = axis.get_xlim()
            ylim = np.inf, -np.inf
            for line in axis.lines:
                x, y = line.get_data()
                start, stop = np.searchsorted(x, xlim)
                y_within_range = y[max(start - 1, 0):(stop + 1)]
                ylim = min(ylim[0], np.nanmin(y_within_range)), max(ylim[1], np.nanmax(y_within_range))

            new_bottom_i = ylim[0] * 1.3 if ylim[0] < 0.0 else ylim[0] * 0.7
            new_top_i = ylim[1] * 1.3 if ylim[1] > 0.0 else ylim[1] * 0.7
            if new_bottom_i < new_bottom:
                new_bottom = new_bottom_i
            if new_top_i > new_top:
                new_top = new_top_i

        return new_bottom, new_top

    def _update_legend(self, ax):
        handles, _ = ax.get_legend_handles_labels()
        if handles:
            ax.legend(prop=dict(size=legend_text_size))
        else:
            ax.legend("")

    def get_axes_titles(self, axes):
        titles = [None] * self.number_of_axes
        for i in range(self.number_of_axes):
            titles[i] = axes[i].get_title()
        return titles
