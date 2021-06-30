# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string

COUNTS_PLOT_TYPE = 'Counts'
ASYMMETRY_PLOT_TYPE = 'Asymmetry'
TILED_BY_GROUP_TYPE = 'Group/Pair'
TILED_BY_RUN_TYPE = 'Run'
FREQ_PLOT_TYPE = "Frequency "


class PlotWidgetModel(object):

    def __init__(self, context):
        self.context = context

    @property
    def tiled_by_group(self):
        return TILED_BY_GROUP_TYPE

    @property
    def tiled_by_run(self):
        return TILED_BY_RUN_TYPE

    @property
    def counts_plot(self):
        return COUNTS_PLOT_TYPE

    @property
    def asymmetry_plot(self):
        return ASYMMETRY_PLOT_TYPE

    def get_workspace_list_to_plot(self, is_raw, plot_type):
        """
         :return: a list of workspace names to plot
         """
        workspace_list = self.get_workspaces_to_plot(is_raw, plot_type)
        return workspace_list

    def get_workspace_list_and_indices_to_plot(self, is_raw, plot_type):
        """
         :return: a list of workspace names to plot
         """
        workspace_list = self.get_workspaces_to_plot(is_raw, plot_type)
        indices = self._generate_run_indices(workspace_list)

        return workspace_list, indices

    def get_workspace_and_indices_for_group_or_pair(self, group_or_pair, is_raw, plot_type):
        """
         :return: a list of workspace names and corresponding indices to plot
         """
        workspace_list = []
        if FREQ_PLOT_TYPE in plot_type:
            workspace_list += self.get_freq_workspaces_to_plot(group_or_pair, plot_type)
        else:
            workspace_list += self.get_time_workspaces_to_plot(group_or_pair, is_raw, plot_type)
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

        if FREQ_PLOT_TYPE in plot_type:
            for grouppair in currently_selected:
                workspace_list += self.get_freq_workspaces_to_plot(grouppair, plot_type)
            workspace_list = list(set(workspace_list))
            return workspace_list
        else:
            for grouppair in currently_selected:
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

    def create_tiled_keys(self, tiled_by):
        if tiled_by == TILED_BY_GROUP_TYPE:
            keys = self.context.group_pair_context.selected_groups_and_pairs
        else:
            keys = [run_list_to_string(item) for item in self.context.data_context.current_runs]
        return keys

    @staticmethod
    def get_plot_types():
        plot_types = [ASYMMETRY_PLOT_TYPE, COUNTS_PLOT_TYPE]
        return plot_types

    @staticmethod
    def get_tiled_by_types():
        return [TILED_BY_GROUP_TYPE, TILED_BY_RUN_TYPE]

    @staticmethod
    def _generate_run_indices(workspace_list):
        indices = [0 for _ in workspace_list]
        return indices

    @staticmethod
    def get_fit_workspace_and_indices(fit, with_diff=True):
        if fit is None:
            return [], []
        workspaces = []
        indices = []
        for workspace_name in fit.output_workspace_names():
            first_fit_index = 1  # calc
            if fit.tf_asymmetry_fit:
                first_fit_index = 3
            second_fit_index = 2  # Diff
            workspaces.append(workspace_name)
            indices.append(first_fit_index)
            if with_diff:
                workspaces.append(workspace_name)
                indices.append(second_fit_index)

        return workspaces, indices
