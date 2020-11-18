# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List
from Muon.GUI.Common.muon_load_data import MuonLoadData


class DataContext(object):
    def __init__(self, load_data=MuonLoadData()):
        self.instrument = "rooth"
        self.current_runs = []
        self._main_field_direction = ''
        self._loaded_data = load_data
        self._run_info = []

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
            # The number of histograms commonly used for PSI data, in real data,
            # very often the number of detectors == number of histograms
            return self.current_workspace.getNumberHistograms()
        return n_det

    @property
    def main_field_direction(self):
        return self._main_field_direction


class RunObject(object):
    def __init__(self, run, detectors, groupworkspace):
        self._run_number: int = run
        self._detectors: List[str] = detectors
        self._groupworkspace = groupworkspace
