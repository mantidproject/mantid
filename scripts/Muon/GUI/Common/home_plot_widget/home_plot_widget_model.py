# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import numpy as np

from mantidqt.plotting.functions import plot
from mantid.api import AnalysisDataService


class HomePlotWidgetModel(object):
    def __init__(self):
        """
        :param plotting_window_model: This is the plotting manager class to use
        """
        self.plot_figure = None
        self._plotted_workspaces = []
        self._plotted_workspaces_inverse_binning = {}
        self._plotted_fit_workspaces = []
        self.plotted_group = ''

    @property
    def plotted_workspaces(self):
        """
        This property is needed to check whether all the workspaces contained in the list are still on the graph. They can
        be removed from the graph from the figure window without this class knowing.
        :return:
        """
        self._plotted_workspaces = [item for item in self._plotted_workspaces if item in
                                    self.plot_figure.gca().tracked_workspaces.keys()]
        return self._plotted_workspaces

    @property
    def plotted_workspaces_inverse_binning(self):
        self._plotted_workspaces_inverse_binning = {key: item for key, item in self._plotted_workspaces_inverse_binning.items()
                                                    if key in self.plot_figure.gca().tracked_workspaces.keys()}
        return self._plotted_workspaces_inverse_binning

    @property
    def plotted_fit_workspaces(self):
        self._plotted_fit_workspaces = [item for item in self._plotted_fit_workspaces if item in
                                        self.plot_figure.gca().tracked_workspaces.keys()]
        return self._plotted_fit_workspaces

    @plotted_workspaces.setter
    def plotted_workspaces(self, value):
        self._plotted_workspaces = value

    @plotted_workspaces_inverse_binning.setter
    def plotted_workspaces_inverse_binning(self, value):
        self._plotted_workspaces_inverse_binning = value

    @plotted_fit_workspaces.setter
    def plotted_fit_workspaces(self, value):
        self._plotted_fit_workspaces = value

    def plot(self, workspace_list, title, domain, force_redraw, window_title):
        """
        Plots a list of workspaces in a new plot window, closing any existing plot windows.
        :param workspace_list: A list of workspace name to plot. They must be in the ADS
        :param title: The name to give to the subplot created, currently only one subplot is ever created
        :param domain: if frequency or time domain
        :param force_redraw: if to force a redraw
        :param window_title: title for the plot window
        :return: A reference to the newly created plot window is passed back
        """
        if not workspace_list:
            self.plotted_workspaces = []
            self.plotted_workspaces_inverse_binning = {}
            self.plotted_fit_workspaces = []
            self.plot_figure.clear()
            self.plot_figure.canvas.draw()
            return self.plot_figure
        try:
            workspaces = AnalysisDataService.Instance().retrieveWorkspaces(workspace_list, unrollGroups=True)
        except RuntimeError:
            return
        if (force_redraw or self.plotted_workspaces == []) and self.plot_figure:

            self.plot_figure.clear()
            self.plotted_workspaces = []
            self.plotted_workspaces_inverse_binning = {}
            self.plotted_fit_workspaces = []
            self.plot_figure = plot(workspaces,wksp_indices=[0], fig=self.plot_figure, window_title=title,
                                    plot_kwargs={'distribution': True, 'autoscale_on_update': False}, errors=True)
            self.set_x_lim(domain)

        elif self.plot_figure:
            axis = self.plot_figure.gca()
            xlim = axis.get_xlim()
            ylim = axis.get_ylim()
            self._remove_all_data_workspaces_from_plot()
            self.plot_figure = plot(workspaces, wksp_indices=[0], fig=self.plot_figure, window_title=title,
                                    plot_kwargs={'distribution': True, 'autoscale_on_update': False}, errors=True)
            axis = self.plot_figure.gca()
            axis.set_xlim(xlim)
            axis.set_ylim(ylim)
        else:
            self.plot_figure = plot(workspaces, wksp_indices=[0], window_title=title, plot_kwargs={'distribution': True,
                                    'autoscale_on_update': False}, errors=True)
            self.set_x_lim(domain)

        self.plot_figure.canvas.set_window_title(window_title)
        self.plot_figure.gca().set_title(title)

        self.plot_figure.canvas.window().closing.connect(self._clear_plot_references)

        workspaces_to_remove = [workspace for workspace in self.plotted_workspaces if workspace not in workspace_list]
        for workspace in workspaces_to_remove:
            self.remove_workpace_from_plot(workspace)

        self.plotted_workspaces = workspace_list

    def set_x_lim(self,domain):
        if domain == "Time":
            self.plot_figure.gca().set_xlim(left=0.0, right=15.0)
            self.autoscale_y_to_data_in_view()
            self.plot_figure.canvas.draw()

    def add_workspace_to_plot(self, workspace_name, workspace_index, label):
        """
        Adds a plot line to the specified subplot
        :param workspace: Name of workspace to get plot data from
        :param workspace_index: workspace index to plot from workspace
        :return:
        """
        try:
            workspaces = AnalysisDataService.Instance().retrieveWorkspaces([workspace_name], unrollGroups=True)
        except RuntimeError:
            return

        if all([workspace.getNumberHistograms() == 4 for workspace in workspaces]) and workspace_index == 1:
            workspace_index = 3

        self.plot_figure = plot(workspaces, wksp_indices=[workspace_index], fig=self.plot_figure, overplot=True,
                                plot_kwargs={'distribution': True, 'zorder': 4, 'autoscale_on_update': False, 'label': label})

        if workspace_name not in self._plotted_fit_workspaces:
            self._plotted_fit_workspaces.append(workspace_name)

    def remove_workpace_from_plot(self, workspace_name):
        """
        :param workspace_name: Name of workspace to remove from plot
        :return:
        """
        try:
            workspace = AnalysisDataService.Instance().retrieve(workspace_name)
        except RuntimeError:
            return
        self.plot_figure.gca().remove_workspace_artists(workspace)
        self.plotted_workspaces = [item for item in self.plotted_workspaces if item != workspace_name]
        self.plotted_fit_workspaces = [item for item in self.plotted_fit_workspaces if item != workspace_name]
        if workspace_name in self.plotted_workspaces_inverse_binning:
            self.plotted_workspaces_inverse_binning.pop(workspace_name)

    def _clear_plot_references(self):
        """
        callback to call when the plot window is closed. Removes the reference and resets plotted workspaces
        :return:
        """
        self.plot_figure = None
        self.plotted_workspaces = []
        self.plotted_workspaces_inverse_binning = {}
        self.plotted_fit_workspaces = []

    def force_redraw(self):
        if not self.plot_figure:
            return

        self.plot_figure.canvas.draw()

    def autoscale_y_to_data_in_view(self):
        axis = self.plot_figure.gca()
        xlim = axis.get_xlim()
        ylim = np.inf, -np.inf
        for line in axis.lines:
            x, y = line.get_data()
            start, stop = np.searchsorted(x, xlim)
            y_within_range = y[max(start-1,0):(stop+1)]
            ylim = min(ylim[0], np.nanmin(y_within_range)), max(ylim[1], np.nanmax(y_within_range))

        new_bottom = ylim[0] * 1.3 if ylim[0] < 0.0 else ylim[0] * 0.7
        new_top = ylim[1] * 1.3 if ylim[1] > 0.0 else ylim[1] * 0.7

        axis.set_ylim(bottom=new_bottom, top=new_top)

    def _remove_all_data_workspaces_from_plot(self):
        workspaces_to_remove = self.plotted_workspaces
        for workspace in workspaces_to_remove:
            self.remove_workpace_from_plot(workspace)
