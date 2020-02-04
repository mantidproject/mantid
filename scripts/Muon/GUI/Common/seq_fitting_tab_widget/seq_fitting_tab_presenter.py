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
from Muon.GUI.Common.ADSHandler.workspace_naming import get_run_number_from_workspace_name, get_group_or_pair_from_name
from mantid.api import MultiDomainFunction, AnalysisDataService
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
        print("parameters = ", parameters, "parameter values =", parameter_values)
        self.view.set_fit_table_function_parameters(parameters, parameter_values)

    def handle_selected_workspaces_changed(self):
        runs, groups_and_pairs = self.model.get_selected_workspace_runs_groups_and_pairs()
        self.view.set_fit_table_workspaces(runs, groups_and_pairs)

    def handle_single_fit_requested(self):

        if self.model.fit_function is None:
            return

        print(self.model.fit_function)
        self.selected_row = self.view.selected_row
        runs, group_and_pairs = self.view.get_workspace_info_from_fit_table_row(self.selected_row)
        xRange = self.view.get_fit_x_range(self.selected_row)
        workspace_name = self.model.get_workspace_names_from_group_and_runs(runs, group_and_pairs)

        # do a single fit with that workspace
        params = {}
        params['Minimizer'] = 'Levenberg-Marquardt'
        params['EvaluationType'] = 'CentrePoint'
        params['Function'] = self.model.fit_function
        params['InputWorkspace'] = workspace_name
        params['StartX'] = 0.11
        params['EndX'] = 15
        # pull values from fit table
        self.do_a_single_fit(params)

    def do_a_single_fit(self, params):
        calculation_function = functools.partial(
            self.model.do_single_fit, params, False)
        self.calculation_thread = self.create_thread(
            calculation_function)

        self.calculation_thread.threadWrapperSetUp(on_thread_end_callback=self.handle_fit_finished)

        self.calculation_thread.start()

    def handle_fit_finished(self):
        fit_function, fit_status, fit_chi_squared = self.fitting_calculation_model.result
        number_of_parameters = fit_function.nParams()
        parameters = [fit_function.parameterName(i) for i in range(number_of_parameters)]
        parameter_values = [fit_function.getParameterValue(parameters[i]) for i in
                            range(number_of_parameters)]
        print("parameters = ", parameters, "parameter values =", parameter_values)
        self.view.update_fit_function_parameters(self.selected_row, parameter_values)
        self.view.update_fit_quality(self.selected_row, fit_status, fit_chi_squared)

    def create_thread(self, callback):
        self.fitting_calculation_model = ThreadModelWrapperWithOutput(callback)
        return thread_model.ThreadModel(self.fitting_calculation_model)
