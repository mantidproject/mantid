# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist
from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.fitting_context import FittingContext

SINGLE_FITS_KEY = "SingleFits"
X_FROM_FIT_RANGE = "x from fit range"
X_FROM_DATA_RANGE = "Uniform points across data range"
X_FROM_CUSTOM = "Custom x range"


class BasicFittingContext(FittingContext):
    def __init__(self, allow_double_pulse_fitting: bool = False):
        super(BasicFittingContext, self).__init__()

        self._allow_double_pulse_fitting: bool = allow_double_pulse_fitting

        # A list of FitInformation's detailing all the single fits that have happened including the fits that have been
        # overridden by an updated fit. The last single fit performed is at the end of the list, and undoing will remove
        # it.
        self._fit_history[SINGLE_FITS_KEY] = []

        self._current_dataset_index: int = None
        self._dataset_names: list = []

        self._dataset_indices_for_undo: list = []

        self._single_fit_functions: list = []
        self._single_fit_functions_for_undo: list = []

        self._fit_statuses: list = []
        self._fit_statuses_for_undo: list = []

        self._chi_squared: list = []
        self._chi_squared_for_undo: list = []

        self._plot_guess: bool = False
        self._plot_guess_type: str = X_FROM_FIT_RANGE
        self._plot_guess_points: int = None
        self._plot_guess_start_x: float = None
        self._plot_guess_end_x: float = None
        self._guess_workspace_name: str = None

        self._function_name: str = ""
        self._function_name_auto_update: bool = True

        self._start_xs: list = []
        self._end_xs: list = []

        self._exclude_range: bool = False
        self._exclude_start_xs: list = []
        self._exclude_end_xs: list = []

        self._minimizer: str = ""
        self._evaluation_type: str = ""
        self._fit_to_raw: bool = True

    def all_latest_fits(self) -> list:
        """Returns the latest unique fits for all fitting modes."""
        return self._latest_unique_fits_in(self._fit_history[SINGLE_FITS_KEY])

    @property
    def active_fit_history(self) -> list:
        """Returns the fit history for the currently active fitting mode."""
        return self._fit_history[SINGLE_FITS_KEY]

    @active_fit_history.setter
    def active_fit_history(self, fit_history: list) -> None:
        """Sets the fit history for the currently active fitting mode."""
        self._fit_history[SINGLE_FITS_KEY] = fit_history

    def clear(self, removed_fits: list = []) -> None:
        """Removes all the stored Fits from the context when an ADS clear event happens."""
        if len(removed_fits) == 0:
            removed_fits = self.all_latest_fits()

        self._fit_history[SINGLE_FITS_KEY] = []
        self._dataset_indices_for_undo = []
        self._single_fit_functions_for_undo = []
        self._fit_statuses_for_undo = []
        self._chi_squared_for_undo = []

        super().clear(removed_fits)

    def remove_workspace_by_name(self, workspace_name: str) -> None:
        """Remove a Fit from the history when an ADS delete event happens on one of its output workspaces."""
        self.remove_fit_by_name(self._fit_history[SINGLE_FITS_KEY], workspace_name)

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
        assert index is None or index < self.number_of_datasets, f"The dataset index ({index}) is too large."

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
    def dataset_indices_for_undo(self) -> list:
        """Returns the dataset indices from previous fits used for single fitting."""
        return self._dataset_indices_for_undo

    @dataset_indices_for_undo.setter
    def dataset_indices_for_undo(self, dataset_indices: list) -> None:
        """Sets the dataset indices from previous fits used for single fitting."""
        self._dataset_indices_for_undo = dataset_indices

    @property
    def single_fit_functions(self) -> list:
        """Returns all of the fit functions used for single fitting. Each function corresponds to a dataset."""
        return self._single_fit_functions

    @single_fit_functions.setter
    def single_fit_functions(self, fit_functions: list) -> None:
        """Sets all of the single fit functions stored in the model."""
        assert len(fit_functions) == self.number_of_datasets, "The number of functions is not equal to the number of datasets."

        self._single_fit_functions = fit_functions

    @property
    def single_fit_functions_for_undo(self) -> list:
        """Returns the fit functions from previous fits used for single fitting."""
        return self._single_fit_functions_for_undo

    @single_fit_functions_for_undo.setter
    def single_fit_functions_for_undo(self, fit_functions: list) -> None:
        """Sets the fit functions from previous fits used for single fitting."""
        self._single_fit_functions_for_undo = fit_functions

    @property
    def fit_statuses(self) -> list:
        """Returns all of the fit statuses in a list."""
        return self._fit_statuses

    @fit_statuses.setter
    def fit_statuses(self, fit_statuses: list) -> None:
        """Sets the value of all fit statuses."""
        assert len(fit_statuses) == self.number_of_datasets, "The number of fit statuses is not equal to the number of datasets."

        self._fit_statuses = fit_statuses

    @property
    def fit_statuses_for_undo(self) -> list:
        """Returns the fit statuses from previous fits used for single fitting."""
        return self._fit_statuses_for_undo

    @fit_statuses_for_undo.setter
    def fit_statuses_for_undo(self, fit_statuses: list) -> None:
        """Sets the fit statuses from previous fits used for single fitting."""
        self._fit_statuses_for_undo = fit_statuses

    @property
    def chi_squared(self) -> list:
        """Returns all of the chi squared values."""
        return self._chi_squared

    @chi_squared.setter
    def chi_squared(self, chi_squared: list) -> None:
        """Sets all of the chi squared values."""
        assert len(chi_squared) == self.number_of_datasets, "The number of chi squared is not equal to the number of datasets."

        self._chi_squared = chi_squared

    @property
    def chi_squared_for_undo(self) -> list:
        """Returns the chi squared from previous fits used for single fitting."""
        return self._chi_squared_for_undo

    @chi_squared_for_undo.setter
    def chi_squared_for_undo(self, chi_squared: list) -> None:
        """Sets the chi squared from previous fits used for single fitting."""
        self._chi_squared_for_undo = chi_squared

    @property
    def plot_guess(self) -> bool:
        """Returns true if plot guess is turned on."""
        return self._plot_guess

    @plot_guess.setter
    def plot_guess(self, plot_guess: bool) -> None:
        """Sets that the plot guess should or should not be plotted."""
        self._plot_guess = plot_guess

    @property
    def plot_guess_type(self) -> str:
        """Returns the guess plot range type."""
        return self._plot_guess_type

    @plot_guess_type.setter
    def plot_guess_type(self, plot_guess_type: str) -> None:
        """Sets the guess plot range type."""
        self._plot_guess_type = plot_guess_type

    @property
    def plot_guess_points(self) -> int:
        """Returns the number of points to use in the guess plot."""
        return self._plot_guess_points

    @plot_guess_points.setter
    def plot_guess_points(self, plot_guess_type: int) -> None:
        """Sets the number of points to use in the guess plot."""
        self._plot_guess_points = plot_guess_type

    @property
    def plot_guess_start_x(self) -> float:
        """Returns the start x to use in the guess plot."""
        return self._plot_guess_start_x

    @plot_guess_start_x.setter
    def plot_guess_start_x(self, plot_guess_start_x: float) -> None:
        """Sets the start x to use in the guess plot."""
        self._plot_guess_start_x = plot_guess_start_x

    @property
    def plot_guess_end_x(self) -> float:
        """Returns the end x to use in the guess plot."""
        return self._plot_guess_end_x

    @plot_guess_end_x.setter
    def plot_guess_end_x(self, plot_guess_end_x: float) -> None:
        """Sets the end x to use in the guess plot."""
        self._plot_guess_end_x = plot_guess_end_x

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
    def start_xs(self) -> list:
        """Returns all of the start Xs stored by the model."""
        return self._start_xs

    @start_xs.setter
    def start_xs(self, start_xs: list) -> None:
        """Sets all of the start Xs in the model."""
        assert len(start_xs) == self.number_of_datasets, "The number of start Xs is not equal to the number of datasets."

        self._start_xs = start_xs

    @property
    def end_xs(self) -> list:
        """Returns all of the end Xs stored by the model."""
        return self._end_xs

    @end_xs.setter
    def end_xs(self, end_xs: list) -> None:
        """Sets all of the end Xs in the model."""
        assert len(end_xs) == self.number_of_datasets, "The number of end Xs is not equal to the number of datasets."

        self._end_xs = end_xs

    @property
    def exclude_range(self) -> bool:
        """Returns true if the Exclude Range option is on in the context."""
        return self._exclude_range

    @exclude_range.setter
    def exclude_range(self, exclude_range_on: bool) -> None:
        """Sets whether the Exclude Range option is on in the context."""
        self._exclude_range = exclude_range_on

    @property
    def exclude_start_xs(self) -> list:
        """Returns the exclude start Xs stored by the context."""
        return self._exclude_start_xs

    @exclude_start_xs.setter
    def exclude_start_xs(self, start_xs: list) -> None:
        """Sets the exclude start Xs stored by the context."""
        assert len(start_xs) == self.number_of_datasets, "The number of exclude start Xs is not equal to the number of datasets."

        self._exclude_start_xs = start_xs

    @property
    def exclude_end_xs(self) -> list:
        """Returns the exclude end Xs stored by the context."""
        return self._exclude_end_xs

    @exclude_end_xs.setter
    def exclude_end_xs(self, end_xs: list) -> None:
        """Sets the exclude end Xs stored by the context."""
        assert len(end_xs) == self.number_of_datasets, "The number of exclude end Xs is not equal to the number of datasets."

        self._exclude_end_xs = end_xs

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
