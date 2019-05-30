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
        self.view.set_fit_result_workspaces(
            self.model.fit_selection(
                existing_selection=self.view.fit_result_workspaces()))
        self.view.set_log_values(
            self.model.log_selection(
                existing_selection=self.view.log_values()))
        self.view.set_output_results_button_enabled(True)

    def on_output_results_request(self):
        """React to the output results table request"""
        results_selection = self.view.selected_result_workspaces()
        if not results_selection:
            return
        log_selection = self.view.selected_log_values()
        self.model.create_results_table(log_selection, results_selection)

    # private api
    def _init_view(self):
        """Perform any setup for the view that is related to the model"""
        self.view.set_results_table_name(self.model.results_table_name())
        self.view.results_name_edited.connect(
            self.on_results_table_name_edited)
        self.view.output_results_requested.connect(
            self.on_output_results_request)
        self.view.set_output_results_button_enabled(False)

    # def handle_fit_function_changed(self):
    #     currently_selected_workspaces = self.view.get_selected_fit_list()
    #     updated_fit_list = self.get_performed_fits_by_fit_function(self.view.fit_function)
    #
    #     new_model_dictionary = {item.parameter_name: [index, item.parameter_name in currently_selected_workspaces, True]
    #                             for index, item in enumerate(updated_fit_list)}
    #
    #     self.view.update_fit_selector_model(new_model_dictionary)
    #     self.update_logs_list(updated_fit_list)
    #
