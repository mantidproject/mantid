# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantidqt.utils.qt import load_ui
from qtpy import QtCore, QtWidgets

from Muon.GUI.Common.list_selector.list_selector_presenter import ListSelectorPresenter
from Muon.GUI.Common.list_selector.list_selector_view import ListSelectorView

# Constants
FIT_SELECTOR_COL0_WIDTH = 600
# from_row, from_col, row_span, col_span
FIT_SELECTOR_GRID_POS = (0, 1, 4, 1)
LOG_SELECTOR_COL0_WIDTH = FIT_SELECTOR_COL0_WIDTH
LOG_SELECTOR_GRID_POS = FIT_SELECTOR_GRID_POS

ui_fitting_tab, _ = load_ui(__file__, "results_tab_view.ui")


class ResultsTabView(QtWidgets.QWidget, ui_fitting_tab):
    # Public signals
    function_selection_changed = QtCore.Signal()
    results_name_edited = QtCore.Signal()
    output_results_requested = QtCore.Signal()

    def __init__(self, parent=None):
        super(ResultsTabView, self).__init__(parent)
        self._init_layout()
        self._init_signals()

    def set_output_results_button_enabled(self, on):
        """
        Set the status of the output results button
        :param on: If True then enable the button otherwise disable it
        """
        self.output_results_table_btn.setEnabled(on)

    def results_table_name(self):
        """Return the name of the output table."""
        return self.results_name_editor.text()

    def set_results_table_name(self, name):
        """Set the name of the output table.

        :param name: A new name for the table
        """
        self.results_name_editor.setText(name)

    def set_fit_function_names(self, names):
        """Set a new list of function names for the function selector. This blocks
        signals from the widget during the update

        :param names: A list of strings specifying function names used in known fits
        """
        function_selector = self.fit_function_selector
        original_selection = function_selector.currentText()
        function_selector.blockSignals(True)
        function_selector.clear()
        function_selector.addItems(names)
        if original_selection:
            function_selector.setCurrentText(original_selection)
        function_selector.blockSignals(False)

    def selected_result_workspaces(self):
        """
        :return: The list of selected workspaces and their positions in the list
        """
        return self.fit_selector_presenter.get_selected_items_and_positions()

    def fit_result_workspaces(self):
        """
        :return: The current state of the workspace list selector
        """
        return self.fit_selector_presenter.model

    def set_fit_result_workspaces(self, workspace_list_state):
        """Set the map of workspaces

        :param workspace_list_state: Dictionary containing the updated
        state for the workspace list selector
        """
        self.fit_selector_presenter.update_model(workspace_list_state)

    def selected_fit_function(self):
        """Return the text of the selected item in the function
        selection box"""
        return self.fit_function_selector.currentText()

    def selected_log_values(self):
        """
        :return: A list of selected log values and their positions in the list
        """
        return self.log_selector_presenter.get_selected_items()

    def log_values(self):
        """
        :return: The current state of the log list selector
        """
        return self.log_selector_presenter.model

    def set_log_values(self, logs_list_state):
        """Set the map of log values and selected status

        :param logs_list_state: Dictionary containing the updated
        state for the workspace list selector
        """
        self.log_selector_presenter.update_model(logs_list_state)

    # Private methods
    def _init_layout(self):
        """Setup the layout of the view"""
        self.setupUi(self)

        self.log_selector_presenter = _create_empty_list_selector(
            self, LOG_SELECTOR_COL0_WIDTH)
        self.log_value_layout.addWidget(self.log_selector_presenter.view,
                                        *FIT_SELECTOR_GRID_POS)
        self.fit_selector_presenter = _create_empty_list_selector(
            self, FIT_SELECTOR_COL0_WIDTH)
        self.fit_layout.addWidget(self.fit_selector_presenter.view,
                                  *LOG_SELECTOR_GRID_POS)

    def _init_signals(self):
        """Connect internal signals to external notifiers"""
        self.fit_function_selector.currentIndexChanged.connect(
            self.function_selection_changed)
        self.results_name_editor.editingFinished.connect(
            self.results_name_edited)
        self.output_results_table_btn.clicked.connect(
            self.output_results_requested)

    # def update_fit_function_list(self, fit_function_list):
    #     name = self.fit_function_combo.currentText()
    #     self.fit_function_combo.clear()
    #
    #     self.fit_function_combo.addItems(fit_function_list)
    #
    #     index = self.fit_function_combo.findText(name)
    #
    #     if index != -1:
    #         self.fit_function_combo.setCurrentIndex(index)
    #     else:
    #         self.fit_function_combo.setCurrentIndex(0)
    #
    # def update_fit_selector_model(self, new_model_dictionary):
    #     self.fit_selector_presenter.update_model(new_model_dictionary)
    #
    # def update_log_selector_model(self, new_model_dictionary):
    #     self.log_selector_presenter.update_model(new_model_dictionary)
    #
    # def get_selected_fit_list(self):
    #     return self.fit_selector_presenter.get_selected_items()
    #
    # def get_selected_logs_list(self):
    #     return self.log_selector_presenter.get_selected_items()


# Private helper functions
def _create_empty_list_selector(parent, col_zero_width):
    """
    Create a ListSelector around the given parent
    and return the presenter

    :param parent: The parent widget for the selector
    :param col_zero_width: The width of the first column
    :return: A new presenter that controls the widget
    """
    presenter = ListSelectorPresenter(ListSelectorView(parent), {})
    presenter.view.item_table_widget.setColumnWidth(0, col_zero_width)
    return presenter
