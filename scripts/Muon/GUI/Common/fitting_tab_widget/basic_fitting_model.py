# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.fitting_context import FitInformation

from typing import List, NamedTuple

DEFAULT_START_X = 0.0
DEFAULT_END_X = 15.0
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

        self._current_dataset_index = 0
        self._dataset_names = []

        self._single_fit_functions = [None]
        self._single_fit_functions_cache = [None]

        self._fit_statuses = [None]
        self._fit_chi_squares = [0.0]

        self._start_xs = [DEFAULT_START_X]
        self._end_xs = [DEFAULT_END_X]

        self._function_name = ""
        self._function_name_auto_update = True

        self._minimizer = ""
        self._evaluation_type = ""
        self._fit_to_raw = False

    @property
    def current_dataset_index(self):
        return self._current_dataset_index

    @current_dataset_index.setter
    def current_dataset_index(self, index):
        self._current_dataset_index = index

    @property
    def dataset_names(self):
        return self._dataset_names

    @dataset_names.setter
    def dataset_names(self, names):
        self._dataset_names = names

    @property
    def current_dataset_name(self):
        return self.dataset_names[self.current_dataset_index]

    @current_dataset_name.setter
    def current_dataset_name(self, name):
        self.dataset_names[self.current_dataset_index] = name

    @property
    def number_of_datasets(self):
        return len(self.dataset_names)

    @property
    def single_fit_functions(self):
        return self._single_fit_functions

    @single_fit_functions.setter
    def single_fit_functions(self, fit_functions):
        if len(self._single_fit_functions) == 0:
            raise ValueError("The provided list of fit functions is empty.")

        self._single_fit_functions = [self._clone_function(function) for function in fit_functions]

    @property
    def current_single_fit_function(self):
        return self.single_fit_functions[self.current_dataset_index]

    @current_single_fit_function.setter
    def current_single_fit_function(self, fit_function):
        self.single_fit_functions[self.current_dataset_index] = fit_function

    @property
    def single_fit_functions_cache(self):
        return self._single_fit_functions_cache

    @single_fit_functions_cache.setter
    def single_fit_functions_cache(self, fit_functions):
        self._single_fit_functions_cache = fit_functions

    def cache_the_current_fit_functions(self):
        self.single_fit_functions_cache = [self._clone_function(function) for function in self.single_fit_functions]

    @property
    def fit_statuses(self):
        return self._fit_statuses

    @fit_statuses.setter
    def fit_statuses(self, fit_statuses):
        self._fit_statuses = fit_statuses

    @property
    def current_fit_status(self):
        return self.fit_statuses[self.current_dataset_index]

    @current_fit_status.setter
    def current_fit_status(self, fit_status):
        self.fit_statuses[self.current_dataset_index] = fit_status

    @property
    def fit_chi_squares(self):
        return self._fit_chi_squares

    @fit_chi_squares.setter
    def fit_chi_squares(self, fit_chi_squares):
        self._fit_chi_squares = fit_chi_squares

    @property
    def current_fit_chi_squared(self):
        return self.fit_chi_squares[self.current_dataset_index]

    @current_fit_chi_squared.setter
    def current_fit_chi_squared(self, fit_chi_squared):
        self.fit_chi_squares[self.current_dataset_index] = fit_chi_squared

    @property
    def start_xs(self):
        return self._start_xs

    @start_xs.setter
    def start_xs(self, start_xs):
        self._start_xs = start_xs

    @property
    def current_start_x(self):
        return self.start_xs[self.current_dataset_index]

    @current_start_x.setter
    def current_start_x(self, value):
        self.start_xs[self.current_dataset_index] = value

    @property
    def end_xs(self):
        return self._end_xs

    @end_xs.setter
    def end_xs(self, end_xs):
        self._end_xs = end_xs

    @property
    def current_end_x(self):
        return self.end_xs[self.current_dataset_index]

    @current_end_x.setter
    def current_end_x(self, value):
        self.end_xs[self.current_dataset_index] = value

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

    @property
    def minimizer(self):
        return self._minimizer

    @minimizer.setter
    def minimizer(self, minimizer):
        self._minimizer = minimizer

    @property
    def evaluation_type(self):
        return self._evaluation_type

    @evaluation_type.setter
    def evaluation_type(self, evaluation_type):
        self._evaluation_type = evaluation_type

    @property
    def fit_to_raw(self):
        return self._fit_to_raw

    @fit_to_raw.setter
    def fit_to_raw(self, fit_to_raw):
        self._fit_to_raw = fit_to_raw

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
