# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List
from Muon.GUI.Common.muon_load_data import MuonLoadData


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


class ElementalAnalysisContext(object):
    def __init__(self):
        self._window_title = "Elemental Analysis 2"
        self.data_context = DataContext()

    @property
    def name(self):
        return self._window_title


class RunObject(object):
    def __init__(self, run, detectors, groupworkspace):
        self._run_number: int = run
        self._detectors: List[str] = detectors
        self._groupworkspace = groupworkspace
