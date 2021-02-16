# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import AlgorithmManager, logger
from mantid.simpleapi import CopyLogs, EvaluateFunction

from Muon.GUI.Common.ADSHandler.workspace_naming import create_fitted_workspace_name, create_parameter_table_name
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.contexts.fitting_context import FitInformation
from Muon.GUI.Common.utilities.algorithm_utils import run_Fit

import math
import re
from typing import List, NamedTuple

DEFAULT_CHI_SQUARED = 0.0
DEFAULT_FIT_STATUS = None
DEFAULT_SINGLE_FIT_FUNCTION = None
DEFAULT_START_X = 0.0
DEFAULT_FDA_END_X = 250.0
DEFAULT_MA_END_X = 15.0

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
    def __init__(self, context, is_frequency_domain):
        self.context = context

        self._default_end_x = DEFAULT_FDA_END_X if is_frequency_domain else DEFAULT_MA_END_X

        self._current_dataset_index = None
        self._dataset_names = []

        self._start_xs = []
        self._end_xs = []

        self._single_fit_functions = []
        self._single_fit_functions_cache = []

        self._fit_statuses = []
        self._chi_squared = []

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
        if index is not None and index >= self.number_of_datasets:
            raise RuntimeError(f"The provided dataset index ({index}) is too large.")

        self._current_dataset_index = index

    @property
    def dataset_names(self):
        return self._dataset_names

    @dataset_names.setter
    def dataset_names(self, names):
        self._dataset_names = names

        self.reset_fit_functions()
        self.reset_current_dataset_index()
        self.reset_start_xs_and_end_xs()
        self.reset_fit_statuses_and_chi_squared()
        self.clear_cached_fit_functions()

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
    def start_xs(self):
        return self._start_xs

    @start_xs.setter
    def start_xs(self, start_xs):
        self._start_xs = start_xs

    @property
    def current_start_x(self):
        if self.number_of_datasets > 0:
            return self.start_xs[self.current_dataset_index]
        else:
            return DEFAULT_START_X

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
        if self.current_dataset_index is not None:
            return self.end_xs[self.current_dataset_index]
        else:
            return self._default_end_x

    @current_end_x.setter
    def current_end_x(self, value):
        self.end_xs[self.current_dataset_index] = value

    def clear_single_fit_functions(self):
        self.single_fit_functions = [None] * self.number_of_datasets

    @property
    def single_fit_functions(self):
        return self._single_fit_functions

    @single_fit_functions.setter
    def single_fit_functions(self, fit_functions):
        self._single_fit_functions = [self._clone_function(function) for function in fit_functions]

    @property
    def current_single_fit_function(self):
        if self.current_dataset_index is not None:
            return self.single_fit_functions[self.current_dataset_index]
        else:
            return DEFAULT_SINGLE_FIT_FUNCTION

    @current_single_fit_function.setter
    def current_single_fit_function(self, fit_function):
        self.single_fit_functions[self.current_dataset_index] = fit_function

    def get_single_fit_function_for(self, dataset_name):
        if dataset_name in self.dataset_names:
            return self.single_fit_functions[self.dataset_names.index(dataset_name)]
        else:
            return None

    @property
    def single_fit_functions_cache(self):
        return self._single_fit_functions_cache

    @single_fit_functions_cache.setter
    def single_fit_functions_cache(self, fit_functions):
        self._single_fit_functions_cache = fit_functions

    def cache_the_current_fit_functions(self):
        self.single_fit_functions_cache = [self._clone_function(function) for function in self.single_fit_functions]

    def clear_cached_fit_functions(self):
        self.single_fit_functions_cache = [None] * self.number_of_datasets
        self.remove_all_fits_from_context()

    @property
    def fit_statuses(self):
        return self._fit_statuses

    @fit_statuses.setter
    def fit_statuses(self, fit_statuses):
        self._fit_statuses = fit_statuses

    @property
    def current_fit_status(self):
        if self.current_dataset_index is not None:
            return self.fit_statuses[self.current_dataset_index]
        else:
            return DEFAULT_FIT_STATUS

    @current_fit_status.setter
    def current_fit_status(self, fit_status):
        self.fit_statuses[self.current_dataset_index] = fit_status

    @property
    def chi_squared(self):
        return self._chi_squared

    @chi_squared.setter
    def chi_squared(self, chi_squared_list):
        self._chi_squared = chi_squared_list

    @property
    def current_chi_squared(self):
        if self.current_dataset_index is not None:
            return self.chi_squared[self.current_dataset_index]
        else:
            return DEFAULT_CHI_SQUARED

    @current_chi_squared.setter
    def current_chi_squared(self, chi_squared):
        self.chi_squared[self.current_dataset_index] = chi_squared

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
            self.function_name = self._get_function_name(self.current_single_fit_function)

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
        self.context.fitting_context.fit_raw = fit_to_raw

    @property
    def simultaneous_fitting_mode(self):
        """Returns true if the fitting mode is simultaneous. Override this method if you require simultaneous."""
        return False

    @property
    def do_rebin(self):
        return self.context._do_rebin()

    def use_cached_function(self):
        """Sets the current function as being the cached function."""
        self.single_fit_functions = self.single_fit_functions_cache

    def update_plot_guess(self, plot_guess):
        """Updates the guess plot using the current dataset and function."""
        guess_workspace_name = self._evaluate_plot_guess(plot_guess)
        self.context.fitting_context.notify_plot_guess_changed(plot_guess, guess_workspace_name)

    def remove_all_fits_from_context(self):
        """Removes all fit results from the context."""
        self.context.fitting_context.remove_all_fits()

    def reset_current_dataset_index(self):
        """Resets the current dataset index stored by the model. This shouldn't be necessary."""
        if self.number_of_datasets == 0:
            self.current_dataset_index = None
        elif self.current_dataset_index is None or self.current_dataset_index >= self.number_of_datasets:
            self.current_dataset_index = 0

    def reset_start_xs_and_end_xs(self):
        """Resets the start and end Xs stored by the model."""
        self._reset_start_xs()
        self._reset_end_xs()

    def reset_fit_statuses_and_chi_squared(self):
        """Reset the fit statuses and chi squared stored by the model."""
        self.fit_statuses = [DEFAULT_FIT_STATUS] * self.number_of_datasets
        self.chi_squared = [DEFAULT_CHI_SQUARED] * self.number_of_datasets

    def reset_fit_functions(self):
        """Reset the fit functions stored by the model. Attempts to use the currently selected function."""
        fit_function = None if len(self.single_fit_functions) == 0 else self.current_single_fit_function
        self.single_fit_functions = [self._clone_function(fit_function)] * self.number_of_datasets

    def _reset_start_xs(self):
        """Resets the start Xs stored by the model."""
        if self.number_of_datasets > 0:
            self.start_xs = [self.retrieve_first_good_data_from_run(name) for name in self.dataset_names]
        else:
            self.start_xs = []

    def _reset_end_xs(self):
        """Resets the end Xs stored by the model."""
        end_x = self.current_end_x if len(self.end_xs) > 0 else self._default_end_x
        self.end_xs = [end_x] * self.number_of_datasets

    def retrieve_first_good_data_from_run(self, workspace_name):
        """Returns the first good data value from a run number within a workspace name."""
        try:
            return self.context.first_good_data([float(re.search("[0-9]+", workspace_name).group())])
        except AttributeError:
            return DEFAULT_START_X

    def get_active_fit_function(self):
        """Returns the fit function that is active and will be used for a fit."""
        return self.current_single_fit_function

    def get_active_workspace_names(self):
        """Returns the names of the workspaces that will be fitted. Single Fit mode only fits the selected workspace."""
        return [self.current_dataset_name]

    def get_active_fit_results(self):
        """Returns the results of the currently active fit. For Single Fit there is only one fit result."""
        if self.number_of_datasets > 0:
            return self._create_fit_plot_information(self.get_active_workspace_names(), self.function_name)
        else:
            return []

    def get_workspace_names_to_display_from_context(self):
        """Returns the workspace names to display in the view and store in the model."""
        raise NotImplementedError("This method must be overridden by a child class.")

    def perform_fit(self):
        """Performs a single fit and returns the resulting function, status and chi squared."""
        function, fit_status, chi_squared = self._do_single_fit(self._get_parameters_for_single_fit())
        return function, fit_status, chi_squared

    def _do_single_fit(self, parameter_dict):
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = \
            self._do_single_fit_and_return_workspace_parameters_and_fit_function(parameter_dict)

        self._add_single_fit_results_to_ADS_and_context(self.current_dataset_name, parameter_table, output_workspace,
                                                        covariance_matrix)
        return function, fit_status, chi_squared

    def _do_single_fit_and_return_workspace_parameters_and_fit_function(self, parameters_dict):
        alg = self._create_fit_algorithm()
        workspace, parameters, function, fit_status, chi_squared, covariance_matrix = run_Fit(parameters_dict, alg)

        CopyLogs(InputWorkspace=self.current_dataset_name, OutputWorkspace=workspace, StoreInADS=False)
        return workspace, parameters, function, fit_status, chi_squared, covariance_matrix

    def _get_parameters_for_single_fit(self):
        params = self._get_common_parameters()
        params["InputWorkspace"] = self.current_dataset_name
        params["StartX"] = self.current_start_x
        params["EndX"] = self.current_end_x
        return params

    def _get_common_parameters(self):
        return {"Function": self.get_active_fit_function(),
                "Minimizer": self.minimizer,
                "EvaluationType": self.evaluation_type}

    def _create_fit_algorithm(self):
        if self._double_pulse_enabled():
            return self._create_double_pulse_alg()
        else:
            return AlgorithmManager.create("Fit")

    def _create_double_pulse_alg(self):
        alg = AlgorithmManager.create("DoublePulseFit")
        offset = self.context.gui_context['DoublePulseTime']
        first_pulse_weighting, second_pulse_weighting = self._get_pulse_weightings(offset, 2.2)
        alg.setProperty("PulseOffset", offset)
        alg.setProperty("FirstPulseWeight", first_pulse_weighting)
        alg.setProperty("SecondPulseWeight", second_pulse_weighting)
        return alg

    def _double_pulse_enabled(self):
        return "DoublePulseEnabled" in self.context.gui_context and self.context.gui_context["DoublePulseEnabled"]

    @staticmethod
    def _get_pulse_weightings(offset, muon_halflife):
        decay = math.exp(-offset / muon_halflife)
        first_pulse_weighting = decay / (1 + decay)
        second_pulse_weighting = 1 / (1 + decay)
        return first_pulse_weighting, second_pulse_weighting

    def _add_single_fit_results_to_ADS_and_context(self, input_workspace, parameters_table, output_workspace,
                                                   covariance_matrix):
        workspace_name, table_name, table_directory = self._add_single_fit_workspaces_to_ADS(input_workspace,
                                                                                             output_workspace,
                                                                                             covariance_matrix)

        self._add_fit_to_context(self._add_workspace_to_ADS(parameters_table, table_name, table_directory),
                                 input_workspace, [workspace_name])

    def _add_single_fit_workspaces_to_ADS(self, input_workspace, output_workspace, covariance_matrix):
        workspace_name, workspace_directory = create_fitted_workspace_name(input_workspace, self.function_name)
        table_name, table_directory = create_parameter_table_name(input_workspace, self.function_name)

        self._add_workspace_to_ADS(output_workspace, workspace_name, workspace_directory)
        self._add_workspace_to_ADS(covariance_matrix, workspace_name + '_CovarianceMatrix', table_directory)

        return workspace_name, table_name, table_directory

    def _add_fit_to_context(self, parameter_workspace, input_workspaces, output_workspaces, global_parameters=None):
        self.context.fitting_context.add_fit_from_values(parameter_workspace, self.function_name, input_workspaces,
                                                         output_workspaces, global_parameters)

    def _create_fit_plot_information(self, workspace_names, function_name):
        return [FitPlotInformation(input_workspaces=workspace_names,
                                   fit=self._get_fit_results_from_context(workspace_names, function_name))]

    def _get_fit_results_from_context(self, workspace_names, function_name):
        return self.context.fitting_context.find_fit_for_input_workspace_list_and_function(workspace_names,
                                                                                           function_name)

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

    def _evaluate_plot_guess(self, plot_guess: bool):
        if not plot_guess or self.current_dataset_name is None:
            return None

        if fit_function := self._get_plot_guess_fit_function():
            return self._evaluate_function(fit_function, self._get_plot_guess_name())
        return None

    def _evaluate_function(self, fit_function, output_workspace):
        try:
            EvaluateFunction(InputWorkspace=self.current_dataset_name,
                             Function=fit_function,
                             StartX=self.current_start_x,
                             EndX=self.current_end_x,
                             OutputWorkspace=output_workspace)
        except RuntimeError:
            logger.error("Failed to plot guess.")
            return None
        return output_workspace

    def _get_plot_guess_name(self):
        if self.context.workspace_suffix == MA_SUFFIX:
            return MA_GUESS_WORKSPACE + self.current_dataset_name
        else:
            return FDA_GUESS_WORKSPACE + self.current_dataset_name

    def _get_plot_guess_fit_function(self):
        fit_function = self.get_active_fit_function()
        if fit_function is not None and self.simultaneous_fitting_mode:
            return fit_function.createEquivalentFunctions()[self.current_dataset_index]
        else:
            return fit_function

    def _add_workspace_to_ADS(self, workspace, name, directory):
        self.context.ads_observer.observeRename(False)
        workspace_wrapper = MuonWorkspaceWrapper(workspace)
        workspace_wrapper.show(directory + name)
        self.context.ads_observer.observeRename(True)
        return workspace_wrapper
