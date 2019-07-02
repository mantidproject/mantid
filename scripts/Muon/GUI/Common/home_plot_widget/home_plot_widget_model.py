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
        self.plotted_workspaces = []
        self.plotted_workspaces_inverse_binning = []
        self.plotted_fit_workspaces = []
        self.plotted_group = ''

    def plot(self, workspace_list, title, domain, force_redraw):
        """
        Plots a list of workspaces in a new plot window, closing any existing plot windows.
        :param workspace_list: A list of workspace name to plot. They must be in the ADS
        :param title: The name to give to the subplot created, currently only one subplot is ever created
        :param domain: if frequency or time domain
        :param force_redraw: if to force a redraw
        :return: A reference to the newly created plot window is passed back
        """
        if not workspace_list:
            self.plot_figure.clear()
            self.plot_figure.canvas.draw()
            return self.plot_figure
        try:
            workspaces = AnalysisDataService.Instance().retrieveWorkspaces(workspace_list, unrollGroups=True)
        except RuntimeError:
            return

        if force_redraw and self.plot_figure:
            self.plot_figure.clear()
            self.plot_figure = plot(workspaces, spectrum_nums=[1], fig=self.plot_figure, window_title=title,
                                    plot_kwargs={'distribution': True, 'autoscale_on_update': False}, errors=True)
            self.set_x_lim(domain)
            
        elif self.plot_figure:
            self.plot_figure = plot(workspaces, spectrum_nums=[1], fig=self.plot_figure, window_title=title,
                                    plot_kwargs={'distribution': True, 'autoscale_on_update': False}, errors=True)
        else:
            self.plot_figure = plot(workspaces, spectrum_nums=[1], window_title=title, plot_kwargs={'distribution': True,
                                                                                                    'autoscale_on_update': False},
                                    errors=True)
            self.set_x_lim(domain)

        self.plot_figure.canvas.set_window_title('Muon Analysis')
        self.plot_figure.gca().set_title(title)

        self.plot_figure.canvas.window().closing.connect(self._close_plot)

        workspaces_to_remove = [workspace for workspace in self.plotted_workspaces if workspace not in workspace_list]
        for workspace in workspaces_to_remove:
            self.remove_workpace_from_plot(workspace)

        self.plotted_workspaces = workspace_list

    def set_x_lim(self,domain):
        if domain == "Time":
            self.plot_figure.gca().set_xlim(left=0.0, right=15.0)
            self.autoscale_y_to_data_in_view()
            self.plot_figure.canvas.draw()

    def add_workspace_to_plot(self, workspace, specNum, label):
        """
        Adds a plot line to the specified subplot
        :param workspace: Name of workspace to get plot data from
        :param specNum: Spectrum number to plot from workspace
        :return:
        """
        try:
            workspaces = AnalysisDataService.Instance().retrieveWorkspaces([workspace], unrollGroups=True)
        except RuntimeError:
            return

        self.plot_figure = plot(workspaces, spectrum_nums=[specNum], fig=self.plot_figure, overplot=True,
                                plot_kwargs={'distribution': True, 'zorder': 4, 'autoscale_on_update': False, 'label': label})

        self.plotted_fit_workspaces.append(workspace)

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

    def _close_plot(self):
        """
        callback to call when the plot window is closed. Removes the reference and resets plotted workspaces
        :return:
        """
        self.plot_figure = None
        self.plotted_workspaces = []

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
