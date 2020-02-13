# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput
from Muon.GUI.Common import thread_model
import functools


class SeqFittingTabPresenter(object):

    def __init__(self, view, model, context):
        self.view = view
        self.model = model
        self.context = context

        self.fit_function = None
        self.selected_row = -1
        self.calculation_thread = None
        self.fitting_calculation_model = None

        # Observers
        self.selected_workspaces_observer = GenericObserver(self.handle_selected_workspaces_changed)
        self.fit_type_changed_observer = GenericObserver(self.handle_selected_workspaces_changed)
        self.fit_function_updated_observer = GenericObserver(self.handle_fit_function_updated)
        self.fit_parameter_updated_observer = GenericObserver(self.handle_fit_function_parameter_changed)
        self.fit_parameter_changed_in_view = GenericObserverWithArgPassing(self.handle_updated_fit_parameter_in_table)

    def create_thread(self, callback):
        self.fitting_calculation_model = ThreadModelWrapperWithOutput(callback)
        return thread_model.ThreadModel(self.fitting_calculation_model)

    def handle_fit_function_updated(self):
        if self.model.fit_function is None:
            self.model.clear_fit_information()
            self.view.set_fit_table_function_parameters([], None)
            self.view.set_fit_quality_to_default()
            return

        self.model.clear_fit_information()
        parameter_values = []
        number_of_parameters = self.model.fit_function.nParams()
        parameters = [self.model.fit_function.parameterName(i) for i in range(number_of_parameters)]
        # get parameters for each fit
        for i in range(self.view.get_number_of_entries()):
            ws_names = self.get_workspaces_for_entry_in_fit_table(i)
            fit_function = self.model.get_ws_fit_function(ws_names)
            parameter_values.append([fit_function.getParameterValue(parameters[i]) for i in
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

    def handle_single_fit_requested(self):
        if self.model.fit_function is None or self.view.get_selected_row() == -1:
            return

        self.selected_row = self.view.get_selected_row()
        workspace_names = self.get_workspaces_for_entry_in_fit_table(self.selected_row)

        calculation_function = functools.partial(
            self.model.evaluate_single_fit, workspace_names, self.view.is_plotting_checked())
        self.calculation_thread = self.create_thread(
            calculation_function)

        self.calculation_thread.threadWrapperSetUp(on_thread_start_callback=self.handle_fit_started,
                                                   on_thread_end_callback=self.handle_single_fit_finished,
                                                   on_thread_exception_callback=self.handle_fit_error)

        self.calculation_thread.start()

    def handle_fit_started(self):
        self.view.seq_fit_button.setEnabled(False)
        self.view.fit_selected_button.setEnabled(False)

    def handle_fit_error(self, error):
        self.view.warning_popup(error)
        self.view.fit_selected_button.setEnabled(True)
        self.view.seq_fit_button.setEnabled(True)

    def handle_single_fit_finished(self):
        if self.fitting_calculation_model.result is None:
            return

        fit_function, fit_status, fit_chi_squared = self.fitting_calculation_model.result
        number_of_parameters = fit_function.nParams()
        parameters = [fit_function.parameterName(i) for i in range(number_of_parameters)]
        parameter_values = [fit_function.getParameterValue(parameters[i]) for i in
                            range(number_of_parameters)]
        self.view.set_fit_function_parameters(self.selected_row, parameter_values)
        self.view.set_fit_quality(self.selected_row, fit_status, fit_chi_squared)
        self.view.seq_fit_button.setEnabled(True)
        self.view.fit_selected_button.setEnabled(True)

    def handle_sequential_fit_requested(self):
        if self.model.fit_function is None:
            return

        workspace_names = []
        for i in range(self.view.get_number_of_entries()):
            workspace_names += [self.get_workspaces_for_entry_in_fit_table(i)]

        calculation_function = functools.partial(
            self.model.evaluate_sequential_fit, workspace_names, self.view.is_plotting_checked(),
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
        count = 0
        for fit_function, fit_status, fit_chi_squared in zip(fit_functions, fit_statuses, fit_chi_squareds):
            number_of_parameters = fit_function.nParams()
            parameters = [fit_function.parameterName(i) for i in range(number_of_parameters)]
            parameter_values = [fit_function.getParameterValue(parameters[i]) for i in
                                range(number_of_parameters)]
            self.view.set_fit_function_parameters(count, parameter_values)
            self.view.set_fit_quality(count, fit_status, fit_chi_squared)
            count += 1
        self.view.seq_fit_button.setEnabled(True)
        self.view.fit_selected_button.setEnabled(True)

    def handle_updated_fit_parameter_in_table(self, row, column):
        # make sure its a parameter we changed
        if column < 3:
            return
        workspaces = self.get_workspaces_for_entry_in_fit_table(row)
        params = self.view.get_entry_fit_parameter_values(row)
        self.model.update_ws_fit_function_parameters(workspaces, params)

    def get_workspaces_for_entry_in_fit_table(self, entry):
        runs, group_and_pairs = self.view.get_workspace_info_from_fit_table_row(entry)
        separated_runs = runs.split(';')
        separated_group_and_pairs = group_and_pairs.split(';')
        workspace_names = self.model.get_fit_workspace_names_from_groups_and_runs(separated_runs,
                                                                                  separated_group_and_pairs)
        return workspace_names
