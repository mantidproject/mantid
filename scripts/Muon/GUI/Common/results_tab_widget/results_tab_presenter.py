# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.observer_pattern import GenericObserver


class ResultsTabPresenter(object):
    def __init__(self, context, view):
        self.view = view
        self.context = context

        self.new_fit_performed_observer = GenericObserver(self.handle_new_fit_performed)

    def get_selected_fit_list(self):
        return []

    def get_selected_logs_list(self):
        return []

    def get_tuple_of_logname_log_value_pairs(self, selected_workspace_list):
        return ('run_number', '22275')

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
