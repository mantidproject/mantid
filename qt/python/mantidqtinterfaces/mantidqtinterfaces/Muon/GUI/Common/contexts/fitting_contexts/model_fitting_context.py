# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.basic_fitting_context import BasicFittingContext

# The PARAMETERUNITS dict contains two copies of each units because the X and Y unit setting options on workspaces take
# different formatting for special symbols
PARAMETERUNITS = {
    "Frequency": ["MHz", "MHz"],
    "Lambda": [r"\mu s^{-1}", r"$\mu$ $s^{-1}$"],
    "Phi": ["Radians", "Radians"],
    "run_start_seconds": ["s", "s"],
    "run_end_seconds": ["s", "s"],
    "sample_magn_field": ["G", "G"],
    "sample_temp": ["K", "K"],
    "Sigma": [r"\mu s^{-1}", r"$\mu$ $s^{-1}$"],
}


class ModelFittingContext(BasicFittingContext):
    def __init__(self, allow_double_pulse_fitting: bool = False):
        super(ModelFittingContext, self).__init__(allow_double_pulse_fitting)

        self._result_table_names: list = []
        self._current_result_table_index: int = None

        self._x_parameters: dict = {}
        self._x_parameter_errors: dict = {}

        self._y_parameters: dict = {}
        self._y_parameter_errors: dict = {}

        self._parameter_units = PARAMETERUNITS

    @property
    def result_table_names(self) -> list:
        """Returns the names of the results tables loaded into the model fitting tab."""
        return self._result_table_names

    @result_table_names.setter
    def result_table_names(self, table_names: list) -> None:
        """Sets the names of the results tables loaded into the model fitting tab."""
        self._result_table_names = table_names

    @property
    def current_result_table_index(self) -> int:
        """Returns the index of the currently selected result table."""
        return self._current_result_table_index

    @current_result_table_index.setter
    def current_result_table_index(self, index: int) -> None:
        """Sets the index of the currently selected result table."""
        assert index is None or index < self.number_of_result_tables(), f"The result table index ({index}) is too large."

        self._current_result_table_index = index

    def number_of_result_tables(self) -> int:
        """Returns the number of result tables which are held by the context."""
        return len(self._result_table_names)

    @property
    def x_parameters(self) -> dict:
        """Returns the available x parameters for the selected results table."""
        return self._x_parameters

    @x_parameters.setter
    def x_parameters(self, parameters: dict) -> None:
        """Sets the available x parameters for the selected results table."""
        self._x_parameters = parameters

    @property
    def x_parameter_errors(self) -> dict:
        """Returns the available x parameter errors for the selected results table."""
        return self._x_parameter_errors

    @x_parameter_errors.setter
    def x_parameter_errors(self, errors: dict) -> None:
        """Sets the available x parameter errors for the selected results table."""
        self._x_parameter_errors = errors

    @property
    def y_parameters(self) -> dict:
        """Returns the available y parameters for the selected results table."""
        return self._y_parameters

    @y_parameters.setter
    def y_parameters(self, parameters: dict) -> None:
        """Sets the available y parameters for the selected results table."""
        self._y_parameters = parameters

    @property
    def y_parameter_errors(self) -> dict:
        """Returns the available y parameter errors for the selected results table."""
        return self._y_parameter_errors

    @y_parameter_errors.setter
    def y_parameter_errors(self, errors: list) -> None:
        """Sets the available y parameter errors for the selected results table."""
        self._y_parameter_errors = errors

    @property
    def parameter_units(self) -> dict:
        """Returns the units of a parameter if it exists in the model fitting unit dictionary."""
        return self._parameter_units

    @parameter_units.setter
    def parameter_units(self, parameter_units: dict) -> None:
        """Returns the units of a parameter if it exists in the model fitting unit dictionary."""
        self._parameter_units = parameter_units
