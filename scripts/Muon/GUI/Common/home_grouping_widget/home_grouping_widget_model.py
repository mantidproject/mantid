from __future__ import (absolute_import, division, print_function)

# import mantid.simpleapi as mantid

from Muon.GUI.Common.muon_context import MuonContext
from mantid.api import WorkspaceGroup


class HomeGroupingWidgetModel(object):

    def __init__(self, muon_data=MuonContext()):
        self._data = muon_data

    def get_group_names(self):
        return self._data._groups.keys()

    def get_pair_names(self):
        return self._data._pairs.keys()

    def is_data_multi_period(self):
        return isinstance(self._data.loaded_data["OutputWorkspace"], list)

    def is_group(self, name):
        return name in self.get_group_names()

    def is_pair(self, name):
        return name in self.get_pair_names()

    def update_pair_alpha(self, pair_name, alpha):
        pair = self._data._pairs.get(pair_name, None)
        if pair:
            pair.alpha = alpha

    def get_alpha(self, pair_name):
        pair = self._data._pairs.get(pair_name, None)
        if pair:
            return pair.alpha

    def update_summed_periods(self, summed_periods):
        self._data.loaded_data["SummedPeriods"] = summed_periods

    def update_subtracted_periods(self, subtracted_periods):
        self._data.loaded_data["SubtractedPeriods"] = subtracted_periods

    def number_of_periods(self):
        if self.is_data_multi_period():
            return len(self._data.loaded_data["OutputWorkspace"])
        else:
            return 1
