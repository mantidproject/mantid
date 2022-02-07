# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import QMetaObject, QObject, Slot, Q_ARG
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserver, GenericObserverWithArgPassing


# Ordinarily this does not need to be a QObject but it is required to use some QMetaObject.invokeMethod
# magic. See on_new_fit_performed() for more details
class ResultsTabPresenter(QObject):
    """Controller for the results tab"""

    def __init__(self, view, model):
        super(ResultsTabPresenter, self).__init__()
        self.view = view
        self.model = model

        self.new_fit_performed_observer = GenericObserverWithArgPassing(
            self.on_new_fit_performed)

        self.update_view_from_model_observer = GenericObserver(self.update_view_from_model)

        self._init_view()

        self.disable_tab_observer = GenericObserver(lambda: self.view.
                                                    setEnabled(False))
        self.enable_tab_observer = GenericObserver(lambda: self.view.
                                                   setEnabled(True))

        self.results_table_created_notifier = GenericObservable()
        self.view.set_output_results_button_no_warning()

    # callbacks
    def on_results_table_name_edited(self):
        """React to the results table name being edited"""
        name = self.view.results_table_name()
        self.model.set_results_table_name(name)
        # add a check for if it exists
        if check_if_workspace_exist(name):
            self.view.set_output_results_button_warning()
        else:
            self.view.set_output_results_button_no_warning()

    def on_new_fit_performed(self, fit_info=None):
        """React to a new fit created in the fitting tab"""
        # It's possible that this call can come in on a thread that
        # is different to the one that the view lives on.
        # In order to update the GUI we use invokeMethod with the assumption
        # that 'self' lives on the same thread as the view and Qt forces
        # the call to the chose method to be done on the thread the
        # view lives on. This avoids errors from painting on non-gui threads.

        new_fit_name = ";"
        if fit_info:
            new_fit_list = fit_info.output_workspace_names()
            if new_fit_list and len(new_fit_list)>0:
                new_fit_name = new_fit_list[0]
        QMetaObject.invokeMethod(self, "_on_new_fit_performed_impl", Q_ARG(str, new_fit_name))

    def on_output_results_request(self):
        """React to the output results table request"""
        results_selection = self.view.selected_result_workspaces()
        if not results_selection:
            return
        log_selection = self.view.selected_log_values()
        try:
            self.model.create_results_table(log_selection, results_selection)
            QMetaObject.invokeMethod(self, "_notify_results_table_created")
            self.view.set_output_results_button_warning()

        except Exception as exc:
            self.view.show_warning(str(exc))

    @Slot()
    def _notify_results_table_created(self):
        self.results_table_created_notifier.notify_subscribers(self.model.results_table_name())

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

    @Slot(str)
    def _on_new_fit_performed_impl(self, new_fit_name=""):
        """Use as part of an invokeMethod call to call across threads"""
        self.model.on_new_fit_performed()
        self.view.set_fit_function_names(self.model.fit_functions())
        self._update_fit_results_view_on_new_fit(new_fit_name)
        self._update_logs_view()
        if self.model._fit_context.all_latest_fits():
            self.view.set_output_results_button_enabled(True)
        else:
            self.view.set_output_results_button_enabled(False)

    def _get_workspace_list(self):
        fit_context = self.model._fit_context
        workspace_list = []
        fit_list = fit_context.all_latest_fits()
        if len(fit_list) == 0:
            return workspace_list, ''
        for ii in range(1, len(fit_list) + 1):
            workspace_list.append(fit_list[-ii].parameter_workspace_name)
        return workspace_list, fit_list[len(fit_list) - 1].fit_function_name

    def _update_fit_results_view_on_new_fit(self, new_fit_name):
        """Update the view of results workspaces based on the current model"""
        workspace_list, function_name = self._get_workspace_list()
        self.view.set_selected_fit_function(function_name)
        current_view = self.view.fit_result_workspaces()

        def _check(name, new_fit_name, state):
            trim_name = name.rsplit(";",1)[0]
            trim_new_name = new_fit_name.rsplit(";",1)[0]
            return (trim_name != trim_new_name and not state)

        not_selected = [key for key in current_view.keys() if _check(key, new_fit_name, current_view[key][1])]
        workspace_list = [ws for ws in workspace_list if ws not in not_selected]
        selection = self.model.fit_selection(workspace_list)
        self.view.set_fit_result_workspaces(selection)

    def _update_fit_results_view(self):
        """Update the view of results workspaces based on the current model"""
        workspace_list, _ = self._get_workspace_list()
        selection = self.model.fit_selection(workspace_list)
        self.view.set_fit_result_workspaces(selection)

    def _update_logs_view(self):
        """Update the view of logs based on the current model"""
        self.view.set_log_values(
            self.model.log_selection(
                existing_selection=self.view.log_values()))

    def update_view_from_model(self):
        self.on_new_fit_performed()
