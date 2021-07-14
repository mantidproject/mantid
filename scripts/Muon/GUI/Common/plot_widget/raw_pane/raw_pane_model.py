# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plot_widget.base_pane.base_pane_model import BasePaneModel
from Muon.GUI.Common.ADSHandler.workspace_naming import (get_raw_data_workspace_name,
                                                         get_run_number_from_raw_name,
                                                         get_period_from_raw_name)
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string


class RawPaneModel(BasePaneModel):

    def __init__(self, context):
        super().__init__(context,"Raw Data")
        self.context.plot_panes_context[self.name].set_defaults([0., 9.9], [0., 1e3])
        self.context.plot_panes_context[self.name].settings.set_condensed(True)
        self._max_spec = 16

    def get_ws_names(self, run_string, multi_period, period):
        return [get_raw_data_workspace_name(self.context.data_context.instrument, run_string, multi_period,
                                                       period=period, workspace_suffix=self.context.workspace_suffix) for
                           _ in range(self._max_spec)]

    def get_workspaces_to_plot(self, is_raw, plot_type):
        """
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: plotting type, e.g Counts, Frequency Re
        :return: a list of workspace names
        """
        workspace_list = []

        # need to handle multi periodO
        for run in self.context.data_context.current_runs:
            run_string = run_list_to_string(run)
            multi_period = False
            if self.context._data_context.num_periods(run) >1:
                multi_period = True
                for period in range(self.context._data_context.num_periods(run)):
                    # periods start at 1 and not 0
                    workspace_list += self.get_ws_names(run_string, multi_period, str(period+1))
            else:
                workspace_list += self.get_ws_names(run_string, multi_period, '1')
        return workspace_list

    def _generate_run_indices(self, workspace_list):
        indicies = []
        for k in range(len(workspace_list)):
            for spec in range(self._max_spec):
                indicies += [spec]
        return indicies

    def get_workspace_list_and_indices_to_plot(self, is_raw, plot_type):
        """
         :return: a list of workspace names to plot
         """
        workspace_list = self.get_workspaces_to_plot(is_raw, plot_type)
        indices = self._generate_run_indices(workspace_list)

        return workspace_list, indices

    def create_tiled_keys(self, tiled_by):
        return ["Detector: "+str(spec+1) for spec in range(self._max_spec)]

    def _get_workspace_plot_axis(self, workspace_name: str, axes_workspace_map, index = 0):
        if not self.context.plot_panes_context[self.name].settings._is_tiled:
            return 0
        return index

    def _create_workspace_label(self, workspace_name, index):
        run = get_run_number_from_raw_name(workspace_name, self.context.data_context.instrument)
        period = get_period_from_raw_name(workspace_name, self.context.workspace_suffix)

        return "Run"+run+period+"_Det"+str(index+1)
