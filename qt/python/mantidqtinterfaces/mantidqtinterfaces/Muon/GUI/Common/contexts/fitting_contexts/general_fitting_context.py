# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction
from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.basic_fitting_context import BasicFittingContext, SINGLE_FITS_KEY

SIMULTANEOUS_FITS_KEY = "SimultaneousFits"


class GeneralFittingContext(BasicFittingContext):
    def __init__(self, allow_double_pulse_fitting: bool = False):
        super(GeneralFittingContext, self).__init__(allow_double_pulse_fitting)

        # A list of FitInformation's detailing all the simultaneous fits that have happened including the fits that have
        # been overridden by an updated fit. The last simultaneous fit performed is at the end of the list, and undoing
        # will remove it.
        self._fit_history[SIMULTANEOUS_FITS_KEY] = []

        self._simultaneous_fitting_mode: bool = False

        # This is a MultiDomainFunction if there are multiple domains in the function browser.
        self._simultaneous_fit_function: IFunction = None

        self._global_parameters: list = []

        self._simultaneous_fit_functions_for_undo: list = []
        self._simultaneous_fit_statuses_for_undo: list = []
        self._simultaneous_chi_squared_for_undo: list = []
        self._global_parameters_for_undo: list = []

        self._simultaneous_fit_by: str = ""
        self._simultaneous_fit_by_specifier: str = ""

    def all_latest_fits(self):
        """Returns the latest unique fits for all fitting modes."""
        return super().all_latest_fits() + self._latest_unique_fits_in(self._fit_history[SIMULTANEOUS_FITS_KEY])

    @property
    def active_fit_history(self):
        """Returns the fit history for the currently active fitting mode."""
        return self._fit_history[SIMULTANEOUS_FITS_KEY if self.simultaneous_fitting_mode else SINGLE_FITS_KEY]

    @active_fit_history.setter
    def active_fit_history(self, fit_history: list) -> None:
        """Sets the fit history for the currently active fitting mode."""
        self._fit_history[SIMULTANEOUS_FITS_KEY if self.simultaneous_fitting_mode else SINGLE_FITS_KEY] = fit_history

    def clear(self, removed_fits: list = []):
        """Removes all the stored Fits from the context when an ADS clear event happens."""
        if len(removed_fits) == 0:
            removed_fits = self.all_latest_fits()

        self._fit_history[SIMULTANEOUS_FITS_KEY] = []
        self._simultaneous_fit_functions_for_undo = []
        self._simultaneous_fit_statuses_for_undo = []
        self._simultaneous_chi_squared_for_undo = []
        self._global_parameters_for_undo = []

        super().clear(removed_fits)

    def remove_workspace_by_name(self, workspace_name: str) -> None:
        """Remove a Fit from the history when an ADS delete event happens on one of its output workspaces."""
        self.remove_fit_by_name(self._fit_history[SIMULTANEOUS_FITS_KEY], workspace_name)
        super().remove_workspace_by_name(workspace_name)

    @property
    def simultaneous_fitting_mode(self) -> bool:
        """Returns whether or not simultaneous fitting is currently active. If not, single fitting is active."""
        return self._simultaneous_fitting_mode

    @simultaneous_fitting_mode.setter
    def simultaneous_fitting_mode(self, enable_simultaneous: bool) -> None:
        """Sets whether or not simultaneous fitting is currently active in the model."""
        self._simultaneous_fitting_mode = enable_simultaneous

    @property
    def simultaneous_fit_function(self) -> IFunction:
        """Returns the simultaneous fit function stored in the model."""
        return self._simultaneous_fit_function

    @simultaneous_fit_function.setter
    def simultaneous_fit_function(self, fit_function: IFunction) -> None:
        """Sets the simultaneous fit function stored in the model."""
        assert fit_function is None or self.number_of_datasets > 0, (
            "Cannot set a simultaneous fit function when there are no datasets in the model."
        )

        self._simultaneous_fit_function = fit_function

    @property
    def global_parameters(self) -> list:
        """Returns the global parameters stored in the model."""
        return self._global_parameters

    @global_parameters.setter
    def global_parameters(self, global_parameters: list) -> None:
        """Sets the global parameters stored in the model."""
        self._global_parameters = global_parameters

    @property
    def simultaneous_fit_functions_for_undo(self) -> list:
        """Returns the previous simultaneous fit function that can be used when undoing."""
        return self._simultaneous_fit_functions_for_undo

    @simultaneous_fit_functions_for_undo.setter
    def simultaneous_fit_functions_for_undo(self, fit_functions: list) -> None:
        """Sets the previous simultaneous fit functions that can be used when undoing."""
        self._simultaneous_fit_functions_for_undo = fit_functions

    @property
    def simultaneous_fit_statuses_for_undo(self) -> list:
        """Returns the previous simultaneous fit statues that can be used when undoing."""
        return self._simultaneous_fit_statuses_for_undo

    @simultaneous_fit_statuses_for_undo.setter
    def simultaneous_fit_statuses_for_undo(self, fit_statuses: list) -> None:
        """Sets the previous simultaneous fit statues that can be used when undoing."""
        self._simultaneous_fit_statuses_for_undo = fit_statuses

    @property
    def simultaneous_chi_squared_for_undo(self) -> list:
        """Returns the previous simultaneous chi squared that can be used when undoing."""
        return self._simultaneous_chi_squared_for_undo

    @simultaneous_chi_squared_for_undo.setter
    def simultaneous_chi_squared_for_undo(self, chi_squared: list) -> None:
        """Sets the previous simultaneous chi squared that can be used when undoing."""
        self._simultaneous_chi_squared_for_undo = chi_squared

    @property
    def global_parameters_for_undo(self) -> list:
        """Returns the previous global parameters that can be used when undoing."""
        return self._global_parameters_for_undo

    @global_parameters_for_undo.setter
    def global_parameters_for_undo(self, global_parameters: list) -> None:
        """Sets the previous global parameters that can be used when undoing."""
        self._global_parameters_for_undo = global_parameters

    @property
    def simultaneous_fit_by(self) -> str:
        """Returns the simultaneous fit by parameter stored in the model."""
        return self._simultaneous_fit_by

    @simultaneous_fit_by.setter
    def simultaneous_fit_by(self, simultaneous_fit_by: str) -> None:
        """Sets the simultaneous fit by parameter stored in the model."""
        self._simultaneous_fit_by = simultaneous_fit_by

    @property
    def simultaneous_fit_by_specifier(self) -> str:
        """Returns the simultaneous fit by specifier stored in the model."""
        return self._simultaneous_fit_by_specifier

    @simultaneous_fit_by_specifier.setter
    def simultaneous_fit_by_specifier(self, simultaneous_fit_by_specifier: str) -> None:
        """Sets the simultaneous fit by specifier stored in the model."""
        self._simultaneous_fit_by_specifier = simultaneous_fit_by_specifier
