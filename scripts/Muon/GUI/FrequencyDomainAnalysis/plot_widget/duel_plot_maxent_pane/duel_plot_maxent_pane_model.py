# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plot_widget.base_pane.base_pane_model import BasePaneModel
from Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws
from Muon.GUI.Common.ADSHandler.workspace_naming import (MAXENT_STR,
                                                         get_group_or_pair_from_name,
                                                         get_run_numbers_as_string_from_workspace_name,
                                                         RECONSTRUCTED_SPECTRA)


class DuelPlotMaxentPaneModel(BasePaneModel):

    def __init__(self, context, time_group_model):
        super().__init__(context, "Maxent Duel plot")

        end_x = self.context.default_end_x
        self._time_group_model = time_group_model
        self.context.plot_panes_context[self.name].set_defaults([0.,end_x], [0.0, 1.0])
        self.reconstructed_data = {}
        self.reconstructed_data_name = ""

    def _create_workspace_label(self, workspace_name, index):
        group = str(get_group_or_pair_from_name(workspace_name))
        run = str(get_run_numbers_as_string_from_workspace_name(workspace_name, self.context.data_context.instrument))
        instrument = self.context.data_context.instrument
        freq_label = self._get_freq_label(workspace_name)
        if RECONSTRUCTED_SPECTRA in workspace_name:
            group = self.reconstructed_data[index]
            return f"{group}{freq_label}"+RECONSTRUCTED_SPECTRA
        if not self.context.plot_panes_context[self.name].settings._is_tiled:
            return f"{instrument}{run};{group}{freq_label}"
        else:
            return f"{group}{freq_label}"

    def get_workspace_list_and_indices_to_plot(self, is_groups):
        workspace_list, indicies = [], []
        if is_groups:
            workspace_list, indicies = self._time_group_model.get_workspace_list_and_indices_to_plot(True, "Counts")
        return workspace_list, indicies

    @staticmethod
    def _get_freq_label(workspace_name):
        label = ''
        if MAXENT_STR in workspace_name:
            label = f';{MAXENT_STR}'
        return label

    def create_tiled_keys(self, tiled_by):
        keys=[]
        keys = ["Maxent"]+ self.context.group_pair_context.selected_groups_and_pairs
        return keys

    def _get_workspace_plot_axis(self, workspace_name: str, axes_workspace_map, index = 0):
        if not self.context.plot_panes_context[self.name].settings._is_tiled:
            return 0
        if MAXENT_STR in workspace_name and RECONSTRUCTED_SPECTRA in workspace_name:
            group = self.reconstructed_data[index]
            return axes_workspace_map[group] # index is the group name
        if MAXENT_STR in workspace_name:
            return axes_workspace_map["Maxent"]
        group_pair_name, run_as_string = self.context.group_pair_context.get_group_pair_name_and_run_from_workspace_name(workspace_name)

        if group_pair_name in axes_workspace_map:
            return axes_workspace_map[group_pair_name]

        if run_as_string in axes_workspace_map:
            return axes_workspace_map[run_as_string]

        return 0

    def clear_reconstructed_data(self):
        self.reconstructed_data.clear()
        self.reconstructed_data_name = ""

    def set_reconstructed_data(self, ws, table_name):
        # get the indicies and groups first
        table = retrieve_ws(table_name)
        self.reconstructed_data_name = ws
        for index in range(table.rowCount()):
            data = table.row(index)
            self.reconstructed_data[index] = data["Group"]

    def add_reconstructed_data(self, workspaces, indicies):
        for key in self.reconstructed_data.keys():
            workspaces += [self.reconstructed_data_name]
            indicies += [key]
        return workspaces, indicies
