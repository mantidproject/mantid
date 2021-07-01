# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plot_widget.base_pane.base_pane_model import BasePaneModel

SPECTRA_INDICES = {"Delayed": 0, "Prompt": 1, "Total": 2}


class EAPlotDataPaneModel(BasePaneModel):

    def __init__(self, context):
        super(EAPlotDataPaneModel, self).__init__(context)
        self.context.plot_panes_context[self.name].set_defaults([-10.0, 1000.0], [-100, 2000])

    @staticmethod
    def _generate_run_indices(workspace_list, plot_type):
        indices = [SPECTRA_INDICES[plot_type]] * len(workspace_list)
        return indices

    def get_count_workspaces_to_plot(self, current_group, is_raw):
        """
        :param current_group:
        :param is_raw: Whether to use raw or rebinned data
        :return: a list of workspace names
        """
        try:
            if is_raw:
                workspace_list = [self.context.group_context[current_group].get_counts_workspace_for_run(False)]
            else:
                workspace_list = \
                    [self.context.group_context[
                         current_group].get_rebined_or_unbinned_version_of_workspace_if_it_exists()]

            return workspace_list
        except AttributeError:
            return []

    def get_workspaces_to_remove(self, group_names, is_raw):
        """
        :param group_names:
        :param is_raw: Whether to use raw or rebinned data
        :return: a list of workspace names
        """
        workspace_list = []
        for group in group_names:
            workspace_list += self.get_count_workspaces_to_plot(group, is_raw)
        return workspace_list

    def get_workspace_and_indices_for_group(self, group, is_raw, plot_type):
        """
         :return: a list of workspace names and corresponding indices to plot
         """
        workspace_list = self.get_count_workspaces_to_plot(group, is_raw)
        indices = self._generate_run_indices(workspace_list, plot_type)

        return workspace_list, indices

    def get_workspaces_to_plot(self, is_raw):
        """
        :param is_raw: Whether to use raw or rebinned data
        :return: a list of workspace names
        """
        currently_selected = self.context.group_context.selected_groups
        workspace_list = []
        for group in currently_selected:
            workspace_list += self.get_count_workspaces_to_plot(group, is_raw)
        return workspace_list

    def get_workspace_list_and_indices_to_plot(self, is_raw, plot_type):
        """
         :return: a list of workspace names to plot
         """
        workspace_list = self.get_workspaces_to_plot(is_raw)
        indices = self._generate_run_indices(workspace_list, plot_type)

        return workspace_list, indices
