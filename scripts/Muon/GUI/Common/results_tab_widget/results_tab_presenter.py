# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.observer_pattern import GenericObserver
from mantid.api import AnalysisDataService
from mantid.kernel import StringPropertyWithValue, FloatTimeSeriesProperty
special_case_log_names = ["run_number", "run_start", "run_end", "group", "period", "sample_temp", "sample_magn_field"]
from mantid.simpleapi import CreateEmptyTableWorkspace


class ResultsTabPresenter(object):
    def __init__(self, context, view):
        self.view = view
        self.context = context

        self.new_fit_performed_observer = GenericObserver(self.handle_new_fit_performed)

    def get_selected_fit_list(self):
        return []

    def get_selected_logs_list(self):
        return []

    def get_log_names_from_workspace(self, workspace_name):
        log_names = []
        log_data = list(AnalysisDataService.retrieve(workspace_name).run().getLogData())

        for log in log_data:
            if type(log) == FloatTimeSeriesProperty:
                log_names.append(log.name)
            elif log.name in special_case_log_names:
                log_names.append(log.name)

        return log_names

    def get_performed_fits_by_fit_function(self, fit_function_name):
        return self.context.fitting_context.get_list_of_fits_for_a_given_fit_function(fit_function_name)

    def get_list_of_fit_functions_used(self):
        return self.context.fitting_context.get_list_of_fit_functions()

    def handle_new_fit_performed(self):
        fit_function_list = self.get_list_of_fit_functions_used()
        self.view.update_fit_function_list(fit_function_list)

    def handle_fit_function_changed(self):
        currently_selected_workspaces = self.view.get_selected_fit_list()
        updated_fit_list = self.get_performed_fits_by_fit_function(self.view.fit_function)

        new_model_dictionary = {item.parameter_name: [index, item.parameter_name in currently_selected_workspaces, True]
                                for index, item in enumerate(updated_fit_list)}

        self.view.update_fit_selector_model(new_model_dictionary)
        self.update_logs_list(updated_fit_list)

    def handle_create_results_table_button_clicked(self):
        selected_fits = [self.context.fitting_context.get_fit_object_from_parameter_name(item)
                         for item in self.view.get_selected_fit_list()]
        selected_logs = self.view.get_selected_logs_list()
        table_workspace = CreateEmptyTableWorkspace(OutputWorkspace=str(self.view.results_table_line_edit.text()))

        table_workspace.addColumn('str', 'workspace_name')
        first_parameter_dict = selected_fits[0].parameter_workspace.workspace.toDict()
        # for log_name in selected_logs:
        #     table_workspace.addColumn('float', log_name)

        for name in first_parameter_dict['Name']:
            table_workspace.addColumn('float', name)
            table_workspace.addColumn('float', name + 'Error')

        for fit in selected_fits:
            parameter_dict = fit.parameter_workspace.workspace.toDict()
            row_dict = {'workspace_name': fit.parameter_name}
            for name, value, error in zip(parameter_dict['Name'], parameter_dict['Value'], parameter_dict['Error']):
                row_dict.update({name: value, name + 'Error': error})

            table_workspace.addRow(row_dict)

    def update_logs_list(self, updated_fit_list):
        if not updated_fit_list:
            return

        currently_selected_logs = self.view.get_selected_logs_list()
        input_workspace_name = updated_fit_list[0].input_workspace

        log_names_list = self.get_log_names_from_workspace(input_workspace_name)

        new_model_dict = {log_name: [index, log_name in currently_selected_logs, True] for index, log_name in enumerate(log_names_list)}

        self.view.update_log_selector_model(new_model_dict)
