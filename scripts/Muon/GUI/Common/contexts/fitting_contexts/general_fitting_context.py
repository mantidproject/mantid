# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction
from Muon.GUI.Common.contexts.fitting_contexts.basic_fitting_context import BasicFittingContext


class GeneralFittingContext(BasicFittingContext):

    def __init__(self, allow_double_pulse_fitting: bool = False):
        super(GeneralFittingContext, self).__init__(allow_double_pulse_fitting)

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
        if fit_function is not None and self.number_of_datasets == 0:
            raise RuntimeError(f"Cannot set a simultaneous fit function when there are no datasets in the model.")

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
