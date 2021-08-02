# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
from Muon.GUI.Common.plot_widget.base_pane.base_pane_model import BasePaneModel
from Muon.GUI.Common.ADSHandler.workspace_naming import (get_group_or_pair_from_name, REBIN_STR,
                                                         get_run_numbers_as_string_from_workspace_name)


class PlotGroupPairModel(BasePaneModel):

    def __init__(self, context, name):
        super().__init__(context, name)

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
        if not self.context.plot_panes_context[self.name].settings._is_tiled:
            return f"{instrument}{run};{group}{rebin_label}"
        if self.context.plot_panes_context[self.name].settings.is_tiled_by == "Group/Pair":
            return f"{run}{rebin_label}"
        else:
            return f"{group}{rebin_label}"

    def _get_workspace_plot_axis(self, workspace_name: str, axes_workspace_map, index = 0):
        if not self.context.plot_panes_context[self.name].settings._is_tiled:
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
            return ';' + REBIN_STR
        else:
            return ''
