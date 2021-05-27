# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist
from Muon.GUI.Common.contexts.fitting_contexts.fitting_context import FittingContext


class BasicFittingContext(FittingContext):

    def __init__(self, allow_double_pulse_fitting: bool = False):
        super(BasicFittingContext, self).__init__()

        self._allow_double_pulse_fitting: bool = allow_double_pulse_fitting

        self._current_dataset_index: int = None
        self._dataset_names: list = []

        self._start_xs: list = []
        self._end_xs: list = []

        self._single_fit_functions: list = []
        self._single_fit_functions_cache: list = []

        self._fit_statuses: list = []
        self._fit_statuses_cache: list = []

        self._chi_squared: list = []
        self._chi_squared_cache: list = []

        self._plot_guess: bool = False
        self._guess_workspace_name: str = None

        self._function_name: str = ""
        self._function_name_auto_update: bool = True

        self._minimizer: str = ""
        self._evaluation_type: str = ""
        self._fit_to_raw: bool = True

    @property
    def allow_double_pulse_fitting(self) -> bool:
        """Returns true if double pulse fitting should be allowed in the model owning this fitting context."""
        return self._allow_double_pulse_fitting

    @property
    def current_dataset_index(self) -> int:
        """Returns the index of the currently selected dataset."""
        return self._current_dataset_index

    @current_dataset_index.setter
    def current_dataset_index(self, index: int) -> None:
        """Sets the index of the currently selected dataset."""
        if index is not None and index >= self.number_of_datasets:
            raise RuntimeError(f"The provided dataset index ({index}) is too large.")

        self._current_dataset_index = index

    @property
    def dataset_names(self) -> list:
        """Returns the names of all the datasets stored in the model."""
        return self._dataset_names

    @dataset_names.setter
    def dataset_names(self, names: list) -> None:
        """Sets the dataset names stored by the model. Resets the other fitting data."""
        self._dataset_names = names

    @property
    def number_of_datasets(self) -> int:
        """Returns the number of datasets stored by the model."""
        return len(self.dataset_names)

    @property
    def start_xs(self) -> list:
        """Returns all of the start Xs stored by the model."""
        return self._start_xs

    @start_xs.setter
    def start_xs(self, start_xs: list) -> None:
        """Sets all of the start Xs in the model."""
        if len(start_xs) != self.number_of_datasets:
            raise RuntimeError(f"The provided number of start Xs is not equal to the number of datasets.")

        self._start_xs = start_xs

    @property
    def end_xs(self) -> list:
        """Returns all of the end Xs stored by the model."""
        return self._end_xs

    @end_xs.setter
    def end_xs(self, end_xs: list) -> None:
        """Sets all of the end Xs in the model."""
        if len(end_xs) != self.number_of_datasets:
            raise RuntimeError(f"The provided number of end Xs is not equal to the number of datasets.")

        self._end_xs = end_xs

    @property
    def single_fit_functions(self) -> list:
        """Returns all of the fit functions used for single fitting. Each function corresponds to a dataset."""
        return self._single_fit_functions

    @single_fit_functions.setter
    def single_fit_functions(self, fit_functions: list) -> None:
        """Sets all of the single fit functions stored in the model."""
        if len(fit_functions) != self.number_of_datasets:
            raise RuntimeError(f"The provided number of functions is not equal to the number of datasets.")

        self._single_fit_functions = fit_functions

    @property
    def single_fit_functions_cache(self) -> list:
        """Returns the cache of fit functions used for single fitting."""
        return self._single_fit_functions_cache

    @single_fit_functions_cache.setter
    def single_fit_functions_cache(self, fit_functions: list) -> None:
        """Sets the cache of fit functions used for single fitting."""
        if len(fit_functions) != self.number_of_datasets:
            raise RuntimeError(f"The provided number of fit functions is not equal to the number of datasets.")

        self._single_fit_functions_cache = fit_functions

    @property
    def fit_statuses(self) -> list:
        """Returns all of the fit statuses in a list."""
        return self._fit_statuses

    @fit_statuses.setter
    def fit_statuses(self, fit_statuses: list) -> None:
        """Sets the value of all fit statuses."""
        if len(fit_statuses) != self.number_of_datasets:
            raise RuntimeError(f"The provided number of fit statuses is not equal to the number of datasets.")

        self._fit_statuses = fit_statuses

    @property
    def fit_statuses_cache(self) -> list:
        """Returns all of the cached fit statuses in a list."""
        return self._fit_statuses_cache

    @fit_statuses_cache.setter
    def fit_statuses_cache(self, fit_statuses: list) -> None:
        """Sets the value of the cached fit statuses."""
        if len(fit_statuses) != self.number_of_datasets:
            raise RuntimeError(f"The provided number of fit statuses is not equal to the number of datasets.")

        self._fit_statuses_cache = fit_statuses

    @property
    def chi_squared(self) -> list:
        """Returns all of the chi squared values."""
        return self._chi_squared

    @chi_squared.setter
    def chi_squared(self, chi_squared: list) -> None:
        """Sets all of the chi squared values."""
        if len(chi_squared) != self.number_of_datasets:
            raise RuntimeError(f"The provided number of chi squared is not equal to the number of datasets.")

        self._chi_squared = chi_squared

    @property
    def chi_squared_cache(self) -> list:
        """Returns all of the cached chi squares in a list."""
        return self._chi_squared_cache

    @chi_squared_cache.setter
    def chi_squared_cache(self, chi_squared: list) -> None:
        """Sets the value of the cached fit statuses."""
        if len(chi_squared) != self.number_of_datasets:
            raise RuntimeError(f"The provided number of chi squared is not equal to the number of datasets.")

        self._chi_squared_cache = chi_squared

    @property
    def plot_guess(self) -> bool:
        """Returns true if plot guess is turned on."""
        return self._plot_guess

    @plot_guess.setter
    def plot_guess(self, plot_guess: bool) -> None:
        """Sets that the plot guess should or should not be plotted."""
        self._plot_guess = plot_guess

    @property
    def guess_workspace_name(self) -> str:
        """Returns the name of the currently selected guess workspace."""
        return self._guess_workspace_name

    @guess_workspace_name.setter
    def guess_workspace_name(self, workspace_name: str) -> None:
        """Set the name of the currently selected guess workspace."""
        if workspace_name is not None:
            self._guess_workspace_name = workspace_name if check_if_workspace_exist(workspace_name) else None
        else:
            self._guess_workspace_name = None

    @property
    def function_name(self) -> str:
        """Returns the function name to add to the end of a fitted workspace."""
        return self._function_name

    @function_name.setter
    def function_name(self, new_name: str) -> None:
        """Sets the function name to add to the end of a fitted workspace."""
        self._function_name = new_name
        if self._function_name != "" and self._function_name[:1] != " ":
            self._function_name = " " + self._function_name

    @property
    def function_name_auto_update(self) -> bool:
        """Returns a boolean whether or not to automatically update the function name."""
        return self._function_name_auto_update

    @function_name_auto_update.setter
    def function_name_auto_update(self, auto_update: bool) -> None:
        """Sets whether or not to automatically update the function name."""
        self._function_name_auto_update = auto_update

    @property
    def minimizer(self) -> str:
        """Returns the minimizer to be used during a fit."""
        return self._minimizer

    @minimizer.setter
    def minimizer(self, minimizer: str) -> None:
        """Sets the minimizer to be used during a fit."""
        self._minimizer = minimizer

    @property
    def evaluation_type(self) -> str:
        """Returns the evaluation type to be used during a fit."""
        return self._evaluation_type

    @evaluation_type.setter
    def evaluation_type(self, evaluation_type: str) -> None:
        """Sets the evaluation type to be used during a fit."""
        self._evaluation_type = evaluation_type

    @property
    def fit_to_raw(self) -> bool:
        """Returns true if fit to raw is turned on."""
        return self._fit_to_raw

    @fit_to_raw.setter
    def fit_to_raw(self, fit_to_raw: bool) -> None:
        """Sets the fit to raw property."""
        self._fit_to_raw = fit_to_raw
