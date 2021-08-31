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
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string, run_string_to_list
from math import floor, ceil


class RawPaneModel(BasePaneModel):

    def __init__(self, context):
        super().__init__(context,"Raw Data")
        self.context.plot_panes_context[self.name].set_defaults([0., 9.9], [0., 1e3])
        self.context.plot_panes_context[self.name].settings.set_condensed(True)
        self._max_spec = 16
        self._spec_limit = 16

    def get_num_detectors(self):
        return self.context.data_context.num_detectors

    def _get_first_and_last_detector_to_plot(self, detectors):
        if detectors=="":
            return 0,0
        # -1 to convert to ws index
        lower = int(detectors.split(':')[0])-1
        # also add one to allow range to include it
        upper = int(detectors.split(':')[1])
        return lower, upper

    def get_ws_names(self, run_string, multi_period, period, detectors:str):
        name = get_raw_data_workspace_name(self.context.data_context.instrument, run_string, multi_period,
                                                       period=period, workspace_suffix=self.context.workspace_suffix)
        lower, upper = self._get_first_and_last_detector_to_plot(detectors)
        return [name for _ in range(lower, upper)]

    def check_num_detectors(self):
        num_detectors = self.get_num_detectors()
        if self._max_spec > num_detectors:
            self._max_spec = num_detectors
        elif self._max_spec < num_detectors:
            self._max_spec = self._spec_limit

    def get_workspaces_to_plot(self, is_raw, plot_type, detectors:str, run_string, period=None):
        """
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: plotting type, e.g Counts, Frequency Re
        :return: a list of workspace names
        """
        workspace_list = []
        run = run_string_to_list(run_string)
        # make sure the run is in the context
        if run not in self.context.data_context.current_runs:
            return workspace_list
        multi_period = False
        if self.context.data_context.num_periods(run) >1:
            multi_period = True
            if period:
                workspace_list += self.get_ws_names(run_string, multi_period, str(period),detectors)
            else:
                for period_i in range(self.context.data_context.num_periods(run)):
                    # periods start at 1 and not 0
                    workspace_list += self.get_ws_names(run_string, multi_period, str(period_i+1),detectors)
        else:
            workspace_list += self.get_ws_names(run_string, multi_period, '1',detectors)
        return workspace_list

    def _generate_run_indices(self, workspace_list, detectors):
        lower, upper = self._get_first_and_last_detector_to_plot(detectors)
        indices = []
        if upper == lower:
            return indices
        # workspace list repeats value multiple times
        # step size only fills in unique workspaces
        for k in range(0, len(workspace_list), upper-lower):
            for spec in range(lower, upper):
                indices += [spec]
        return indices

    def get_workspace_list_and_indices_to_plot(self, is_raw, plot_type, detectors:str, run, period=None):
        """
         :return: a list of workspace names to plot
         """
        workspace_list = self.get_workspaces_to_plot(is_raw, plot_type, detectors, run, period)
        indices = self._generate_run_indices(workspace_list, detectors)
        return workspace_list, indices

    def create_tiled_keys(self, tiled_by, def_zero = 1):
        return ["Detector: "+str(spec+def_zero) for spec in range(self._max_spec)]

    def convert_index_to_axis(self, index):
        return index - floor(index/self._max_spec)*self._max_spec

    def _get_workspace_plot_axis(self, workspace_name: str, axes_workspace_map, index = 0):
        if not self.context.plot_panes_context[self.name].settings._is_tiled:
            return 0
        return self.convert_index_to_axis(index)

    def _create_workspace_label(self, workspace_name, index):
        run = get_run_number_from_raw_name(workspace_name, self.context.data_context.instrument)
        period = get_period_from_raw_name(workspace_name, self.context.workspace_suffix)

        return "Run"+run+period+"_Det"+str(index+1)

    def gen_detector_options(self):
        det_list = []
        num_detectors = self.get_num_detectors()
        num_selections = ceil(num_detectors/self._max_spec)
        for j in range(num_selections-1):
            lower = (j*self._max_spec)+1
            upper = (j+1)*self._max_spec
            det_list.append(str(lower)+":"+str(upper))
        if num_selections > 1:
            lower = ((num_selections-1)*self._max_spec)+1
        else:
            lower = 1
        det_list.append(str(lower)+":"+str(num_detectors))
        return det_list

    def gen_run_list(self):
        run_list = []
        for run in self.context.data_context.current_runs:
            run_list.append(run_list_to_string(run))
        return run_list
