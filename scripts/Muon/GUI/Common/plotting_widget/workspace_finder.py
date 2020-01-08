# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

COUNTS_PLOT_TYPE = 'Counts'
ASYMMETRY_PLOT_TYPE = 'Asymmetry'
FREQ_PLOT_TYPE = "Frequency "


class WorkspaceFinder(object):

    def __init__(self, context):
        self.context = context

    def get_workspace_list_to_plot(self, is_raw, plot_type):
        """
         :return: a list of workspace names to plot
         """
        workspace_list = self.get_workspaces_to_plot(is_raw, plot_type)
        return workspace_list

    def get_workspaces_to_plot(self, is_raw, plot_type):
        """
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: plotting type, e.g Counts, Frequency Re
        :return: a list of workspace names
        """
        currently_selected_groups = self.context.group_pair_context.selected_groups
        currently_selected_pairs = self.context.group_pair_context.selected_pairs
        workspace_list = []

        if FREQ_PLOT_TYPE in plot_type:
            for grouppair in currently_selected_groups + currently_selected_pairs:
                workspace_list += self.get_freq_workspaces_to_plot(grouppair, plot_type)
            return workspace_list
        else:
            for grouppair in currently_selected_groups + currently_selected_pairs:
                workspace_list += self.get_time_workspaces_to_plot(grouppair, is_raw, plot_type)
            return workspace_list

    def get_freq_workspaces_to_plot(self, current_group_pair, plot_type):
        """
        :param current_group_pair: The group/pair currently selected
        :param plot_type: Whether to plot counts or asymmetry
        :return: a list of workspace names
        """
        try:
            runs = ""
            seperator = ""
            for run in self.context.data_context.current_runs:
                runs += seperator + str(run[0])
                seperator = ", "
            workspace_list = self.context.get_names_of_frequency_domain_workspaces_to_fit(
                runs, current_group_pair, True, plot_type[len(FREQ_PLOT_TYPE):])

            return workspace_list
        except AttributeError:
            return []

    def get_time_workspaces_to_plot(self, current_group_pair, is_raw, plot_type):
        """
        :param current_group_pair: The group/pair currently selected
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: Whether to plot counts or asymmetry
        :return: a list of workspace names
        """
        try:
            if is_raw:
                workspace_list = self.context.group_pair_context[current_group_pair].get_asymmetry_workspace_names(
                    self.context.data_context.current_runs)
            else:
                workspace_list = self.context.group_pair_context[
                    current_group_pair].get_asymmetry_workspace_names_rebinned(
                    self.context.data_context.current_runs)

            if plot_type == COUNTS_PLOT_TYPE:
                workspace_list = [item.replace(ASYMMETRY_PLOT_TYPE, COUNTS_PLOT_TYPE)
                                  for item in workspace_list if ASYMMETRY_PLOT_TYPE in item]

            return workspace_list
        except AttributeError:
            return []
