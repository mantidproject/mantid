# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateWorkspace

from Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws
from Muon.GUI.Common.contexts.fitting_contexts.model_fitting_context import ModelFittingContext
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import BasicFittingModel


class ModelFittingModel(BasicFittingModel):
    """
    The ModelFittingModel derives from BasicFittingModel.
    """

    def __init__(self, context: MuonContext, fitting_context: ModelFittingContext):
        """Initialize the GeneralFittingModel with emtpy fit data."""
        super(ModelFittingModel, self).__init__(context, fitting_context)

    @property
    def current_result_table_index(self) -> int:
        """Returns the index of the currently selected result table."""
        return self.fitting_context.current_result_table_index

    @current_result_table_index.setter
    def current_result_table_index(self, index: int) -> None:
        """Sets the index of the currently selected result table."""
        self.fitting_context.current_result_table_index = index

    @property
    def result_table_names(self) -> list:
        """Returns the names of the results tables loaded into the model fitting tab."""
        return self.fitting_context.result_table_names

    @result_table_names.setter
    def result_table_names(self, table_names: list) -> None:
        """Sets the names of the results tables loaded into the model fitting tab."""
        self.fitting_context.result_table_names = table_names

    @property
    def current_result_table_name(self) -> str:
        """Returns the currently selected dataset name"""
        current_result_table_index = self.fitting_context.current_result_table_index
        if current_result_table_index is not None:
            return self.fitting_context.result_table_names[current_result_table_index]
        else:
            return None

    @property
    def number_of_result_tables(self):
        """Returns the number of result tables which are held by the context."""
        return self.fitting_context.number_of_result_tables

    def get_workspace_names_to_display_from_context(self) -> list:
        """Returns the names of results tables to display in the view."""
        return self._check_data_exists(self.context.results_context.result_table_names)

    def create_x_and_y_parameter_combination_workspaces(self) -> None:
        """Discovers the available X and Y parameters, and creates a MatrixWorkspace for each parameter combination."""
        self.fitting_context.x_parameters = {}
        self.fitting_context.y_parameters = {}
        self.fitting_context.y_parameter_errors = {}

        self._extract_x_and_y_from_current_result_table()
        self.dataset_names = self._create_matrix_workspaces_for_parameter_combinations()

        return self.fitting_context.x_parameters.keys(), self.fitting_context.y_parameters.keys()

    def _extract_x_and_y_from_current_result_table(self) -> None:
        """Extracts the X, Y and error values from the currently selected result table and saves them in the context."""
        current_results_table = retrieve_ws(self.current_result_table_name)

        for i, column_name in enumerate(current_results_table.getColumnNames()):
            self._save_values_from_table_column(column_name, current_results_table.column(i))

        self._populate_empty_parameter_errors(current_results_table.rowCount())

    def _save_values_from_table_column(self, column_name: str, values: list) -> None:
        """Saves the values from a results table column in the correct location based on the column name."""
        if "Error" not in column_name:
            self.fitting_context.x_parameters[column_name] = values
            self.fitting_context.y_parameters[column_name] = values
        else:
            self.fitting_context.y_parameter_errors[column_name.replace("Error", "")] = values

    def _populate_empty_parameter_errors(self, number_of_data_points: int) -> None:
        """Populates the Y parameter errors with zeros if they are empty. For instance, workspace_name has no errors."""
        y_parameter_error_names = self.fitting_context.y_parameter_errors.keys()
        for parameter_name in self.fitting_context.y_parameters.keys():
            if parameter_name not in y_parameter_error_names:
                self.fitting_context.y_parameter_errors[parameter_name] = [0.0] * number_of_data_points

    def _create_matrix_workspaces_for_parameter_combinations(self) -> list:
        """Creates a MatrixWorkspace for each parameter combination. These are the workspaces that will be fitted."""
        workspace_names = []
        for x_parameter_name in self.fitting_context.x_parameters.keys():
            for y_parameter_name in self.fitting_context.y_parameters.keys():
                x_values = self._convert_str_column_values_to_int(x_parameter_name, self.fitting_context.x_parameters)
                y_values = self._convert_str_column_values_to_int(y_parameter_name, self.fitting_context.y_parameters)
                y_errors = self.fitting_context.y_parameter_errors[y_parameter_name]
                output_name = x_parameter_name + "_" + y_parameter_name

                CreateWorkspace(DataX=x_values, DataY=y_values, DataE=y_errors, OutputWorkspace=output_name)

                workspace_names.append(output_name)
        return workspace_names

    @staticmethod
    def _convert_str_column_values_to_int(parameter_name: str, parameter_values: list) -> list:
        """Converts any str column values to an int so that they can be fitted."""
        if type(parameter_values[parameter_name][0]) == str:
            return range(len(parameter_values[parameter_name]))
        else:
            return parameter_values[parameter_name]
