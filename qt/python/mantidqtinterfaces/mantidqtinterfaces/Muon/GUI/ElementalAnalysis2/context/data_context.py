# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List
from mantidqtinterfaces.Muon.GUI.Common.muon_load_data import MuonLoadData
import mantidqtinterfaces.Muon.GUI.Common.utilities.load_utils as load_utils


class DataContext(object):
    def __init__(self, load_data=MuonLoadData()):
        self.instrument = "rooth"
        self.current_runs = []
        self._loaded_data = load_data
        self._run_info = []
        self.previous_runs = []

    @property
    def run_info(self):
        return self._run_info

    def run_info_update(self, run_object):
        self._run_info.append(run_object)

    def clear_run_info(self):
        self._run_info = []

    @property
    def num_detectors(self):
        try:
            n_det = self.current_workspace.detectorInfo().size()
        except AttributeError:
            # default to 1
            n_det = 1
        if n_det == 0:
            # Number of detectors is number of workspace in group
            return self.current_workspace.size()
        return n_det

    def is_data_loaded(self):
        return self._loaded_data.num_items() > 0

    def check_group_contains_valid_detectors(self, group):
        return max(group.detectors) <= self.num_detectors and min(group.detectors) >= 1

    @property
    def _current_data(self):
        loaded_data = {}
        if self.current_runs:
            loaded_data = self._loaded_data.get_data(run=self.current_runs[0])

        return loaded_data or {"workspace": load_utils.empty_loaded_data(), "run": []}

    @property
    def current_data(self):
        return self._current_data["workspace"]

    @property
    def current_workspace(self):
        return self.current_data["OutputWorkspace"][0].workspace

    def get_loaded_data_for_run(self, run):
        loaded_dict = self._loaded_data.get_data(run=run, instrument=self.instrument)
        if loaded_dict:
            return self._loaded_data.get_data(run=run, instrument=self.instrument)["workspace"]
        else:
            return None

    def clear(self):
        self._loaded_data.clear()
        self.current_runs = []

    def remove_workspace_by_name(self, workspace_name):
        runs_removed = self._loaded_data.remove_workspace_by_name(workspace_name, self.instrument)

        self.current_runs = [item for item in self.current_runs if item not in runs_removed]


class RunObject(object):
    def __init__(self, run, detectors, groupworkspace):
        self._run_number: int = run
        self._detectors: List[str] = detectors
        self._groupworkspace = groupworkspace
