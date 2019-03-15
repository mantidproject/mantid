# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.muon_data_context import MuonDataContext


class HomeGroupingWidgetModel(object):

    def __init__(self, muon_data=MuonDataContext()):
        self._data = muon_data

    def get_group_names(self):
        return self._data.groups.keys()

    def get_pair_names(self):
        return self._data.pairs.keys()

    def is_group(self, name):
        return name in self.get_group_names()

    def is_pair(self, name):
        return name in self.get_pair_names()

    def update_pair_alpha(self, pair_name, alpha):
        try:
            self._data.pairs[pair_name].alpha = alpha
        except Exception:
            print("Exception in update_pair_alpha")

    def get_alpha(self, pair_name):
        pair = self._data.pairs[pair_name]
        if pair:
            return pair.alpha
        else:
            return None

    # ------------------------------------------------------------------------------------------------------------------
    # Periods
    # ------------------------------------------------------------------------------------------------------------------

    def is_data_multi_period(self):
        return self._data.is_multi_period()

    def number_of_periods(self):
        if self.is_data_multi_period():
            return len(self._data.current_data["OutputWorkspace"])
        else:
            return 1

    def update_periods(self, summed_periods, subtracted_periods):
        self._data.add_or_replace_gui_variables(SubtractedPeriods=subtracted_periods, SummedPeriods=summed_periods)

    def get_summed_periods(self):
        return self._data.gui_variables["SummedPeriods"]

    def get_subtracted_periods(self):
        return self._data.gui_variables["SubtractedPeriods"]
