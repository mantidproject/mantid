# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plot_widget.data_pane.plot_group_pair_model import PlotGroupPairModel


class PlotDataPaneModel(PlotGroupPairModel):

    def __init__(self, context):
        super().__init__(context,"Plot Data")
        self.context.plot_panes_context[self.name].set_defaults([0., 15.0], [-0.3, 0.3])

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

    def get_workspace_and_indices_for_group_or_pair(self, group_or_pair, is_raw, plot_type):
        """
         :return: a list of workspace names and corresponding indices to plot
         """
        workspace_list = self.get_time_workspaces_to_plot(group_or_pair, is_raw, plot_type)
        indices = self._generate_run_indices(workspace_list)

        return workspace_list, indices

    def get_workspaces_to_plot(self, is_raw, plot_type,currently_selected):
        """
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: plotting type, e.g Counts, Frequency Re
        :return: a list of workspace names
        """
        workspace_list = []
        for group_pair in currently_selected:
            workspace_list += self.get_time_workspaces_to_plot(group_pair, is_raw, plot_type)
        return workspace_list

    def get_workspace_list_and_indices_to_plot(self, is_raw, plot_type, selected = None):
        """
         :return: a list of workspace names to plot
         """
        if selected is None:
            selected =  self.context.group_pair_context.selected_groups_and_pairs
        workspace_list = self.get_workspaces_to_plot(is_raw, plot_type, selected)
        indices = self._generate_run_indices(workspace_list)

        return workspace_list, indices
