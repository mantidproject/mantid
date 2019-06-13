# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division)

from qtpy.QtCore import QMetaObject, QObject, Slot

from Muon.GUI.Common.observer_pattern import GenericObserver


# Ordinarily this does not need to be a QObject but it is required to use some QMetaObject.invokeMethod
# magic. See on_new_fit_performed() for more details
class ResultsTabPresenter(QObject):
    """Controller for the results tab"""

    def __init__(self, view, model):
        super(ResultsTabPresenter, self).__init__()
        self.view = view
        self.model = model
        self.new_fit_performed_observer = GenericObserver(
            self.on_new_fit_performed)

        self.update_view_from_model_observer = GenericObserver(self.update_view_from_model)

        self._init_view()

    # callbacks
    def on_results_table_name_edited(self):
        """React to the results table name being edited"""
        self.model.set_results_table_name(self.view.results_table_name())

    def on_new_fit_performed(self):
        """React to a new fit created in the fitting tab"""
        # It's possible that this call can come in on a thread that
        # is different to the one that the view lives on.
        # In order to update the GUI we use invokeMethod with the assumption
        # that 'self' lives on the same thread as the view and Qt forces
        # the call to the chose method to be done on the thread the
        # view lives on. This avoids errors from painting on non-gui threads.
        QMetaObject.invokeMethod(self, "_on_new_fit_performed_impl")

    def on_output_results_request(self):
        """React to the output results table request"""
        results_selection = self.view.selected_result_workspaces()
        if not results_selection:
            return
        log_selection = self.view.selected_log_values()
        try:
            self.model.create_results_table(log_selection, results_selection)
        except Exception as exc:
            self.view.show_warning(str(exc))

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

    @Slot()
    def _on_new_fit_performed_impl(self):
        """Use as part of an invokeMethod call to call across threads"""
        self.view.set_fit_function_names(self.model.fit_functions())
        self._update_fit_results_view()
        self._update_logs_view()
        self.view.set_output_results_button_enabled(True)

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

    def update_view_from_model(self):
        pass
