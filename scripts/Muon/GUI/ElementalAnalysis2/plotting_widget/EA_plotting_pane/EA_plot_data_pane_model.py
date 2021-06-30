# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plot_widget.base_pane.base_pane_model import BasePaneModel


class EAPlotDataPaneModel(BasePaneModel):

    def __init__(self, context):
        super(EAPlotDataPaneModel, self).__init__(context)
        self.context.plot_panes_context[self.name].set_defaults([-100.0, 8200.0], [-100, 2000])

    @staticmethod
    def _generate_run_indices(workspace_list):
        indices = [2] * len(workspace_list)
        return indices

    def get_count_workspaces_to_plot(self, current_group, is_raw, plot_type):
        """
        :param current_group:
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: Whether to plot counts or asymmetry
        :return: a list of workspace names
        """
        try:
            if is_raw:
                workspace_list = [self.context.group_context[current_group].get_counts_workspace_for_run(False)]
            else:
                workspace_list = [self.context.group_context[current_group].get_counts_workspace_for_run(True)]

            return workspace_list
        except AttributeError:
            return []

    def get_workspaces_to_remove(self, group_names, is_raw, plot_type):
        """
        :param group_names:
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: plotting type, e.g Counts, Frequency Re
        :return: a list of workspace names
        """
        workspace_list = []
        for group in group_names:
            workspace_list += self.get_count_workspaces_to_plot(group, is_raw, plot_type)
        return workspace_list

    def get_workspace_and_indices_for_group(self, group, is_raw, plot_type):
        """
         :return: a list of workspace names and corresponding indices to plot
         """
        workspace_list = self.get_count_workspaces_to_plot(group, is_raw, plot_type)
        indices = self._generate_run_indices(workspace_list)

        return workspace_list, indices

    def get_workspaces_to_plot(self, is_raw, plot_type):
        """
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: plotting type, e.g Counts, Frequency Re
        :return: a list of workspace names
        """
        currently_selected = self.context.group_context.selected_groups
        workspace_list = []
        for group in currently_selected:
            workspace_list += self.get_count_workspaces_to_plot(group, is_raw, plot_type)
        return workspace_list

    def get_workspace_list_and_indices_to_plot(self, is_raw, plot_type):
        """
         :return: a list of workspace names to plot
         """
        workspace_list = self.get_workspaces_to_plot(is_raw, plot_type)
        plot_range = self.get_plot_range_from_group()
        indices = self._generate_run_indices(workspace_list)

        return workspace_list, indices, plot_range

    def get_plot_range_from_group(self):
        currently_selected = self.context.group_context.selected_groups
        current_plot_range = [-50.0, 1100.0]
        for group in currently_selected:
            print(self.context.group_context[group].plot_range)
            if self.context.group_context[group].plot_range[1] > current_plot_range[1]:
                current_plot_range = self.context.group_context[group].plot_range
        return current_plot_range
