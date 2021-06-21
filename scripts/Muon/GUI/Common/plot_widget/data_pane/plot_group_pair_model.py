# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
from Muon.GUI.Common.plot_widget.base_pane.base_pane_model import BasePaneModel
from Muon.GUI.Common.ADSHandler.workspace_naming import *


class PlotGroupPairModel(BasePaneModel):

    def __init__(self, context, name):
        super().__init__(context, name)

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

    def _create_workspace_label(self, workspace_name, index):
        group = str(get_group_or_pair_from_name(workspace_name))
        run = str(get_run_numbers_as_string_from_workspace_name(workspace_name, self.context.data_context.instrument))
        instrument = self.context.data_context.instrument
        rebin_label = self._get_rebin_label(workspace_name)
        if not self.context.plot_panes_context[self.name]._is_tiled:
            return "".join([instrument, run, ';', group, rebin_label])
        if self.context.plot_panes_context[self.name].is_tiled_by == "Group/Pair":
            return "".join([run, rebin_label])
        else:
            return "".join([group, rebin_label])

    def _get_workspace_plot_axis(self, workspace_name: str, axes_workspace_map):
        if not self.context.plot_panes_context[self.name]._is_tiled:
            return 0

        group_pair_name, run_as_string = self.context.group_pair_context.get_group_pair_name_and_run_from_workspace_name(workspace_name)

        if group_pair_name in axes_workspace_map:
            return axes_workspace_map[group_pair_name]

        if run_as_string in axes_workspace_map:
            return axes_workspace_map[run_as_string]

        return 0

    @staticmethod
    def _get_rebin_label(workspace_name):
        if REBIN_STR in workspace_name:
            return ''.join([';', REBIN_STR])
        else:
            return ''
