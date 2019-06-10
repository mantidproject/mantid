# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string

class HomePlotWidgetModel(object):

    def __init__(self, plotting_window_model, context):
        self.plot_raw = True
        self.overlay = True
        self.context = context
        self._plotting_window_constructor = plotting_window_model

    def create_new_plot(self, workspace_list, title, callback):
        plot_window = self._plotting_window_constructor('Muon Analysis', close_callback=callback)

        plotting = plot_window.multi_plot
        plotting.add_subplot(title)

        for workspace in workspace_list:
            plotting.plot(title, workspace)

        return plot_window

    def get_workspaces_to_plot(self, is_raw):
        current_group_pair = self.context.group_pair_context.selected
        if is_raw:
            return self.context.group_pair_context[current_group_pair].get_asymmetry_workspace_names\
                (self.context.data_context.current_runs)
        else:
            return self.context.group_pair_context[current_group_pair].get_asymmetry_workspace_names_rebinned \
                (self.context.data_context.current_runs)

    def get_plot_title(self):
        flattened_run_list = [item for sublist in self.context.data_context.current_runs for item in sublist]
        return self.context.data_context.instrument + ' ' + run_list_to_string(flattened_run_list) + ' ' +\
               self.context.group_pair_context.selected
