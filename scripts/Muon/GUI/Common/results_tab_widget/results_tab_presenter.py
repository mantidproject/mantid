# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division)

from Muon.GUI.Common.observer_pattern import GenericObserver


class ResultsTabPresenter(object):
    """Controller for the results tab"""

    def __init__(self, view, model):
        self.view = view
        self.model = model
        self.new_fit_performed_observer = GenericObserver(
            self.on_new_fit_performed)

        self._init_view()

    # callbacks
    def on_results_table_name_edited(self):
        """React to the results table name being edited"""
        self.model.set_results_table_name(self.view.results_table_name())

    def on_new_fit_performed(self):
        """React to a new fit created in the fitting tab"""
        self.view.set_fit_function_names(self.model.fit_functions())

        self._update_fit_results_view()
        self._update_logs_view()
        self.view.set_output_results_button_enabled(True)

    def on_output_results_request(self):
        """React to the output results table request"""
        results_selection = self.view.selected_result_workspaces()
        if not results_selection:
            return
        log_selection = self.view.selected_log_values()
        self.model.create_results_table(log_selection, results_selection)

    def on_function_selection_changed(self):
        """React to the change in function selection"""
        self.model.set_selected_fit_function(self.view.selected_fit_function())
        self._update_fit_results_view()

    # private api
    def _init_view(self):
        """Perform any setup for the view that is related to the model"""
        self.view.set_results_table_name(self.model.results_table_name())
        self.view.results_name_edited.connect(
            self.on_results_table_name_edited)
        self.view.output_results_requested.connect(
            self.on_output_results_request)
        self.view.function_selection_changed.connect(
            self.on_function_selection_changed)
        self.view.set_output_results_button_enabled(False)

    def _update_fit_results_view(self):
        """Update the view of results workspaces based on the current model"""
        self.view.set_fit_result_workspaces(
            self.model.fit_selection(
                existing_selection=self.view.fit_result_workspaces()))

    def _update_logs_view(self):
        """Update the view of logs based on the current model"""
        self.view.set_log_values(
            self.model.log_selection(
                existing_selection=self.view.log_values()))
