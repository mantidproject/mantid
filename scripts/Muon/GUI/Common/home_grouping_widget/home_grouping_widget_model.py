# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


class HomeGroupingWidgetModel(object):

    def __init__(self, context=None):
        self._data = context.data_context
        self._context = context

    def get_group_names(self):
        return self._context.group_pair_context.group_names

    def get_pair_names(self):
        return self._context.group_pair_context.pair_names

    def get_default_group_pair(self):
        return self._context.group_pair_context.selected

    def is_group(self, name):
        return name in self.get_group_names()

    def is_pair(self, name):
        return name in self.get_pair_names()

    def update_pair_alpha(self, pair_name, alpha):
        try:
            self._context.group_pair_context[pair_name].alpha = alpha
        except Exception:
            print("Exception in update_pair_alpha")

    def get_alpha(self, pair_name):
        pair = self._context.group_pair_context[pair_name]
        if pair:
            return pair.alpha
        else:
            return None

    def update_selected_group_pair_in_context(self, name):
        self._context.group_pair_context.selected = name

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
        self._context.gui_context.update_and_send_signal(SubtractedPeriods=subtracted_periods, SummedPeriods=summed_periods)

    def get_summed_periods(self):
        return self._context.gui_context["SummedPeriods"]

    def get_subtracted_periods(self):
        return self._context.gui_context["SubtractedPeriods"]
