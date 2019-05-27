# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.observer_pattern import Observable


class FittingNotifier(Observable):
    def __init__(self, outer):
        Observable.__init__(self)
        self.outer = outer  # handle to containing class

    def notify_subscribers(self, *args, **kwargs):
        Observable.notify_subscribers(self, *args, **kwargs)


class FitInformation(object):
    def __init__(self, parameter_workspace, fit_function_name, input_workspace):
        self.parameter_workspace = parameter_workspace
        self.fit_function_name = fit_function_name
        self.input_workspace = input_workspace

    @property
    def parameter_name(self):
        return self.parameter_workspace.workspace_name

    def __eq__(self, other):
        return self.parameter_workspace == other.parameter_workspace\
               and self.fit_function_name == other.fit_function_name\
               and self.input_workspace == other.input_workspace


class FittingContext(object):
    def __init__(self):
        self.fit_list = []
        self.new_fit_notifier = FittingNotifier(self)

    def add_fit(self, fit):
        self.fit_list.append(fit)
        self.new_fit_notifier.notify_subscribers()

    def add_fit_from_values(self, parameter_workspace, fit_function_name, input_workspace):
        fit_information = FitInformation(parameter_workspace, fit_function_name, input_workspace)
        self.add_fit(fit_information)

    def get_list_of_fits_for_a_given_fit_function(self, fit_function_name):
        return [fit for fit in self.fit_list if fit.fit_function_name == fit_function_name]

    def get_list_of_fit_functions(self):
        return list(set([fit.fit_function_name for fit in self.fit_list]))

    def get_fit_object_from_parameter_name(self, parameter_name):
        for fit in self.fit_list:
            if fit.parameter_name == parameter_name:
                return fit
