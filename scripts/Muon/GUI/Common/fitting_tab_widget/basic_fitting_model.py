# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.fitting_context import FitInformation

from typing import List, NamedTuple

FDA_GUESS_WORKSPACE = "__frequency_domain_analysis_fitting_guess"
MA_GUESS_WORKSPACE = "__muon_analysis_fitting_guess"
FDA_SUFFIX = " FD"
MA_SUFFIX = " MA"


def get_function_name_for_composite(composite):
    function_names = [composite.getFunction(i).name() for i in range(composite.nFunctions())]

    if len(function_names) > 3:
        function_names = function_names[:3]
        function_names.append("...")
    return ",".join(function_names)


class FitPlotInformation(NamedTuple):
    input_workspaces: List[str]
    fit: FitInformation


class BasicFittingModel:
    def __init__(self, context):
        self.context = context

        self._current_domain_index = 0

        self._single_fit_functions = [None]
        self._single_fit_functions_cache = [None]

        self._function_name = ""
        self._function_name_auto_update = True

        #self._grppair_index = {}
        self.fitting_options = {}
        self.ws_fit_function_map = {}

    @property
    def single_fit_functions(self):
        return self._single_fit_functions

    @single_fit_functions.setter
    def single_fit_functions(self, fit_functions):
        if len(self._single_fit_functions) == 0:
            raise ValueError("The provided list of fit functions is empty.")

        self._single_fit_functions = [self._clone_function(function) for function in fit_functions]

    def update_single_fit_function(self, index, function):
        if index >= len(self._single_fit_functions):
            raise ValueError("The provided domain index does not exist.")

        self._single_fit_functions[index] = self._clone_function(function)

    @property
    def current_domain_index(self):
        return self._current_domain_index

    @current_domain_index.setter
    def current_domain_index(self, index):
        self._current_domain_index = index

    @property
    def single_fit_functions_cache(self):
        return self._single_fit_functions_cache

    @single_fit_functions_cache.setter
    def single_fit_functions_cache(self, fit_functions):
        self._single_fit_functions_cache = fit_functions

    def cache_the_current_fit_functions(self):
        self.single_fit_functions_cache = [self._clone_function(function) for function in self.single_fit_functions]

    @property
    def function_name(self):
        return self._function_name

    @function_name.setter
    def function_name(self, new_name):
        self._function_name = " " + new_name
        if self._function_name.isspace():
            self._function_name = ""

    @property
    def function_name_auto_update(self):
        return self._function_name_auto_update

    @function_name_auto_update.setter
    def function_name_auto_update(self, auto_update):
        self._function_name_auto_update = auto_update

    def automatically_update_function_name(self):
        if self.function_name_auto_update:
            self.function_name = self._get_function_name(self.single_fit_functions[0])

    @staticmethod
    def _clone_function(function):
        if function is None:
            return function
        return function.clone()

    @staticmethod
    def _get_function_name(function):
        if function is None:
            return ""
        if function.getNumberDomains() > 1:
            function = function.getFunction(0)

        try:
            return get_function_name_for_composite(function)
        except AttributeError:
            return function.name()
