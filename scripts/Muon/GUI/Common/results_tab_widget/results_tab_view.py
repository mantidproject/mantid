# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets
from mantidqt.utils.qt import load_ui
from Muon.GUI.Common.list_selector.list_selector_presenter import ListSelectorPresenter
from Muon.GUI.Common.list_selector.list_selector_view import ListSelectorView

ui_fitting_tab, _ = load_ui(__file__, "results_tab_view.ui")


class ResultsTabView(QtWidgets.QWidget, ui_fitting_tab):
    def __init__(self, parent=None):
        super(ResultsTabView, self).__init__(parent)
        self.setupUi(self)

        self.log_selector_widget = ListSelectorView(self)
        self.fit_selector_widget = ListSelectorView(self)

        self.log_value_layout.addWidget(self.log_selector_widget, 0, 1, 4, 1)
        self.log_value_layout.setContentsMargins(0, 0, 0, 0)
        self.fit_layout.addWidget(self.fit_selector_widget, 0, 1, 4, 1)
        self.fit_layout.setContentsMargins(0, 0, 0, 0)
        self.log_selector_widget.item_table_widget.setColumnWidth(0, 600)
        self.fit_selector_widget.item_table_widget.setColumnWidth(0, 600)

        self.log_selector_presenter = ListSelectorPresenter(self.log_selector_widget, {})
        self.fit_selector_presenter = ListSelectorPresenter(self.fit_selector_widget, {})

    def update_fit_function_list(self, fit_function_list):
        name = self.fit_function_combo.currentText()
        self.fit_function_combo.clear()

        self.fit_function_combo.addItems(fit_function_list)

        index = self.fit_function_combo.findText(name)

        if index != -1:
            self.fit_function_combo.setCurrentIndex(index)
        else:
            self.fit_function_combo.setCurrentIndex(0)

    @property
    def fit_function(self):
        return self.fit_function_combo.currentText()

    def update_fit_selector_model(self, new_model_dictionary):
        self.fit_selector_presenter.update_model(new_model_dictionary)

    def update_log_selector_model(self, new_model_dictionary):
        self.log_selector_presenter.update_model(new_model_dictionary)

    def get_selected_fit_list(self):
        return self.fit_selector_presenter.get_selected_items()

    def get_selected_logs_list(self):
        return self.log_selector_presenter.get_selected_items()
