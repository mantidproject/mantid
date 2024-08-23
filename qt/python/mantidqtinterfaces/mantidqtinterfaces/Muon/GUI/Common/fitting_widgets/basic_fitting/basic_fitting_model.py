# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import AlgorithmManager, logger
from mantid.api import CompositeFunction, IAlgorithm, IFunction
from mantid.simpleapi import CopyLogs
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_group_definition import add_list_to_group

from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist, retrieve_ws, make_group
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import (
    check_phasequad_name,
    create_covariance_matrix_name,
    create_fitted_workspace_name,
    create_parameter_table_name,
    get_diff_asymmetry_name,
    get_group_asymmetry_name,
    get_group_or_pair_from_name,
    get_pair_asymmetry_name,
    get_pair_phasequad_name,
    get_run_numbers_as_string_from_workspace_name,
)
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.basic_fitting_context import (
    BasicFittingContext,
    X_FROM_FIT_RANGE,
    X_FROM_DATA_RANGE,
    X_FROM_CUSTOM,
)
from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.fitting_context import FitInformation
from mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context import MuonContext
from mantidqtinterfaces.Muon.GUI.Common.utilities.algorithm_utils import run_Fit, evaluate_function, run_create_workspace
from mantidqtinterfaces.Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
from mantidqtinterfaces.Muon.GUI.Common.utilities.workspace_data_utils import check_exclude_start_and_end_x_is_valid, x_limits_of_workspace
from mantidqtinterfaces.Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper

import math
import numpy as np
import re
from typing import List, NamedTuple

DEFAULT_CHI_SQUARED = 0.0
DEFAULT_FIT_STATUS = None
DEFAULT_SINGLE_FIT_FUNCTION = None
DEFAULT_START_X = 0.0


def get_function_name_for_composite(composite: CompositeFunction) -> str:
    """Gets the function name to associate with a composite function when saving fit results."""
    assert isinstance(composite, CompositeFunction), "Expected to get function name of a CompositeFunction."

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

        exclude_start_xs, exclude_end_xs = self._get_new_exclude_start_xs_and_end_xs_using_existing_data()
        self.fitting_context.exclude_start_xs = exclude_start_xs
        self.fitting_context.exclude_end_xs = exclude_end_xs

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

    def current_normalised_covariance_matrix(self) -> StaticWorkspaceWrapper:
        """Returns the Normalised Covariance matrix for the currently selected data."""
        return self._get_normalised_covariance_matrix_for(self.get_active_workspace_names(), self.fitting_context.function_name)

    def has_normalised_covariance_matrix(self) -> bool:
        """Returns true if a Normalised Covariance matrix exists for the currently selected dataset."""
        return self.current_normalised_covariance_matrix() is not None

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

    def set_current_start_and_end_x(self, start_x, end_x):
        """Need to set these together because of the checks
        to make sure that start < end"""
        if start_x > end_x:
            return
        elif start_x > self.current_end_x:
            self.current_end_x = end_x
            self.current_start_x = start_x
        else:
            self.current_start_x = start_x
            self.current_end_x = end_x

    @property
    def exclude_range(self) -> bool:
        """Returns true if the Exclude Range option is on in the context."""
        return self.fitting_context.exclude_range

    @exclude_range.setter
    def exclude_range(self, exclude_range_state: bool) -> None:
        """Sets whether the Exclude Range option is on in the context."""
        self.fitting_context.exclude_range = exclude_range_state

    @property
    def current_exclude_start_x(self) -> float:
        """Returns the currently selected exclude start X, or the default exclude start X if nothing is selected."""
        current_dataset_index = self.fitting_context.current_dataset_index
        if current_dataset_index is not None:
            return self.fitting_context.exclude_start_xs[current_dataset_index]
        else:
            return self.current_end_x

    @current_exclude_start_x.setter
    def current_exclude_start_x(self, value: float) -> None:
        """Sets the value of the currently selected exclude start X."""
        if value < self.current_exclude_end_x:
            self.fitting_context.exclude_start_xs[self.fitting_context.current_dataset_index] = value

    @property
    def current_exclude_end_x(self) -> float:
        """Returns the currently selected exclude start X, or the default exclude start X if nothing is selected."""
        current_dataset_index = self.fitting_context.current_dataset_index
        if current_dataset_index is not None:
            return self.fitting_context.exclude_end_xs[current_dataset_index]
        else:
            return self.current_end_x

    @current_exclude_end_x.setter
    def current_exclude_end_x(self, value: float) -> None:
        """Sets the value of the currently selected exclude end X."""
        if value > self.current_exclude_start_x:
            self.fitting_context.exclude_end_xs[self.fitting_context.current_dataset_index] = value

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
        self.fitting_context.single_fit_functions_for_undo.append(self._clone_function(self.current_single_fit_function))
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
            if dataset_index == current_dataset_index and i < len(self.fitting_context.active_fit_history):
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
    def plot_guess_type(self) -> str:
        """Returns the guess plot range type."""
        return self.fitting_context.plot_guess_type

    @plot_guess_type.setter
    def plot_guess_type(self, plot_guess_type: str) -> None:
        """Sets the guess plot range type."""
        self.fitting_context.plot_guess_type = plot_guess_type

    @property
    def plot_guess_points(self) -> int:
        """Returns the number of points to use in the guess plot."""
        return self.fitting_context.plot_guess_points

    @plot_guess_points.setter
    def plot_guess_points(self, plot_guess_points: int) -> None:
        """Sets the number of points to use in the guess plot."""
        self.fitting_context.plot_guess_points = plot_guess_points

    @property
    def plot_guess_start_x(self) -> float:
        """Returns the start x to use in the guess plot."""
        return self.fitting_context.plot_guess_start_x

    @plot_guess_start_x.setter
    def plot_guess_start_x(self, plot_guess_start_x: float) -> None:
        """Sets the start x to use in the guess plot."""
        self.fitting_context.plot_guess_start_x = plot_guess_start_x

    @property
    def plot_guess_end_x(self) -> float:
        """Returns the end x to use in the guess plot."""
        return self.fitting_context.plot_guess_start_x

    @plot_guess_end_x.setter
    def plot_guess_end_x(self, plot_guess_end_x: float) -> None:
        """Sets the end x to use in the guess plot."""
        self.fitting_context.plot_guess_end_x = plot_guess_end_x

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

    def update_attribute_value(self, full_attribute: str, value: float) -> None:
        """Update the value of an attribute in the fit function."""
        if self.current_single_fit_function is not None:
            self.current_single_fit_function.setAttributeValue(full_attribute, value)

    @property
    def do_rebin(self) -> bool:
        """Returns true if rebin is selected within the context."""
        return self.context._do_rebin()

    def x_limits_of_workspace(self, workspace_name: str) -> tuple:
        """Returns the x data limits of a provided workspace or the current dataset."""
        return x_limits_of_workspace(workspace_name, (self.current_start_x, self.current_end_x))

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
        elif (
            self.fitting_context.current_dataset_index is None
            or self.fitting_context.current_dataset_index >= self.fitting_context.number_of_datasets
        ):
            self.fitting_context.current_dataset_index = 0

    def reset_start_xs_and_end_xs(self) -> None:
        """Resets the start and end Xs stored by the context."""
        self._reset_start_xs()
        self._reset_end_xs()
        self._reset_exclude_start_and_end_xs()

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
            self.fitting_context.start_xs = [self.retrieve_first_good_data_from(name) for name in self.fitting_context.dataset_names]
        else:
            self.fitting_context.start_xs = []

    def _reset_end_xs(self) -> None:
        """Resets the end Xs stored by the context."""
        self.fitting_context.end_xs = [self.current_end_x] * self.fitting_context.number_of_datasets

    def _reset_exclude_start_and_end_xs(self) -> None:
        """Resets the exclude start and end Xs. Default exclude range is to have exclude_start_x == exclude_end_x."""
        self.fitting_context.exclude_start_xs = [self.current_end_x] * self.fitting_context.number_of_datasets
        self.fitting_context.exclude_end_xs = [self.current_end_x] * self.fitting_context.number_of_datasets

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
            return (
                self.current_start_x if x_lower < self.current_start_x < x_upper else self.retrieve_first_good_data_from(new_dataset_name)
            )

    def _get_new_end_x_for(self, new_dataset_name: str) -> float:
        """Returns the end X to use for the new dataset. It tries to use an existing end X if possible."""
        dataset_names = self.fitting_context.dataset_names
        if new_dataset_name in dataset_names:
            return self.fitting_context.end_xs[dataset_names.index(new_dataset_name)]
        else:
            x_lower, x_upper = self.x_limits_of_workspace(new_dataset_name)
            return self.current_end_x if x_lower < self.current_end_x < x_upper else x_upper

    def _get_new_exclude_start_xs_and_end_xs_using_existing_data(self) -> tuple:
        """Returns the exclude start and end Xs to use for the new data. It tries to use existing ranges if possible."""
        exclude_start_xs, exclude_end_xs = [], []
        for dataset_index in range(self.fitting_context.number_of_datasets):
            exclude_start_x, exclude_end_x = self._get_new_exclude_start_and_end_x_for(dataset_index)
            exclude_start_xs.append(exclude_start_x)
            exclude_end_xs.append(exclude_end_x)
        return exclude_start_xs, exclude_end_xs

    def _get_new_exclude_start_and_end_x_for(self, dataset_index: int) -> tuple:
        """Gets the new exclude start and end X to use for a specific dataset. It tries to use the current data."""
        start_x, end_x = self.fitting_context.start_xs[dataset_index], self.fitting_context.end_xs[dataset_index]
        exclude_start_xs, exclude_end_xs = self.fitting_context.exclude_start_xs, self.fitting_context.exclude_end_xs

        new_exclude_start_x = exclude_start_xs[dataset_index] if dataset_index < len(exclude_start_xs) else end_x
        new_exclude_end_x = exclude_end_xs[dataset_index] if dataset_index < len(exclude_end_xs) else end_x

        return check_exclude_start_and_end_x_is_valid(start_x, end_x, new_exclude_start_x, new_exclude_end_x)

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
            return self._clear_function_errors(
                self._clone_function(self.fitting_context.single_fit_functions[dataset_names.index(new_dataset_name)])
            )
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

    def get_active_fit_function(self) -> IFunction:
        """Returns the fit function that is active and will be used for a fit."""
        return self.current_single_fit_function

    def get_active_workspace_names(self) -> list:
        """Returns the names of the workspaces that will be fitted. Single Fit mode only fits the selected workspace."""
        return [self.current_dataset_name]

    def get_active_fit_results(self) -> list:
        """Returns the results of the currently active fit. For Single Fit there is only one fit result."""
        if self.fitting_context.number_of_datasets > 0:
            return self._create_fit_plot_information(self.get_active_workspace_names(), self.fitting_context.function_name)
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
        (
            output_workspace,
            parameter_table,
            function,
            fit_status,
            chi_squared,
            covariance_matrix,
        ) = self._do_single_fit_and_return_workspace_parameters_and_fit_function(parameters)

        self._add_single_fit_results_to_ADS_and_context(parameters["InputWorkspace"], parameter_table, output_workspace, covariance_matrix)
        return function, fit_status, chi_squared

    def _do_single_fit_and_return_workspace_parameters_and_fit_function(self, parameters: dict) -> tuple:
        """Does a single fit and returns the fit function, status and chi squared."""
        alg = self._create_fit_algorithm()
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = run_Fit(parameters, alg)
        CopyLogs(InputWorkspace=parameters["InputWorkspace"], OutputWorkspace=output_workspace, StoreInADS=False)
        return output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix

    def _get_parameters_for_single_fit(self, dataset_name: str, single_fit_function: IFunction) -> dict:
        """Returns the parameters used for a single fit."""
        params = self._get_common_parameters()
        params["Function"] = single_fit_function
        params["InputWorkspace"] = dataset_name
        params["StartX"] = self.current_start_x
        params["EndX"] = self.current_end_x
        if self.fitting_context.exclude_range:
            params["Exclude"] = [self.current_exclude_start_x, self.current_exclude_end_x]
        return params

    def _get_common_parameters(self) -> dict:
        """Returns the parameters which are common across different fitting modes."""
        return {"Minimizer": self.fitting_context.minimizer, "EvaluationType": self.fitting_context.evaluation_type}

    def _create_fit_algorithm(self) -> IAlgorithm:
        """Creates the Fit or DoublePulseFit algorithm depending if Double Pulse fit is selected."""
        if self._double_pulse_enabled():
            return self._create_double_pulse_alg()
        else:
            return AlgorithmManager.create("Fit")

    def _create_double_pulse_alg(self) -> IAlgorithm:
        """Creates the DoublePulseFit algorithm."""
        offset = self.context.gui_context["DoublePulseTime"]
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

    def _add_workspaces_to_group(self, ws_names, group_name):
        if check_if_workspace_exist(group_name):
            add_list_to_group(ws_names, retrieve_ws(group_name))
        else:
            make_group(ws_names, group_name)

    def _add_single_fit_results_to_ADS_and_context(
        self, input_workspace_name: str, parameters_table, output_workspace, covariance_matrix
    ) -> None:
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

        self._add_workspaces_to_group([output_workspace_name, parameter_table_name, covariance_matrix_name], directory[:-1])
        self._add_fit_to_context([input_workspace_name], [output_workspace_wrap], parameter_workspace_wrap, covariance_workspace_wrap)

    def _add_fit_to_context(
        self,
        input_workspace_names: list,
        output_workspaces: list,
        parameter_workspace: StaticWorkspaceWrapper,
        covariance_workspace: StaticWorkspaceWrapper,
    ) -> None:
        """Adds the results of a single fit to the context."""
        self.fitting_context.add_fit_from_values(
            input_workspace_names, self.fitting_context.function_name, output_workspaces, parameter_workspace, covariance_workspace
        )

    def _create_fit_plot_information(self, workspace_names: list, function_name: str) -> list:
        """Creates the FitPlotInformation storing fit data to be plotted in the plot widget."""
        return [
            FitPlotInformation(input_workspaces=workspace_names, fit=self._get_fit_results_from_context(workspace_names, function_name))
        ]

    def _get_normalised_covariance_matrix_for(self, workspace_names: list, function_name: str) -> StaticWorkspaceWrapper:
        """Returns the Normalised Covariance Matrix associated with some input workspaces and a function name."""
        fit = self._get_fit_results_from_context(workspace_names, function_name)
        return fit.covariance_workspace if fit is not None else None

    def _get_fit_results_from_context(self, workspace_names: list, function_name: str) -> FitInformation:
        """Gets the fit results from the context using the workspace names and function name."""
        return self.fitting_context.find_fit_for_input_workspace_list_and_function(workspace_names, function_name)

    def _get_equivalent_binned_or_unbinned_workspaces(self):
        """Returns the equivalent binned or unbinned workspaces for the current datasets."""
        return self.context.get_list_of_binned_or_unbinned_workspaces_from_equivalents(self.fitting_context.dataset_names)

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

        if isinstance(function, CompositeFunction):
            return get_function_name_for_composite(function)
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
        if self.fitting_context.plot_guess_type == X_FROM_FIT_RANGE:
            data_ws = retrieve_ws(self.current_dataset_name)
            data = np.linspace(self.current_start_x, self.current_end_x, data_ws.getNumberBins())
        elif self.fitting_context.plot_guess_type == X_FROM_DATA_RANGE:
            data_ws = retrieve_ws(self.current_dataset_name)
            data = np.linspace(data_ws.dataX(0)[0], data_ws.dataX(0)[-1], self.fitting_context.plot_guess_points)
        elif self.fitting_context.plot_guess_type == X_FROM_CUSTOM:
            data = np.linspace(
                self.fitting_context.plot_guess_start_x, self.fitting_context.plot_guess_end_x, self.fitting_context.plot_guess_points
            )
        else:
            raise ValueError(f"Plot guess type '{self.fitting_context.plot_guess_type}' is not recognised.")

        try:
            if self._double_pulse_enabled():
                self._evaluate_double_pulse_function(fit_function, output_workspace)
            else:
                extended_workspace = run_create_workspace(x_data=data, y_data=data, name="extended_workspace")
                evaluate_function(extended_workspace, fit_function, output_workspace)
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

    @staticmethod
    def get_fit_function_parameter_values(fit_function: IFunction) -> list:
        """Get all the parameter values within a given fit function."""
        parameters, errors = [], []
        if fit_function is not None:
            for i in range(fit_function.nParams()):
                parameters.append(fit_function.getParameterValue(i))
                errors.append(fit_function.getError(i))
        return parameters, errors

    """
    Methods used by the Sequential Fitting Tab
    """

    def validate_sequential_fit(self, workspace_names: list) -> str:
        if self.get_active_fit_function() is None or len(workspace_names) == 0:
            return "No data or fit function selected for fitting."
        else:
            return ""

    def get_runs_groups_and_pairs_for_fits(self, display_type: str) -> tuple:
        """Returns the runs and group/pairs corresponding to the selected dataset names."""
        runs, groups_and_pairs = [], []
        for name in self.fitting_context.dataset_names:
            runs.append(get_run_numbers_as_string_from_workspace_name(name, self.context.data_context.instrument))
            groups_and_pairs.append(get_group_or_pair_from_name(name))
        return self._get_datasets_containing_string(display_type, self.fitting_context.dataset_names, runs, groups_and_pairs)

    @staticmethod
    def _get_datasets_containing_string(display_type: str, dataset_names: list, *corresponding_dataset_args) -> tuple:
        """Returns the dataset names that contain a string and returns its associated runs/groups/pairs."""
        if display_type == "All":
            return (dataset_names, *corresponding_dataset_args)

        # Filter the data based on a name in the dataset_names containing a string
        filtered_data = list(zip(*filter(lambda x: display_type in x[0], zip(dataset_names, *corresponding_dataset_args))))

        # Return a tuple of empty lists if the filtered data is empty
        return tuple(filtered_data) if len(filtered_data) != 0 else ([] for _ in range(len(corresponding_dataset_args) + 1))

    def get_all_fit_function_parameter_values_for(self, fit_function: IFunction) -> list:
        """Returns the values of the fit function parameters."""
        parameter_values, _ = self.get_fit_function_parameter_values(fit_function)
        return parameter_values

    @staticmethod
    def _set_fit_function_parameter_values(fit_function: IFunction, parameter_values: list, errors: list = None) -> None:
        """Set the parameter values within a fit function."""
        for i in range(fit_function.nParams()):
            fit_function.setParameter(i, parameter_values[i])
            if errors is not None:
                fit_function.setError(i, errors[i])

    def get_all_fit_functions(self) -> list:
        """Returns all the fit functions for the current fitting mode."""
        return self.fitting_context.single_fit_functions

    def get_all_fit_functions_for(self, display_type: str) -> list:
        """Returns all the fit functions for datasets with a name containing a string."""
        return self._filter_functions_by_dataset_string(display_type, self.single_fit_functions)

    def _filter_functions_by_dataset_string(self, display_type: str, fit_functions: list) -> list:
        """Filters out the fit functions corresponding to dataset names that do not contain a string."""
        if display_type == "All":
            return self.get_all_fit_functions()
        else:
            _, filtered_functions = self._get_datasets_containing_string(display_type, self.dataset_names, fit_functions)
            return filtered_functions

    def get_fit_workspace_names_from_groups_and_runs(self, runs: list, groups_and_pairs: list) -> list:
        """Returns the workspace names to use for the given runs and groups/pairs."""
        workspace_names = []
        for run in runs:
            for group_or_pair in groups_and_pairs:
                workspace_names += self._get_workspace_name_from_run_and_group_or_pair(run, group_or_pair)
        return workspace_names

    def _get_workspace_name_from_run_and_group_or_pair(self, run: str, group_or_pair: str) -> list:
        """Returns the workspace name to use for the given run and group/pair."""
        fit_to_raw = self.fitting_context.fit_to_raw
        if check_phasequad_name(group_or_pair) and group_or_pair in self.context.group_pair_context.selected_pairs:
            return [get_pair_phasequad_name(self.context, group_or_pair, run, not fit_to_raw)]
        elif group_or_pair in self.context.group_pair_context.selected_pairs:
            return [get_pair_asymmetry_name(self.context, group_or_pair, run, not fit_to_raw)]
        elif group_or_pair in self.context.group_pair_context.selected_diffs:
            return [get_diff_asymmetry_name(self.context, group_or_pair, run, not fit_to_raw)]
        elif group_or_pair in self.context.group_pair_context.selected_groups:
            period_string = run_list_to_string(self.context.group_pair_context[group_or_pair].periods)
            return [get_group_asymmetry_name(self.context, group_or_pair, run, period_string, not fit_to_raw)]
        else:
            return []

    def update_ws_fit_function_parameters(self, dataset_names: list, parameter_values: list) -> None:
        """Updates the function parameter values for the given dataset names."""
        self._update_fit_function_parameters_for_single_fit(dataset_names, parameter_values)

    def _update_fit_function_parameters_for_single_fit(self, dataset_names: list, parameter_values: list) -> None:
        """Updates the function parameters for the given dataset names if in single fit mode."""
        for name in dataset_names:
            fit_function = self.get_single_fit_function_for(name)
            if fit_function is not None:
                self._set_fit_function_parameter_values(fit_function, parameter_values)

    def perform_sequential_fit(self, workspaces: list, parameter_values: list, use_initial_values: bool = False) -> tuple:
        """Performs a sequential fit of the workspace names provided for single fitting mode.

        :param workspaces: A list of lists of workspace names e.g. [[Row 1 workspaces], [Row 2 workspaces], etc...]
        :param parameter_values: A list of lists of parameter values e.g. [[Row 1 params], [Row 2 params], etc...]
        :param use_initial_values: If false the parameters at the end of each fit are passed on to the next fit.
        """
        workspaces = self._flatten_workspace_names(workspaces)
        return self._perform_sequential_fit_using_func(self._do_sequential_fit, workspaces, parameter_values, use_initial_values)

    def _perform_sequential_fit_using_func(
        self, fitting_func, workspaces: list, parameter_values: list, use_initial_values: bool = False
    ) -> tuple:
        """Performs a sequential fit of the workspace names provided for the current fitting mode."""
        functions, fit_statuses, chi_squared_list = self._evaluate_sequential_fit(
            fitting_func, workspaces, parameter_values, use_initial_values
        )

        self._update_fit_functions_after_sequential_fit(workspaces, functions)
        self._update_fit_statuses_and_chi_squared_after_sequential_fit(workspaces, fit_statuses, chi_squared_list)
        return functions, fit_statuses, chi_squared_list

    def _do_sequential_fit(
        self, row_index: int, workspace_name: str, parameter_values: list, functions: list, use_initial_values: bool = False
    ):
        """Performs a sequential fit of the single fit data."""
        single_function = (
            functions[row_index - 1].clone()
            if not use_initial_values and row_index >= 1
            else self._get_single_function_with_parameters(parameter_values)
        )

        params = self._get_parameters_for_single_fit(workspace_name, single_function)

        return self._do_single_fit(params)

    def _get_single_function_with_parameters(self, parameter_values: list) -> IFunction:
        """Returns the current single fit function but with the parameter values provided."""
        single_fit_function = self.current_single_fit_function.clone()
        self._set_fit_function_parameter_values(single_fit_function, parameter_values)
        return single_fit_function

    @staticmethod
    def _evaluate_sequential_fit(fitting_func, workspace_names: list, parameter_values: list, use_initial_values: bool = False):
        """Evaluates a sequential fit using the provided fitting func. The workspace_names is either a 1D or 2D list."""
        functions, fit_statuses, chi_squared_list = [], [], []

        for row_index, row_workspaces in enumerate(workspace_names):
            function, fit_status, chi_squared = fitting_func(
                row_index, row_workspaces, parameter_values[row_index], functions, use_initial_values
            )

            functions.append(function)
            fit_statuses.append(fit_status)
            chi_squared_list.append(chi_squared)

        return functions, fit_statuses, chi_squared_list

    def _update_fit_functions_after_sequential_fit(self, workspaces: list, functions: list) -> None:
        """Updates the fit functions after a sequential fit has been run on the Sequential fitting tab."""
        dataset_names = self.fitting_context.dataset_names

        for workspace_index, workspace_name in enumerate(workspaces):
            if workspace_name in dataset_names:
                dataset_index = dataset_names.index(workspace_name)
                self.fitting_context.single_fit_functions[dataset_index] = functions[workspace_index]

    def _update_fit_statuses_and_chi_squared_after_sequential_fit(self, workspaces, fit_statuses, chi_squared_list):
        """Updates the fit statuses and chi squared after a sequential fit."""
        dataset_names = self.fitting_context.dataset_names

        for workspace_index, workspace_name in enumerate(workspaces):
            if workspace_name in dataset_names:
                dataset_index = dataset_names.index(workspace_name)
                self.fitting_context.fit_statuses[dataset_index] = fit_statuses[workspace_index]
                self.fitting_context.chi_squared[dataset_index] = chi_squared_list[workspace_index]

    @staticmethod
    def _flatten_workspace_names(workspaces: list) -> list:
        """Provides a workspace name list of lists to be flattened if in single fitting mode."""
        return [workspace for fit_workspaces in workspaces for workspace in fit_workspaces]
