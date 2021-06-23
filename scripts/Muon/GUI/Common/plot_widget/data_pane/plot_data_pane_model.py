# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
from Muon.GUI.Common.plot_widget.base_pane.base_pane_model import BasePaneModel


class PlotDataPaneModel(BasePaneModel):

    def __init__(self, context):
        super().__init__(context)

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

            if plot_type == "Counts":
                workspace_list = [item.replace("Asymmetry", "Counts")
                                  for item in workspace_list if "Asymmetry" in item]

            return workspace_list
        except AttributeError:
            return []

    def get_workspace_and_indices_for_group_or_pair(self, group_or_pair, is_raw, plot_type):
        """
         :return: a list of workspace names and corresponding indices to plot
         """
        workspace_list = self.get_time_workspaces_to_plot(group_or_pair, is_raw, plot_type)
        indices = self._generate_run_indices(workspace_list)

        return workspace_list, indices

    def get_workspaces_to_plot(self, is_raw, plot_type):
        """
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: plotting type, e.g Counts, Frequency Re
        :return: a list of workspace names
        """
        currently_selected = self.context.group_pair_context.selected_groups_and_pairs
        workspace_list = []
        for group_pair in currently_selected:
            workspace_list += self.get_time_workspaces_to_plot(group_pair, is_raw, plot_type)
        return workspace_list

    def get_workspace_list_and_indices_to_plot(self, is_raw, plot_type):
        """
         :return: a list of workspace names to plot
         """
        workspace_list = self.get_workspaces_to_plot(is_raw, plot_type)
        indices = self._generate_run_indices(workspace_list)

        return workspace_list, indices

    def get_workspaces_to_remove(self, group_pair_names, is_raw, plot_type):
        """
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: plotting type, e.g Counts, Frequency Re
        :return: a list of workspace names
        """
        workspace_list = []
        for group_pair in group_pair_names:
            workspace_list += self.get_time_workspaces_to_plot(group_pair, is_raw, plot_type)
        return workspace_list

    def create_tiled_keys(self, tiled_by):
        if tiled_by == "Group/Pair":
            keys = self.context.group_pair_context.selected_groups_and_pairs
        else:
            keys = [run_list_to_string(item) for item in self.context.data_context.current_runs]
        return keys
