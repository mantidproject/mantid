from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget

class HomeGroupingWidgetPresenter(HomeTabSubWidget):

    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.on_grouppair_selection_changed(self.handle_grouppair_selector_changed)

    def show(self):
        self._view.show()

    def update_group_pair_list(self):
        group_names = self._model.get_group_names()
        pair_names = self._model.get_pair_names()
        self._view.populate_group_pair_selector(group_names, pair_names)

    def hide_multiperiod_widget_if_data_single_period(self):
        if self._model.is_data_multi_period():
            self._view.multi_period_widget_hidden(False)
        else:
            self._view.multi_period_widget_hidden(True)

    def handle_grouppair_selector_changed(self):
        name = self._view.get_selected_group_or_pair_name()
        if self._model.is_group(name):
            self._view.alpha_hidden(True)
        elif self._model.is_pair(name):
            self._view.alpha_hidden(False)
        else:
            self._view.alpha_hidden(True)

    def update_view_from_model(self):
        self.update_group_pair_list()
        self.hide_multiperiod_widget_if_data_single_period()