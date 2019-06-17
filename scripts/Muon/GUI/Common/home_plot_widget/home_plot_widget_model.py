# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantidqt.plotting.functions import plot
from mantid.api import AnalysisDataService


class HomePlotWidgetModel(object):
    def __init__(self):
        """
        :param plotting_window_model: This is the plotting manager class to use
        """
        self.plot_figure = None
        self.plotted_workspaces = []
        self.plotted_group = ''

    def plot(self, workspace_list, title):
        """
        Plots a list of workspaces in a new plot window, closing any existing plot windows.
        :param workspace_list: A list of workspace name to plot. They must be in the ADS
        :param title: The name to give to the subplot created, currently only one subplot is ever created
        :return: A reference to the newly created plot window is passed back
        """
        if not workspace_list:
            self.plot_figure.clear()
            self.plot_figure.canvas.draw()
            return self.plot_figure

        workspaces = AnalysisDataService.Instance().retrieveWorkspaces(workspace_list, unrollGroups=True)
        if self.plot_figure:
            self.plot_figure.clear()
            self.plot_figure = plot(workspaces, spectrum_nums=[1], fig=self.plot_figure, window_title=title,
                                    plot_kwargs={'distribution': True}, errors=True)
        else:
            self.plot_figure = plot(workspaces, spectrum_nums=[1], window_title=title, plot_kwargs={'distribution': True},
                                    errors=True)

        self.plot_figure.canvas.set_window_title('Muon Analysis')
        self.plot_figure.gca().set_title(title)

        self.plot_figure.canvas.window().closing.connect(self._close_plot)

        self.plotted_workspaces = workspace_list

        return self.plot_figure

    def add_workspace_to_plot(self, workspace, specNum):
        """
        Adds a plot line to the specified subplot
        :param workspace: Name of workspace to get plot data from
        :param specNum: Spectrum number to plot from workspace
        :return:
        """
        workspaces = AnalysisDataService.Instance().retrieveWorkspaces([workspace], unrollGroups=True)
        self.plot_figure = plot(workspaces, spectrum_nums=[specNum], fig=self.plot_figure, overplot=True,
                                plot_kwargs={'distribution': True}, errors=True)

    def remove_workpace_from_plot(self, workspace_name):
        """
        :param workspace_name: Name of workspace to remove from plot
        :return:
        """
        workspace = AnalysisDataService.Instance().retrieve(workspace_name)
        self.plot_figure.gca().remove_workspace_artists(workspace)

    def _close_plot(self):
        """
        callback to call when the plot window is closed. Removes the reference and resets plotted workspaces
        :return:
        """
        self.plot_figure = None
        self.plotted_workspaces = []
