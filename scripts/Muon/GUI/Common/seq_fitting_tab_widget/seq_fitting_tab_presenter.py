# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import FitPlotInformation
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing, GenericObservable
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput
from Muon.GUI.Common import thread_model

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

        self.view.set_data_type_options(self.context.data_type_options_for_sequential())

        self.fit_parameter_changed_notifier = GenericObservable()
        self.sequential_fit_finished_notifier = GenericObservable()

        self.view.set_slot_for_display_data_type_changed(self.handle_selected_workspaces_changed)

        # Observers
        self.selected_workspaces_observer = GenericObserver(self.handle_selected_workspaces_changed)
        self.fit_type_changed_observer = GenericObserver(self.handle_selected_workspaces_changed)
        self.fit_function_updated_observer = GenericObserver(self.handle_fit_function_updated)
        self.fit_parameter_updated_observer = GenericObserver(self.handle_fit_function_parameter_changed)
        self.fit_parameter_changed_in_view = GenericObserverWithArgPassing(self.handle_updated_fit_parameter_in_table)
        self.selected_sequential_fit_notifier = GenericObservable()
        self.disable_tab_observer = GenericObserver(lambda: self.view.
                                                    setEnabled(False))
        self.enable_tab_observer = GenericObserver(lambda: self.view.
                                                   setEnabled(True))

    def create_thread(self, callback):
        self.fitting_calculation_model = ThreadModelWrapperWithOutput(callback)
        return thread_model.ThreadModel(self.fitting_calculation_model)

    def handle_fit_function_updated(self):
        parameters = self.model.get_fit_function_parameters()

        if not parameters:
            self.view.fit_table.clear_fit_parameters()
            self.view.fit_table.reset_fit_quality()
        else:
            parameter_values = self._get_fit_function_parameter_values_from_fitting_model()
            self.view.fit_table.set_parameters_and_values(parameters, parameter_values)

    def _get_fit_function_parameter_values_from_fitting_model(self):
        display_type = self.view.selected_data_type()

        parameter_values = [self.model.get_all_fit_function_parameter_values_for(fit_function)
                            for row, fit_function in enumerate(self.model.get_all_fit_functions_for(display_type))]
        if len(parameter_values) != self.view.fit_table.get_number_of_fits():
            parameter_values *= self.view.fit_table.get_number_of_fits()
        return parameter_values

    def handle_fit_function_parameter_changed(self):
        self.view.fit_table.reset_fit_quality()
        for row, fit_function in enumerate(self.model.get_all_fit_functions()):
            parameter_values = self.model.get_all_fit_function_parameter_values_for(fit_function)
            self.view.fit_table.set_parameter_values_for_row(row, parameter_values)

    def handle_selected_workspaces_changed(self):
        display_type = self.view.selected_data_type()

        workspace_names, runs, groups_and_pairs = self.model.get_runs_groups_and_pairs_for_fits(display_type)
        self.view.fit_table.set_fit_workspaces(workspace_names, runs, groups_and_pairs)

        self.handle_fit_function_updated()

    def handle_fit_selected_pressed(self):
        self.selected_rows = self.view.fit_table.get_selected_rows()
        self.handle_sequential_fit_requested()

    def handle_sequential_fit_pressed(self):
        self.view.fit_table.clear_fit_selection()
        self.selected_rows = [i for i in range(self.view.fit_table.get_number_of_fits())]
        self.handle_sequential_fit_requested()

    def handle_fit_started(self):
        self.view.seq_fit_button.setEnabled(False)
        self.view.fit_selected_button.setEnabled(False)
        self.view.fit_table.block_signals(True)

    def handle_fit_error(self, error):
        self.view.warning_popup(error)
        self.view.fit_selected_button.setEnabled(True)
        self.view.seq_fit_button.setEnabled(True)
        self.view.fit_table.block_signals(False)

    def handle_sequential_fit_requested(self):
        workspace_names = [self.get_workspaces_for_row_in_fit_table(row) for row in self.selected_rows]

        if not self.validate_sequential_fit(workspace_names):
            return

        parameter_values = [self.view.fit_table.get_fit_parameter_values_from_row(row) for row in self.selected_rows]

        calculation_function = functools.partial(self.model.perform_sequential_fit, workspace_names, parameter_values,
                                                 self.view.use_initial_values_for_fits())
        self.calculation_thread = self.create_thread(calculation_function)

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
            parameter_values = self.model.get_all_fit_function_parameter_values_for(fit_function)
            self.view.fit_table.set_parameter_values_for_row(row, parameter_values)
            self.view.fit_table.set_fit_quality(row, fit_status, fit_chi_squared)

        self.view.seq_fit_button.setEnabled(True)
        self.view.fit_selected_button.setEnabled(True)
        self.view.fit_table.block_signals(False)

        # if no row is selected (select the last)
        if len(self.view.fit_table.get_selected_rows()) == 0:
            self.view.fit_table.set_selection_to_last_row()
        else:
            self.handle_fit_selected_in_table()

        self.sequential_fit_finished_notifier.notify_subscribers()

    def handle_updated_fit_parameter_in_table(self, index):
        copy_param = self.view.copy_values_for_fits()
        if copy_param:
            self.view.fit_table.set_parameter_values_for_column(index.column(), index.data())
            self._update_parameter_values_in_fitting_model_for_all_rows(self.view.fit_table.get_number_of_fits())
        else:
            self._update_parameter_values_in_fitting_model_for_row(index.row())
        self.fit_parameter_changed_notifier.notify_subscribers()

    def _update_parameter_values_in_fitting_model_for_all_rows(self, num_of_rows):
        for row in range(num_of_rows):
            self._update_parameter_values_in_fitting_model_for_row(row)

    def validate_sequential_fit(self, workspace_names):
        message = self.model.validate_sequential_fit(workspace_names)
        if message != "":
            self.view.warning_popup(message)
        return message == ""

    @staticmethod
    def _flatten_workspace_names(workspaces: list) -> list:
        return [workspace for fit_workspaces in workspaces for workspace in fit_workspaces]

    def _update_parameter_values_in_fitting_model_for_row(self, row):
        workspaces = self.get_workspaces_for_row_in_fit_table(row)
        parameter_values = self.view.fit_table.get_fit_parameter_values_from_row(row)
        self.model.update_ws_fit_function_parameters(workspaces, parameter_values)

    def handle_fit_selected_in_table(self):
        rows = self.view.fit_table.get_selected_rows()
        fit_information = []
        for i, row in enumerate(rows):
            workspaces = self.get_workspaces_for_row_in_fit_table(row)
            fit = self.context.fitting_context.find_fit_for_input_workspace_list_and_function(
                workspaces, self.model.function_name)
            fit_information += [FitPlotInformation(input_workspaces=workspaces, fit=fit)]

        self.selected_sequential_fit_notifier.notify_subscribers(fit_information)

    def get_workspaces_for_row_in_fit_table(self, row):
        return self.view.fit_table.get_workspace_names_from_row(row).split("/")
