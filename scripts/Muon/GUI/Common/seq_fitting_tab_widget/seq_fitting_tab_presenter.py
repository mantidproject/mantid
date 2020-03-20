# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput
from Muon.GUI.Common import thread_model

from collections import defaultdict
import functools


class SeqFittingTabPresenter(object):

    def __init__(self, view, model, context):
        self.view = view
        self.model = model
        self.context = context

        self.fit_function = None
        self.selected_rows = []
        self.calculation_thread = None
        self.fitting_calculation_model = None

        # Observers
        self.selected_workspaces_observer = GenericObserver(self.handle_selected_workspaces_changed)
        self.fit_type_changed_observer = GenericObserver(self.handle_selected_workspaces_changed)
        self.fit_function_updated_observer = GenericObserver(self.handle_fit_function_updated)
        self.fit_parameter_updated_observer = GenericObserver(self.handle_fit_function_parameter_changed)
        self.fit_parameter_changed_in_view = GenericObserverWithArgPassing(self.handle_updated_fit_parameter_in_table)
        self.selected_sequential_fit_notifier = GenericObservable()
        self.leaving_sequential_table_notifer = GenericObservable()

    def create_thread(self, callback):
        self.fitting_calculation_model = ThreadModelWrapperWithOutput(callback)
        return thread_model.ThreadModel(self.fitting_calculation_model)

    def handle_fit_function_updated(self):
        if self.model.fit_function is None:
            self.model.clear_fit_information()
            self.view.set_fit_table_function_parameters([], None)
            self.view.set_fit_quality_to_default()
            return

        parameter_values = []
        number_of_parameters = self.model.fit_function.nParams()
        parameters = [self.model.fit_function.parameterName(i) for i in range(number_of_parameters)]
        # get parameters for each fit
        for i in range(self.view.get_number_of_entries()):
            ws_names = self.get_workspaces_for_entry_in_fit_table(i)
            fit_function = self.model.get_ws_fit_function(ws_names)
            parameter_values.append([fit_function.getParameterValue(parameters[j]) for j in
                                     range(number_of_parameters)])

        self.view.set_fit_table_function_parameters(parameters, parameter_values)

    def handle_fit_function_parameter_changed(self):
        self.model.clear_fit_information()
        self.view.set_fit_quality_to_default()

        number_of_parameters = self.model.fit_function.nParams()
        parameters = [self.model.fit_function.parameterName(i) for i in range(number_of_parameters)]
        parameter_values = [self.model.fit_function.getParameterValue(parameters[i]) for i in
                            range(number_of_parameters)]

        for i in range(self.view.get_number_of_entries()):
            self.view.set_fit_function_parameters(i, parameter_values)

    def handle_selected_workspaces_changed(self):
        runs, groups_and_pairs = self.model.get_runs_groups_and_pairs_for_fits()
        self.view.set_fit_table_workspaces(runs, groups_and_pairs)
        self.handle_fit_function_updated()

    def handle_fit_selected_pressed(self):
        self.selected_rows = self.view.get_selected_rows()
        self.handle_sequential_fit_requested()

    def handle_sequential_fit_pressed(self):
        # Clear selection in fit table
        self.view.fit_results_table.clearSelection()
        self.selected_rows = [i for i in range(self.view.get_number_of_entries())]
        self.handle_sequential_fit_requested()

    def handle_fit_started(self):
        self.view.seq_fit_button.setEnabled(False)
        self.view.fit_selected_button.setEnabled(False)
        self.view.fit_results_table.blockSignals(True)

    def handle_fit_error(self, error):
        self.view.warning_popup(error)
        self.view.fit_selected_button.setEnabled(True)
        self.view.seq_fit_button.setEnabled(True)
        self.view.fit_results_table.blockSignals(False)

    def handle_sequential_fit_requested(self):
        if self.model.fit_function is None or len(self.selected_rows) == 0:
            return

        workspace_names = []
        for i in self.selected_rows:
            workspace_names += [self.get_workspaces_for_entry_in_fit_table(i)]

        calculation_function = functools.partial(
            self.model.evaluate_sequential_fit, workspace_names, False,
            self.view.use_initial_values_for_fits())
        self.calculation_thread = self.create_thread(
            calculation_function)

        self.calculation_thread.threadWrapperSetUp(on_thread_start_callback=self.handle_fit_started,
                                                   on_thread_end_callback=self.handle_seq_fit_finished,
                                                   on_thread_exception_callback=self.handle_fit_error)
        self.calculation_thread.start()

    def handle_seq_fit_finished(self):
        if self.fitting_calculation_model.result is None:
            return

        fit_functions, fit_statuses, fit_chi_squareds = self.fitting_calculation_model.result
        for fit_function, fit_status, fit_chi_squared, row in zip(fit_functions, fit_statuses, fit_chi_squareds,
                                                                  self.selected_rows):
            number_of_parameters = fit_function.nParams()
            parameters = [fit_function.parameterName(i) for i in range(number_of_parameters)]
            parameter_values = [fit_function.getParameterValue(parameters[i]) for i in
                                range(number_of_parameters)]
            self.view.set_fit_function_parameters(row, parameter_values)
            self.view.set_fit_quality(row, fit_status, fit_chi_squared)

        self.view.seq_fit_button.setEnabled(True)
        self.view.fit_selected_button.setEnabled(True)
        self.view.fit_results_table.blockSignals(False)

        # if no row is selected (select the last)
        if len(self.view.get_selected_rows()) == 0:
            self.view.set_table_selection_to_last_row()
        else:
            self.handle_fit_selected_in_table()

    def handle_updated_fit_parameter_in_table(self, row, column):
        # make sure its a parameter we changed
        if column < 3:
            return
        workspaces = self.get_workspaces_for_entry_in_fit_table(row)
        params = self.view.get_entry_fit_parameter_values(row)
        self.model.update_ws_fit_function_parameters(workspaces, params)

    def handle_fit_selected_in_table(self):
        rows = self.view.get_selected_rows()
        workspaces = defaultdict(list)
        for i, row in enumerate(rows):
            workspaces[i] = self.get_workspaces_for_entry_in_fit_table(row)

        self.selected_sequential_fit_notifier.notify_subscribers(workspaces)

    def get_workspaces_for_entry_in_fit_table(self, entry):
        runs, group_and_pairs = self.view.get_workspace_info_from_fit_table_row(entry)
        separated_runs = runs.split(';')
        separated_group_and_pairs = group_and_pairs.split(';')
        workspace_names = self.model.get_fit_workspace_names_from_groups_and_runs(separated_runs,
                                                                                  separated_group_and_pairs)
        return workspace_names
