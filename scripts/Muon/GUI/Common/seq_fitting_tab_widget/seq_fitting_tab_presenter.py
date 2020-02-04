# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)
from mantidqt.utils.observer_pattern import GenericObserver
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput
from Muon.GUI.Common import thread_model
import functools


class SeqFittingTabPresenter(object):

    def __init__(self, view, model, context):
        self.view = view
        self.model = model
        self.context = context

        self.fit_function = None

        # Observers
        self.selected_workspaces_observer = GenericObserver(self.handle_selected_workspaces_changed)
        self.fit_function_changed_observer = GenericObserver(self.handle_fit_function_changed)

    def handle_fit_function_changed(self):
        if self.model.fit_function is None:
            return

        number_of_parameters = self.model.fit_function.nParams()
        parameters = [self.model.fit_function.parameterName(i) for i in range(number_of_parameters)]
        parameter_values = [self.model.fit_function.getParameterValue(parameters[i]) for i in
                            range(number_of_parameters)]
        self.view.set_fit_table_function_parameters(parameters, parameter_values)

    def handle_selected_workspaces_changed(self):
        runs, groups_and_pairs = self.model.get_runs_groups_and_pairs_for_fits()
        self.view.set_fit_table_workspaces(runs, groups_and_pairs)

    def handle_single_fit_requested(self):
        if self.model.fit_function is None:
            return

        self.selected_row = self.view.selected_row
        runs, group_and_pairs = self.view.get_workspace_info_from_fit_table_row(self.selected_row)
        separated_runs = runs.split(';')
        separated_group_and_pairs = group_and_pairs.split(';')
        workspace_names = self.model.get_fit_workspace_names_from_groups_and_runs(separated_runs,
                                                                                  separated_group_and_pairs)
        # pull values from fit table
        self.do_a_single_fit(workspace_names)

    def handle_sequential_fit_requested(self):
        if self.model.fit_function is None:
            return
        
        params = {}
        params['Minimizer'] = 'Levenberg-Marquardt'
        params['EvaluationType'] = 'CentrePoint'
        params['Function'] = self.model.fit_function
        params['StartX'] = 0.11
        params['EndX'] = 15
        params['InputWorkspace'] = []
        for i in range(self.view.number_of_entries()):
            runs, group_and_pairs = self.view.get_workspace_info_from_fit_table_row(i)
            workspace_name = self.model.get_workspace_names_from_group_and_runs(runs, group_and_pairs)
            params['InputWorkspace'] += [workspace_name]

        self.do_seq_fit(params)

    def do_a_single_fit(self, workspaces):
        calculation_function = functools.partial(
            self.model.evaluate_single_fit, workspaces, False)
        self.calculation_thread = self.create_thread(
            calculation_function)

        self.calculation_thread.threadWrapperSetUp(on_thread_end_callback=self.handle_single_fit_finished)

        self.calculation_thread.start()

    def do_seq_fit(self, params):
        calculation_function = functools.partial(
            self.model.do_sequential_fit, params)
        self.calculation_thread = self.create_thread(
            calculation_function)

        self.calculation_thread.threadWrapperSetUp(on_thread_end_callback=self.handle_seq_fit_finished)

        self.calculation_thread.start()

    def handle_single_fit_finished(self):
        fit_function, fit_status, fit_chi_squared = self.fitting_calculation_model.result
        number_of_parameters = fit_function.nParams()
        parameters = [fit_function.parameterName(i) for i in range(number_of_parameters)]
        parameter_values = [fit_function.getParameterValue(parameters[i]) for i in
                            range(number_of_parameters)]
        self.view.update_fit_function_parameters(self.selected_row, parameter_values)
        self.view.update_fit_quality(self.selected_row, fit_status, fit_chi_squared)

    def handle_seq_fit_finished(self):
        fit_functions, fit_statuses, fit_chi_squareds = self.fitting_calculation_model.result
        count = 0
        for fit_function, fit_status, fit_chi_squared in zip(fit_functions, fit_statuses, fit_chi_squareds):
            number_of_parameters = fit_function.nParams()
            parameters = [fit_function.parameterName(i) for i in range(number_of_parameters)]
            parameter_values = [fit_function.getParameterValue(parameters[i]) for i in
                                range(number_of_parameters)]
            self.view.update_fit_function_parameters(count, parameter_values)
            self.view.update_fit_quality(count, fit_status, fit_chi_squared)
            count += 1

    def create_thread(self, callback):
        self.fitting_calculation_model = ThreadModelWrapperWithOutput(callback)
        return thread_model.ThreadModel(self.fitting_calculation_model)
