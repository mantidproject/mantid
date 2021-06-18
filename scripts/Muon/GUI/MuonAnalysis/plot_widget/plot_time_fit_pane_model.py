# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
from Muon.GUI.Common.plot_widget.base_pane.base_pane_model import BasePaneModel
from Muon.GUI.Common.ADSHandler.workspace_naming import *


class PlotTimeFitPaneModel(BasePaneModel):

    def __init__(self, context):
        super().__init__(context,"Fit Data")

    @staticmethod
    def get_fit_workspace_and_indices(fit, with_diff=True):
        if fit is None:
            return [], []
        workspaces = []
        indices = []
        for workspace_name in fit.output_workspace_names:
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

    def _create_workspace_label(self, workspace_name, index):
        group = str(get_group_or_pair_from_name(workspace_name))
        run = str(get_run_numbers_as_string_from_workspace_name(workspace_name, self.context.data_context.instrument))
        instrument = self.context.data_context.instrument
        fit_label = self._get_fit_label(workspace_name, index)
        rebin_label = self._get_rebin_label(workspace_name)
        freq_label = self._get_freq_lebel(workspace_name)
        if not self.context.plot_panes_context[self.name]._is_tiled:
            return "".join([instrument, run, ';', group, fit_label, rebin_label, freq_label])
        if self.context.plot_panes_context[self.name].is_tiled_by == "Group/Pair":
            return "".join([run, fit_label, rebin_label, freq_label])
        else:
            return "".join([group, fit_label, rebin_label, freq_label])

    def _get_workspace_plot_axis(self, workspace_name: str, axes_workspace_map):
        if not self.context.plot_panes_context[self.name]._is_tiled:
            return 0

        group_pair_name, run_as_string = self.context.group_pair_context.get_group_pair_name_and_run_from_workspace_name(workspace_name)

        if group_pair_name in axes_workspace_map:
            return axes_workspace_map[group_pair_name]

        if run_as_string in axes_workspace_map:
            return axes_workspace_map[run_as_string]

        return 0

    # these should not be here!!!!
    @staticmethod
    def _get_fit_label(workspace_name, index):
        label = ''
        fit_function_name = get_fit_function_name_from_workspace(workspace_name)
        if fit_function_name:
            if index in [1, 3]:
                workspace_type = 'Calc'
            elif index == 2:
                workspace_type = 'Diff'
            label = ''.join([';', fit_function_name, ';', workspace_type])
        return label

    @staticmethod
    def _get_rebin_label(workspace_name):
        if REBIN_STR in workspace_name:
            return ''.join([';', REBIN_STR])
        else:
            return ''

    @staticmethod
    def _get_freq_lebel(workspace_name):
        label = ''
        if FFT_STR in workspace_name:
            label = ''.join([';', get_fft_component_from_workspace_name(workspace_name)])
        elif MAXENT_STR in workspace_name:
            label = ''.join([';', MAXENT_STR])
        return label

    def _is_guess_workspace(self, workspace_name):
        return self.context.guess_workspace_prefix in workspace_name
