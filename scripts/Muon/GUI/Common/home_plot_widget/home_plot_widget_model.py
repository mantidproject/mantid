# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string

COUNTS_PLOT_TYPE = 'Counts'
ASYMMETRY_PLOT_TYPE = 'Asymmetry'

class HomePlotWidgetModel(object):

    def __init__(self, plotting_window_model, context):
        self.context = context
        self._plotting_window_constructor = plotting_window_model
        self.plot_window = None
        self.plotted_workspaces = []

    def plot(self, plot_type, use_raw):
        workspace_list = self.get_workspaces_to_plot(self.context.group_pair_context.selected, use_raw, plot_type)
        title = self.get_plot_title()

        if self.plot_window:
            self.plot_window.close()

        self.plot_window = self._plotting_window_constructor('Muon Analysis', close_callback=self._close_plot)

        plotting = self.plot_window.multi_plot
        plotting.add_subplot(title)

        for workspace in workspace_list:
            plotting.plot(title, workspace)

        self.plotted_workspaces = workspace_list

    def create_new_plot(self, workspace_list, title, callback):
        plot_window = self._plotting_window_constructor('Muon Analysis', close_callback=callback)

        plotting = plot_window.multi_plot
        plotting.add_subplot(title)

        for workspace in workspace_list:
            plotting.plot(title, workspace)

        return plot_window

    def get_workspaces_to_plot(self, current_group_pair, is_raw, plot_type):
        if is_raw:
            workspace_list = self.context.group_pair_context[current_group_pair].get_asymmetry_workspace_names\
                (self.context.data_context.current_runs)
        else:
            workspace_list  = self.context.group_pair_context[current_group_pair].get_asymmetry_workspace_names_rebinned\
                (self.context.data_context.current_runs)

        if plot_type == COUNTS_PLOT_TYPE:
            workspace_list = [item.replace(ASYMMETRY_PLOT_TYPE, COUNTS_PLOT_TYPE) for item in workspace_list]

        return workspace_list

    def get_plot_title(self):
        flattened_run_list = [item for sublist in self.context.data_context.current_runs for item in sublist]
        return self.context.data_context.instrument + ' ' + run_list_to_string(flattened_run_list) + ' ' +\
               self.context.group_pair_context.selected

    def _close_plot(self):
        self._plot_window = None
        self.plotted_workspaces = []
