# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget
from Muon.GUI.Common.observer_pattern import Observable, GenericObservable
from Muon.GUI.Common.utilities.run_string_utils import run_string_to_list


class HomeGroupingWidgetPresenter(HomeTabSubWidget):

    @staticmethod
    def string_to_list(text):
        # if text == "":
        #     return []
        # return [int(i) for i in text.split(",")]
        return run_string_to_list(text)

    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.on_grouppair_selection_changed(self.handle_grouppair_selector_changed)

        self._view.on_alpha_changed(self.handle_user_changes_alpha)

        self._view.on_summed_periods_changed(self.handle_periods_changed)
        self._view.on_subtracted_periods_changed(self.handle_periods_changed)

        self.pairAlphaNotifier = HomeGroupingWidgetPresenter.PairAlphaNotifier(self)
        self.selected_group_pair_changed_notifier = GenericObservable()

    def show(self):
        self._view.show()

    def handle_user_changes_alpha(self):
        alpha = self._view.get_current_alpha()
        pair_name = str(self._view.get_currently_selected_group_pair())
        self._model.update_pair_alpha(pair_name, alpha)
        # notify any observers of the change
        self.pairAlphaNotifier.notify_subscribers()

    def update_group_pair_list(self):
        group_names = self._model.get_group_names()
        pair_names = self._model.get_pair_names()
        default_name = self._model.get_default_group_pair()
        self._view.populate_group_pair_selector(group_names, pair_names, default_name)

    def hide_multiperiod_widget_if_data_single_period(self):
        if self._model.is_data_multi_period():
            self._view.multi_period_widget_hidden(False)
        else:
            self._view.multi_period_widget_hidden(True)

    def handle_grouppair_selector_changed(self):
        name = str(self._view.get_selected_group_or_pair_name())
        self._model.update_selected_group_pair_in_context(name)
        if self._model.is_group(name):
            self._view.alpha_hidden(True)
        elif self._model.is_pair(name):
            self._view.alpha_hidden(False)
            alpha = self._model.get_alpha(name)
            self._view.set_current_alpha(alpha)
        else:
            self._view.alpha_hidden(True)

        self.selected_group_pair_changed_notifier.notify_subscribers()

    def update_view_from_model(self):
        self.update_group_pair_list()
        self.hide_multiperiod_widget_if_data_single_period()

        n_periods = self._model.number_of_periods()
        self._view.set_period_number_in_period_label(n_periods)

    def update_period_edits(self):
        summed_periods = self._model.get_summed_periods()
        subtracted_periods = self._model.get_subtracted_periods()

        self._view.set_summed_periods(",".join([str(p) for p in summed_periods]))
        self._view.set_subtracted_periods(",".join([str(p) for p in subtracted_periods]))

    def handle_periods_changed(self):
        self._view.summed_period_edit.blockSignals(True)
        self._view.subtracted_period_edit.blockSignals(True)
        summed = self.string_to_list(self._view.get_summed_periods())
        subtracted = self.string_to_list(self._view.get_subtracted_periods())

        subtracted = [i for i in subtracted if i not in summed]

        n_periods = self._model.number_of_periods()
        bad_periods = [period for period in summed if (period > n_periods) or period == 0] +\
                      [period for period in subtracted if(period > n_periods) or period == 0]
        if len(bad_periods) > 0:
            self._view.warning_popup("The following periods are invalid : " + ",".join([str(period) for period in bad_periods]))

        summed = [p for p in summed if (p <= n_periods) and p > 0 and p not in bad_periods]
        if not summed:
            summed = [1]

        subtracted = [p for p in subtracted if (p <= n_periods) and p > 0 and p not in bad_periods]

        self._model.update_periods(summed, subtracted)

        self.update_period_edits()
        self._view.summed_period_edit.blockSignals(False)
        self._view.subtracted_period_edit.blockSignals(False)

    class PairAlphaNotifier(Observable):

        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer  # handle to containing class

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args, **kwargs)
