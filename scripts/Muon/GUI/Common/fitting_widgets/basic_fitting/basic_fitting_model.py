# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import AlgorithmManager, logger
from mantid.api import CompositeFunction, IAlgorithm, IFunction
from mantid.simpleapi import CopyLogs, EvaluateFunction

from Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist, retrieve_ws
from Muon.GUI.Common.ADSHandler.workspace_naming import (create_covariance_matrix_name, create_fitted_workspace_name,
                                                         create_parameter_table_name)
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.contexts.fitting_contexts.basic_fitting_context import BasicFittingContext
from Muon.GUI.Common.contexts.fitting_contexts.fitting_context import FitInformation
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.utilities.algorithm_utils import run_Fit
from Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper

import math
import re
from typing import List, NamedTuple

DEFAULT_CHI_SQUARED = 0.0
DEFAULT_FIT_STATUS = None
DEFAULT_SINGLE_FIT_FUNCTION = None
DEFAULT_START_X = 0.0
X_OFFSET = 0.001


def get_function_name_for_composite(composite: CompositeFunction) -> str:
    """Gets the function name to associate with a composite function when saving fit results."""
    function_names = [composite.getFunction(i).name() for i in range(composite.nFunctions())]

    if len(function_names) > 3:
        function_names = function_names[:3]
        function_names.append("...")
    return ",".join(function_names)


class FitPlotInformation(NamedTuple):
    """
    This class is used to send fit data to the plot widget for plotting.
    """
    input_workspaces: List[str]
    fit: FitInformation


class BasicFittingModel:
    """
    The BasicFittingModel stores the datasets, start Xs, end Xs, fit statuses and chi squared for Single Fitting.
    """

    def __init__(self, context: MuonContext, fitting_context: BasicFittingContext):
        """Initializes the model with empty fit data."""
        self.context = context
        self.fitting_context = fitting_context

    @property
    def current_dataset_index(self) -> int:
        """Returns the index of the currently selected dataset."""
        return self.fitting_context.current_dataset_index

    @current_dataset_index.setter
    def current_dataset_index(self, index: int) -> None:
        """Sets the index of the currently selected dataset."""
        self.fitting_context.current_dataset_index = index

    @property
    def dataset_names(self) -> list:
        """Returns the names of all the datasets stored in the model."""
        return self.fitting_context.dataset_names

    @dataset_names.setter
    def dataset_names(self, names: list) -> None:
        """Sets the dataset names stored by the model. Resets the other fitting data."""
        start_xs, end_xs = self._get_new_start_xs_and_end_xs_using_existing_datasets(names)
        functions = self._get_new_functions_using_existing_datasets(names)

        self.fitting_context.dataset_names = names

        self.fitting_context.start_xs = start_xs
        self.fitting_context.end_xs = end_xs

        self.reset_fit_functions(functions)
        self.reset_current_dataset_index()
        self.reset_fit_statuses_and_chi_squared()
        self.clear_undo_data()

    @property
    def current_dataset_name(self) -> str:
        """Returns the currently selected dataset name"""
        current_dataset_index = self.fitting_context.current_dataset_index
        if current_dataset_index is not None:
            return self.fitting_context.dataset_names[current_dataset_index]
        else:
            return None

    @current_dataset_name.setter
    def current_dataset_name(self, name: str) -> None:
        """Sets the currently selected dataset name to have a different name."""
        self.fitting_context.dataset_names[self.fitting_context.current_dataset_index] = name

    @property
    def number_of_datasets(self) -> int:
        """Returns the number of datasets stored by the model."""
        return self.fitting_context.number_of_datasets

    @property
    def start_xs(self) -> list:
        """Returns all of the start Xs stored by the model."""
        return self.fitting_context.start_xs

    @start_xs.setter
    def start_xs(self, start_xs: list) -> None:
        """Sets all of the start Xs in the model."""
        self.fitting_context.start_xs = start_xs

    @property
    def current_start_x(self) -> float:
        """Returns the currently selected start X, or the default start X if nothing is selected."""
        current_dataset_index = self.fitting_context.current_dataset_index
        if current_dataset_index is not None:
            return self.fitting_context.start_xs[current_dataset_index]
        else:
            return DEFAULT_START_X

    @current_start_x.setter
    def current_start_x(self, value: float) -> None:
        """Sets the value of the currently selected start X."""
        if value < self.current_end_x:
            self.fitting_context.start_xs[self.fitting_context.current_dataset_index] = value

    @property
    def end_xs(self) -> list:
        """Returns all of the end Xs stored by the model."""
        return self.fitting_context.end_xs

    @end_xs.setter
    def end_xs(self, end_xs: list) -> None:
        """Sets all of the end Xs in the model."""
        self.fitting_context.end_xs = end_xs

    @property
    def current_end_x(self) -> float:
        """Returns the currently selected start X, or the default start X if nothing is selected."""
        current_dataset_index = self.fitting_context.current_dataset_index
        if current_dataset_index is not None:
            return self.fitting_context.end_xs[current_dataset_index]
        else:
            return self.context.default_end_x

    @current_end_x.setter
    def current_end_x(self, value: float) -> None:
        """Sets the value of the currently selected end X."""
        if value > self.current_start_x:
            self.fitting_context.end_xs[self.fitting_context.current_dataset_index] = value

    def clear_single_fit_functions(self) -> None:
        """Clears the single fit functions corresponding to each dataset."""
        self.fitting_context.single_fit_functions = [None] * self.fitting_context.number_of_datasets

    @property
    def single_fit_functions(self) -> list:
        """Returns all of the fit functions used for single fitting. Each function corresponds to a dataset."""
        return self.fitting_context.single_fit_functions

    @single_fit_functions.setter
    def single_fit_functions(self, fit_functions: list) -> None:
        """Sets all of the single fit functions stored in the model."""
        self.fitting_context.single_fit_functions = [self._clone_function(function) for function in fit_functions]

    @property
    def current_single_fit_function(self) -> IFunction:
        """Returns the currently selected fit function for single fitting."""
        current_dataset_index = self.fitting_context.current_dataset_index
        if current_dataset_index is not None:
            return self.fitting_context.single_fit_functions[current_dataset_index]
        else:
            return DEFAULT_SINGLE_FIT_FUNCTION

    @current_single_fit_function.setter
    def current_single_fit_function(self, fit_function: IFunction) -> None:
        """Sets the currently selected single fit function."""
        self.fitting_context.single_fit_functions[self.fitting_context.current_dataset_index] = fit_function

    def get_single_fit_function_for(self, dataset_name: str) -> IFunction:
        """Returns the single fit function that corresponds to the given dataset name."""
        dataset_names = self.fitting_context.dataset_names
        if dataset_name in dataset_names:
            return self.fitting_context.single_fit_functions[dataset_names.index(dataset_name)]
        else:
            return DEFAULT_SINGLE_FIT_FUNCTION

    def number_of_undos(self) -> int:
        """Returns the number of previous single fits that are saved."""
        return len(self.fitting_context.single_fit_functions_for_undo)

    def undo_previous_fit(self) -> None:
        """Undoes the previous fit using the saved undo data."""
        if self.number_of_undos() > 0:
            self.fitting_context.undo_previous_fit()

            undo_dataset_index = self.fitting_context.dataset_indices_for_undo.pop()
            undo_fit_function = self.fitting_context.single_fit_functions_for_undo.pop()
            undo_fit_status = self.fitting_context.fit_statuses_for_undo.pop()
            undo_chi_squared = self.fitting_context.chi_squared_for_undo.pop()

            self.fitting_context.single_fit_functions[undo_dataset_index] = undo_fit_function
            self.fitting_context.fit_statuses[undo_dataset_index] = undo_fit_status
            self.fitting_context.chi_squared[undo_dataset_index] = undo_chi_squared

    def save_current_fit_function_to_undo_data(self) -> None:
        """Saves the current single fit function and other data before performing a fit."""
        self.fitting_context.dataset_indices_for_undo.append(self.fitting_context.current_dataset_index)
        self.fitting_context.single_fit_functions_for_undo.append(self._clone_function(
            self.current_single_fit_function))
        self.fitting_context.fit_statuses_for_undo.append(self.current_fit_status)
        self.fitting_context.chi_squared_for_undo.append(self.current_chi_squared)

    def clear_undo_data(self) -> None:
        """Clears the undo fit functions and other data for all previous fits."""
        self.fitting_context.remove_overridden_fits()
        self.fitting_context.dataset_indices_for_undo = []
        self.fitting_context.single_fit_functions_for_undo = []
        self.fitting_context.fit_statuses_for_undo = []
        self.fitting_context.chi_squared_for_undo = []

    def clear_undo_data_for_current_dataset_index(self) -> None:
        """Clears the undo fit functions and other data for the currently selected index."""
        current_dataset_index = self.fitting_context.current_dataset_index
        for i, dataset_index in reversed(list(enumerate(self.fitting_context.dataset_indices_for_undo))):
            if dataset_index == current_dataset_index:
                del self.fitting_context.active_fit_history[i]
                del self.fitting_context.dataset_indices_for_undo[i]
                del self.fitting_context.single_fit_functions_for_undo[i]
                del self.fitting_context.fit_statuses_for_undo[i]
                del self.fitting_context.chi_squared_for_undo[i]

    @property
    def fit_statuses(self) -> list:
        """Returns all of the fit statuses in a list."""
        return self.fitting_context.fit_statuses

    @fit_statuses.setter
    def fit_statuses(self, fit_statuses: list) -> None:
        """Sets the value of all fit statuses."""
        self.fitting_context.fit_statuses = fit_statuses

    @property
    def current_fit_status(self) -> str:
        """Returns the fit status of the dataset that is currently selected."""
        current_dataset_index = self.fitting_context.current_dataset_index
        if current_dataset_index is not None:
            return self.fitting_context.fit_statuses[current_dataset_index]
        else:
            return DEFAULT_FIT_STATUS

    @current_fit_status.setter
    def current_fit_status(self, fit_status: list) -> None:
        """Sets the fit status of the dataset that is currently selected."""
        self.fitting_context.fit_statuses[self.fitting_context.current_dataset_index] = fit_status

    @property
    def chi_squared(self) -> list:
        """Returns all of the chi squared values."""
        return self.fitting_context.chi_squared

    @chi_squared.setter
    def chi_squared(self, chi_squared: list) -> None:
        """Sets all of the chi squared values."""
        self.fitting_context.chi_squared = chi_squared

    @property
    def current_chi_squared(self) -> float:
        """Returns the chi squared of the dataset that is currently selected."""
        current_dataset_index = self.fitting_context.current_dataset_index
        if current_dataset_index is not None:
            return self.fitting_context.chi_squared[current_dataset_index]
        else:
            return DEFAULT_CHI_SQUARED

    @current_chi_squared.setter
    def current_chi_squared(self, chi_squared: float) -> None:
        """Sets the chi squared of the dataset that is currently selected."""
        self.fitting_context.chi_squared[self.fitting_context.current_dataset_index] = chi_squared

    @property
    def plot_guess(self) -> bool:
        """Returns true if plot guess is turned on."""
        return self.fitting_context.plot_guess

    @plot_guess.setter
    def plot_guess(self, plot_guess: bool) -> None:
        """Sets that the plot guess should or should not be plotted."""
        self.fitting_context.plot_guess = plot_guess

    @property
    def plot_guess(self) -> bool:
        """Returns true if plot guess is turned on."""
        return self.fitting_context.plot_guess

    @plot_guess.setter
    def plot_guess(self, plot_guess: bool) -> None:
        """Sets that the plot guess should or should not be plotted."""
        self.fitting_context.plot_guess = plot_guess

    @property
    def function_name(self) -> str:
        """Returns the function name to add to the end of a fitted workspace."""
        return self.fitting_context.function_name

    @function_name.setter
    def function_name(self, new_name: str) -> None:
        """Sets the function name to add to the end of a fitted workspace."""
        self.fitting_context.function_name = new_name

    @property
    def function_name_auto_update(self) -> bool:
        """Returns a boolean whether or not to automatically update the function name."""
        return self.fitting_context.function_name_auto_update

    @function_name_auto_update.setter
    def function_name_auto_update(self, auto_update: bool) -> None:
        """Sets whether or not to automatically update the function name."""
        self.fitting_context.function_name_auto_update = auto_update

    def automatically_update_function_name(self) -> None:
        """Attempt to update the function name automatically."""
        if self.fitting_context.function_name_auto_update:
            self.fitting_context.function_name = self._get_function_name(self.current_single_fit_function)

    @property
    def minimizer(self) -> str:
        """Returns the minimizer to be used during a fit."""
        return self.fitting_context.minimizer

    @minimizer.setter
    def minimizer(self, minimizer: str) -> None:
        """Sets the minimizer to be used during a fit."""
        self.fitting_context.minimizer = minimizer

    @property
    def evaluation_type(self) -> str:
        """Returns the evaluation type to be used during a fit."""
        return self.fitting_context.evaluation_type

    @evaluation_type.setter
    def evaluation_type(self, evaluation_type: str) -> None:
        """Sets the evaluation type to be used during a fit."""
        self.fitting_context.evaluation_type = evaluation_type

    @property
    def fit_to_raw(self) -> bool:
        """Returns true if fit to raw is turned on."""
        return self.fitting_context.fit_to_raw

    @fit_to_raw.setter
    def fit_to_raw(self, fit_to_raw: bool) -> None:
        """Sets the fit to raw property."""
        if fit_to_raw != self.fitting_context.fit_to_raw:
            self.fitting_context.fit_to_raw = fit_to_raw
            # Avoids resetting the start/end xs and etc. by not using the dataset_names setter.
            self.fitting_context.dataset_names = self._get_equivalent_binned_or_unbinned_workspaces()

    def update_parameter_value(self, full_parameter: str, value: float) -> None:
        """Update the value of a parameter in the fit function."""
        if self.current_single_fit_function is not None:
            self.current_single_fit_function.setParameter(full_parameter, value)

    @property
    def do_rebin(self) -> bool:
        """Returns true if rebin is selected within the context."""
        return self.context._do_rebin()

    def x_limits_of_workspace(self, workspace_name: str) -> tuple:
        """Returns the x data limits of a provided workspace or the current dataset."""
        if workspace_name is not None and check_if_workspace_exist(workspace_name):
            x_data = retrieve_ws(workspace_name).dataX(0)
            if len(x_data) > 0:
                x_data.sort()
                x_lower, x_higher = x_data[0], x_data[-1]
                if x_lower == x_higher:
                    return x_lower - X_OFFSET, x_higher + X_OFFSET
                return x_lower, x_higher
        return self.current_start_x, self.current_end_x

    def update_plot_guess(self) -> None:
        """Updates the guess plot using the current dataset and function."""
        self.fitting_context.guess_workspace_name = self._evaluate_plot_guess(self.plot_guess)

    def remove_all_fits_from_context(self) -> None:
        """Removes all fit results from the context."""
        self.fitting_context.clear()

    def reset_current_dataset_index(self) -> None:
        """Resets the current dataset index stored by the context."""
        if self.fitting_context.number_of_datasets == 0:
            self.fitting_context.current_dataset_index = None
        elif self.fitting_context.current_dataset_index is None or \
                self.fitting_context.current_dataset_index >= self.fitting_context.number_of_datasets:
            self.fitting_context.current_dataset_index = 0

    def reset_start_xs_and_end_xs(self) -> None:
        """Resets the start and end Xs stored by the context."""
        self._reset_start_xs()
        self._reset_end_xs()

    def reset_fit_statuses_and_chi_squared(self) -> None:
        """Reset the fit statuses and chi squared stored by the context."""
        self.fitting_context.fit_statuses = [DEFAULT_FIT_STATUS] * self.fitting_context.number_of_datasets
        self.fitting_context.chi_squared = [DEFAULT_CHI_SQUARED] * self.fitting_context.number_of_datasets

    def reset_current_fit_status_and_chi_squared(self) -> None:
        """Reset the current fit status and chi squared stored by the context."""
        self.current_fit_status = DEFAULT_FIT_STATUS
        self.current_chi_squared = DEFAULT_CHI_SQUARED

    def reset_fit_functions(self, new_functions: list) -> None:
        """Reset the fit functions stored by the model. Attempts to use the currently selected function."""
        self.fitting_context.single_fit_functions = [self._clear_function_errors(func) for func in new_functions]

    @staticmethod
    def _clear_function_errors(function: IFunction) -> IFunction:
        """Clears the errors of a function by setting them to zero."""
        if function is not None:
            for i in range(function.nParams()):
                function.setError(i, 0.0)
        return function

    def _reset_start_xs(self) -> None:
        """Resets the start Xs stored by the context."""
        if self.fitting_context.number_of_datasets > 0:
            self.fitting_context.start_xs = [self.retrieve_first_good_data_from(name)
                                             for name in self.fitting_context.dataset_names]
        else:
            self.fitting_context.start_xs = []

    def _reset_end_xs(self) -> None:
        """Resets the end Xs stored by the context."""
        self.fitting_context.end_xs = [self.current_end_x] * self.fitting_context.number_of_datasets

    def _get_new_start_xs_and_end_xs_using_existing_datasets(self, new_dataset_names: list) -> tuple:
        """Returns the start and end Xs to use for the new datasets. It tries to use existing ranges if possible."""
        if len(self.fitting_context.dataset_names) == len(new_dataset_names):
            return self.fitting_context.start_xs, self.fitting_context.end_xs
        else:
            start_xs = [self._get_new_start_x_for(name) for name in new_dataset_names]
            end_xs = [self._get_new_end_x_for(name) for name in new_dataset_names]
            return start_xs, end_xs

    def _get_new_start_x_for(self, new_dataset_name: str) -> float:
        """Returns the start X to use for the new dataset. It tries to use an existing start X if possible."""
        dataset_names = self.fitting_context.dataset_names
        if new_dataset_name in dataset_names:
            return self.fitting_context.start_xs[dataset_names.index(new_dataset_name)]
        else:
            x_lower, x_upper = self.x_limits_of_workspace(new_dataset_name)
            return self.current_start_x if x_lower < self.current_start_x < x_upper \
                else self.retrieve_first_good_data_from(new_dataset_name)

    def _get_new_end_x_for(self, new_dataset_name: str) -> float:
        """Returns the end X to use for the new dataset. It tries to use an existing end X if possible."""
        dataset_names = self.fitting_context.dataset_names
        if new_dataset_name in dataset_names:
            return self.fitting_context.end_xs[dataset_names.index(new_dataset_name)]
        else:
            x_lower, x_upper = self.x_limits_of_workspace(new_dataset_name)
            return self.current_end_x if x_lower < self.current_end_x < x_upper else x_upper

    def _get_new_functions_using_existing_datasets(self, new_dataset_names: list) -> list:
        """Returns the functions to use for the new datasets. It tries to use the existing functions if possible."""
        if len(self.fitting_context.dataset_names) == len(new_dataset_names):
            return [self._clear_function_errors(function) for function in self.fitting_context.single_fit_functions]
        else:
            return [self._get_new_function_for(name) for name in new_dataset_names]

    def _get_new_function_for(self, new_dataset_name: str) -> IFunction:
        """Returns the function to use for the new dataset. It tries to use an existing function if possible."""
        dataset_names = self.fitting_context.dataset_names
        if new_dataset_name in dataset_names:
            return self._clear_function_errors(self._clone_function(
                self.fitting_context.single_fit_functions[dataset_names.index(new_dataset_name)]))
        else:
            return self._clear_function_errors(self._clone_function(self.current_single_fit_function))

    def retrieve_first_good_data_from(self, workspace_name: str) -> float:
        """Returns the first good data value from a workspace."""
        try:
            return self.context.first_good_data([float(re.search("[0-9]+", workspace_name).group())])
        except AttributeError:
            x_lower, _ = self.x_limits_of_workspace(workspace_name)
            return x_lower

    def get_fit_function_parameters(self) -> list:
        """Returns the names of the fit parameters in the fit functions."""
        single_fit_functions = self.fitting_context.single_fit_functions
        if single_fit_functions:
            fit_function = single_fit_functions[0]
            if fit_function is not None:
                return [fit_function.parameterName(i) for i in range(fit_function.nParams())]
        return []

    def get_all_fit_functions(self) -> list:
        """Returns all the fit functions for the current fitting mode."""
        return self.fitting_context.single_fit_functions

    def get_active_fit_function(self) -> IFunction:
        """Returns the fit function that is active and will be used for a fit."""
        return self.current_single_fit_function

    def get_active_workspace_names(self) -> list:
        """Returns the names of the workspaces that will be fitted. Single Fit mode only fits the selected workspace."""
        return [self.current_dataset_name]

    def get_active_fit_results(self) -> list:
        """Returns the results of the currently active fit. For Single Fit there is only one fit result."""
        if self.fitting_context.number_of_datasets > 0:
            return self._create_fit_plot_information(self.get_active_workspace_names(),
                                                     self.fitting_context.function_name)
        else:
            return []

    def get_workspace_names_to_display_from_context(self) -> list:
        """Returns the workspace names to display in the view based on the selected run and group/pair options."""
        runs, groups_and_pairs = self.get_selected_runs_groups_and_pairs()
        workspace_names = self.context.get_workspace_names_for(runs, groups_and_pairs, self.fitting_context.fit_to_raw)

        return self._check_data_exists(workspace_names)

    @staticmethod
    def _check_data_exists(workspace_names: list) -> list:
        """Returns only the workspace names that exist in the ADS."""
        return [workspace_name for workspace_name in workspace_names if check_if_workspace_exist(workspace_name)]

    @staticmethod
    def is_equal_to_n_decimals(value1: float, value2: float, n_decimals: int) -> bool:
        """Checks that two floats are equal up to n decimal places."""
        return f"{value1:.{n_decimals}f}" == f"{value2:.{n_decimals}f}"

    def get_selected_runs_groups_and_pairs(self) -> tuple:
        """Returns the runs, groups and pairs to use for single fit mode."""
        return "All", self._get_selected_groups_and_pairs()

    def _get_selected_groups_and_pairs(self) -> list:
        """Returns the groups and pairs currently selected in the context."""
        return self.context.group_pair_context.selected_groups_and_pairs

    def perform_fit(self) -> tuple:
        """Performs a single fit and returns the resulting function, status and chi squared."""
        params = self._get_parameters_for_single_fit(self.current_dataset_name, self.current_single_fit_function)
        return self._do_single_fit(params)

    def _do_single_fit(self, parameters: dict) -> tuple:
        """Does a single fit and returns the fit function, status and chi squared. Adds the results to the ADS."""
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = \
            self._do_single_fit_and_return_workspace_parameters_and_fit_function(parameters)

        self._add_single_fit_results_to_ADS_and_context(parameters["InputWorkspace"], parameter_table, output_workspace,
                                                        covariance_matrix)
        return function, fit_status, chi_squared

    def _do_single_fit_and_return_workspace_parameters_and_fit_function(self, parameters: dict) -> tuple:
        """Does a single fit and returns the fit function, status and chi squared."""
        alg = self._create_fit_algorithm()
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = run_Fit(parameters,
                                                                                                          alg)
        CopyLogs(InputWorkspace=parameters["InputWorkspace"], OutputWorkspace=output_workspace, StoreInADS=False)
        return output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix

    def _get_parameters_for_single_fit(self, dataset_name: str, single_fit_function: IFunction) -> dict:
        """Returns the parameters used for a single fit."""
        params = self._get_common_parameters()
        params["Function"] = single_fit_function
        params["InputWorkspace"] = dataset_name
        params["StartX"] = self.current_start_x
        params["EndX"] = self.current_end_x
        return params

    def _get_common_parameters(self) -> dict:
        """Returns the parameters which are common across different fitting modes."""
        return {"Minimizer": self.fitting_context.minimizer,
                "EvaluationType": self.fitting_context.evaluation_type}

    def _create_fit_algorithm(self) -> IAlgorithm:
        """Creates the Fit or DoublePulseFit algorithm depending if Double Pulse fit is selected."""
        if self._double_pulse_enabled():
            return self._create_double_pulse_alg()
        else:
            return AlgorithmManager.create("Fit")

    def _create_double_pulse_alg(self) -> IAlgorithm:
        """Creates the DoublePulseFit algorithm."""
        offset = self.context.gui_context['DoublePulseTime']
        first_pulse_weighting, second_pulse_weighting = self._get_pulse_weightings(offset, 2.2)

        alg = AlgorithmManager.create("DoublePulseFit")
        alg.setProperty("PulseOffset", offset)
        alg.setProperty("FirstPulseWeight", first_pulse_weighting)
        alg.setProperty("SecondPulseWeight", second_pulse_weighting)
        return alg

    def _double_pulse_enabled(self) -> bool:
        """Returns true of Double Pulse fit is selected in the context."""
        return self.fitting_context.allow_double_pulse_fitting and self.context.do_double_pulse_fit()

    @staticmethod
    def _get_pulse_weightings(offset: float, muon_halflife: float) -> tuple:
        """Returns the pulse weightings used for a double pulse fit."""
        decay = math.exp(-offset / muon_halflife)
        first_pulse_weighting = decay / (1 + decay)
        second_pulse_weighting = 1 / (1 + decay)
        return first_pulse_weighting, second_pulse_weighting

    def _add_single_fit_results_to_ADS_and_context(self, input_workspace_name: str, parameters_table, output_workspace,
                                                   covariance_matrix) -> None:
        """Adds the results of a single fit to the ADS and context."""
        function_name = self.fitting_context.function_name

        output_workspace_name, directory = create_fitted_workspace_name(input_workspace_name, function_name)
        parameter_table_name, _ = create_parameter_table_name(input_workspace_name, function_name)
        covariance_matrix_name, _ = create_covariance_matrix_name(input_workspace_name, function_name)

        self._add_workspace_to_ADS(output_workspace, output_workspace_name, directory)
        self._add_workspace_to_ADS(parameters_table, parameter_table_name, directory)
        self._add_workspace_to_ADS(covariance_matrix, covariance_matrix_name, directory)

        output_workspace_wrap = StaticWorkspaceWrapper(output_workspace_name, retrieve_ws(output_workspace_name))
        parameter_workspace_wrap = StaticWorkspaceWrapper(parameter_table_name, retrieve_ws(parameter_table_name))
        covariance_workspace_wrap = StaticWorkspaceWrapper(covariance_matrix_name, retrieve_ws(covariance_matrix_name))

        self._add_fit_to_context([input_workspace_name], [output_workspace_wrap], parameter_workspace_wrap,
                                 covariance_workspace_wrap)

    def _add_fit_to_context(self, input_workspace_names: list, output_workspaces: list,
                            parameter_workspace: StaticWorkspaceWrapper, covariance_workspace: StaticWorkspaceWrapper) -> None:
        """Adds the results of a single fit to the context."""
        self.fitting_context.add_fit_from_values(input_workspace_names, self.fitting_context.function_name,
                                                 output_workspaces, parameter_workspace, covariance_workspace)

    def _create_fit_plot_information(self, workspace_names: list, function_name: str) -> list:
        """Creates the FitPlotInformation storing fit data to be plotted in the plot widget."""
        return [FitPlotInformation(input_workspaces=workspace_names,
                                   fit=self._get_fit_results_from_context(workspace_names, function_name))]

    def _get_fit_results_from_context(self, workspace_names: list, function_name: str) -> FitInformation:
        """Gets the fit results from the context using the workspace names and function name."""
        return self.fitting_context.find_fit_for_input_workspace_list_and_function(workspace_names, function_name)

    def _get_equivalent_binned_or_unbinned_workspaces(self):
        """Returns the equivalent binned or unbinned workspaces for the current datasets."""
        return self.context.get_list_of_binned_or_unbinned_workspaces_from_equivalents(
            self.fitting_context.dataset_names)

    @staticmethod
    def _clone_function(function: IFunction) -> IFunction:
        """Make a clone of the function provided."""
        if function is None:
            return function
        return function.clone()

    @staticmethod
    def _get_function_name(function: IFunction) -> str:
        """Returns the function name to use for saving fit results based on the function provided."""
        if function is None:
            return ""
        if function.getNumberDomains() > 1:
            function = function.getFunction(0)

        try:
            return get_function_name_for_composite(function)
        except AttributeError:
            return function.name()

    def _evaluate_plot_guess(self, plot_guess: bool) -> str:
        """Evaluate the plot guess fit function and returns the name of the resulting guess workspace."""
        if not plot_guess or self.current_dataset_name is None:
            return ""

        fit_function = self._get_plot_guess_fit_function()
        if fit_function is not None:
            return self._evaluate_function(fit_function, self._get_plot_guess_name())
        return ""

    def _evaluate_function(self, fit_function: IFunction, output_workspace: str) -> str:
        """Evaluate the plot guess fit function and returns the name of the resulting guess workspace."""
        try:
            if self._double_pulse_enabled():
                self._evaluate_double_pulse_function(fit_function, output_workspace)
            else:
                EvaluateFunction(InputWorkspace=self.current_dataset_name,
                                 Function=fit_function,
                                 StartX=self.current_start_x,
                                 EndX=self.current_end_x,
                                 OutputWorkspace=output_workspace)
        except RuntimeError:
            logger.error("Failed to plot guess.")
            return ""
        return output_workspace

    def _evaluate_double_pulse_function(self, fit_function: IFunction, output_workspace: str) -> None:
        """Evaluate the plot guess fit function for a double pulse fit. It does this by setting MaxIterations to 1."""
        alg = self._create_double_pulse_alg()
        alg.initialize()
        alg.setAlwaysStoreInADS(True)
        alg.setProperty("Function", fit_function)
        alg.setProperty("InputWorkspace", self.current_dataset_name)
        alg.setProperty("StartX", self.current_start_x)
        alg.setProperty("EndX", self.current_end_x)
        alg.setProperty("Minimizer", self.fitting_context.minimizer)
        alg.setProperty("EvaluationType", self.fitting_context.evaluation_type)
        alg.setProperty("MaxIterations", 0)
        alg.setProperty("Output", output_workspace)
        alg.execute()

    def _get_plot_guess_name(self) -> str:
        """Returns the name to use for the plot guess workspace."""
        return self.context.guess_workspace_prefix + self.current_dataset_name

    def _get_plot_guess_fit_function(self) -> IFunction:
        """Returns the fit function to evaluate when plotting a guess."""
        return self.get_active_fit_function()

    def _add_workspace_to_ADS(self, workspace, name: str, directory: str) -> MuonWorkspaceWrapper:
        """Add a Muon-wrapped workspace to the ADS."""
        self.context.ads_observer.observeRename(False)
        workspace_wrapper = MuonWorkspaceWrapper(workspace)
        workspace_wrapper.show(directory + name)
        self.context.ads_observer.observeRename(True)
        return workspace_wrapper
