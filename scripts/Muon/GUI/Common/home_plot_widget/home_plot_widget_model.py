# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


class HomePlotWidgetModel(object):
    def __init__(self, plotting_window_model):
        """
        :param plotting_window_model: This is the plotting manager class to use
        """
        self._plotting_window_constructor = plotting_window_model
        self.plot_window = None
        self.plotted_workspaces = []
        self.plotted_group = ''

    def plot(self, workspace_list, title):
        """
        Plots a list of workspaces in a new plot window, closing any existing plot windows.
        :param workspace_list: A list of workspace name to plot. They must be in the ADS
        :param title: The name to give to the subplot created, currently only one subplot is ever created
        :return: A reference to the newly created plot window is passed back
        """
        if self.plot_window:
            self.plot_window.emit_close()

        self.plot_window = self._plotting_window_constructor('Muon Analysis', close_callback=self._close_plot)

        plotting = self.plot_window.window
        plotting.add_subplot(title)

        for workspace in workspace_list:
            plotting.plot(title, workspace)

        self.plotted_workspaces = workspace_list

        return self.plot_window

    def add_workspace_to_plot(self, subplot_name, workspace, specNum):
        """
        Adds a plot line to the specified subplot
        :param subplot_name: Name of subplot to which to add a workspace
        :param workspace: Name of workspace to get plot data from
        :param specNum: Spectrum number to plot from workspace
        :return:
        """
        self.plot_window.window.plot(subplot_name, workspace, specNum=specNum)

    def _close_plot(self):
        """
        callback to call when the plot window is closed. Removes the reference and resets plotted workspaces
        :return:
        """
        self._plot_window = None
        self.plotted_workspaces = []
