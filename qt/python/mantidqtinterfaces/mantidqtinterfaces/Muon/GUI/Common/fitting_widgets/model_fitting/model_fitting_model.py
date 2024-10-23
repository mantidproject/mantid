# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import WorkspaceGroup
from mantid.simpleapi import CreateWorkspace
from mantid.kernel import UnitFactory

from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.ADS_calls import add_ws_to_ads, check_if_workspace_exist, retrieve_ws
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import (
    create_model_fitting_parameter_combination_name,
    create_model_fitting_parameters_group_name,
)
from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.model_fitting_context import ModelFittingContext
from mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context import MuonContext
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import BasicFittingModel
from mantidqtinterfaces.Muon.GUI.Common.utilities.workspace_data_utils import is_equal_to_n_decimals


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
        self._reset_current_result_table_index()

    @property
    def current_result_table_name(self) -> str:
        """Returns the currently selected dataset name"""
        current_result_table_index = self.fitting_context.current_result_table_index
        if current_result_table_index is not None:
            return self.fitting_context.result_table_names[current_result_table_index]
        else:
            return None

    def x_parameters(self) -> list:
        """Returns the X parameters that are stored by the fitting context."""
        return list(self.fitting_context.x_parameters.keys())

    def y_parameters(self) -> list:
        """Returns the Y parameters that are stored by the fitting context."""
        return list(self.fitting_context.y_parameters.keys())

    def x_parameter_types(self) -> list:
        """Returns the types of X parameters that are stored by the fitting context"""
        return [parameter[1] for parameter in self.fitting_context.x_parameters.values()]

    def y_parameter_types(self) -> list:
        """Returns the types of Y parameters that are stored by the fitting context"""
        return [parameter[1] for parameter in self.fitting_context.y_parameters.values()]

    def get_first_x_parameter_not(self, parameter: str) -> str:
        """Returns the first X parameter that is not the same as the parameter provided."""
        return self._get_first_parameter_from_list_not(self.x_parameters(), parameter)

    def get_first_y_parameter_not(self, parameter: str) -> str:
        """Returns the first Y parameter that is not the same as the parameter provided."""
        return self._get_first_parameter_from_list_not(self.y_parameters(), parameter)

    def parameter_combination_workspace_name(self, x_parameter: str, y_parameter: str) -> str:
        """Returns the workspace name being used for a particular parameter combination."""
        results_table_name = self.current_result_table_name
        if results_table_name is not None:
            return create_model_fitting_parameter_combination_name(results_table_name, x_parameter, y_parameter)
        else:
            return None

    def parameter_combination_group_name(self):
        """Returns the workspace group name being used to store the current parameter combinations."""
        results_table_name = self.current_result_table_name
        if results_table_name is not None:
            return create_model_fitting_parameters_group_name(results_table_name)
        else:
            return None

    def get_workspace_names_to_display_from_context(self) -> list:
        """Returns the names of results tables to display in the view."""
        return self._check_data_exists(self.context.results_context.result_table_names)

    def create_x_and_y_parameter_combinations(self) -> None:
        """Discovers the available X and Y parameters, and creates a MatrixWorkspace for each parameter combination."""
        self.fitting_context.x_parameters = {}
        self.fitting_context.x_parameter_errors = {}
        self.fitting_context.y_parameters = {}
        self.fitting_context.y_parameter_errors = {}

        self._extract_x_and_y_from_current_result_table()
        self.dataset_names = self._create_parameter_workspace_names()

    def _create_parameter_workspace_names(self) -> list:
        """Creates the names to use for each parameter combination workspace."""
        workspace_names = []
        for x_parameter_name in self.x_parameters():
            for y_parameter_name in self.y_parameters():
                if x_parameter_name != y_parameter_name:
                    workspace_names.append(self.parameter_combination_workspace_name(x_parameter_name, y_parameter_name))
        return workspace_names

    def create_x_and_y_parameter_combination_workspace(self, x_parameter_name: str, y_parameter_name: str) -> None:
        """Creates the parameter combination workspace for the specified x and y parameter."""
        workspace_group = self._create_workspace_group_to_store_combination_workspaces()
        if workspace_group is not None:
            self._create_matrix_workspaces_for_parameter_combination(workspace_group, x_parameter_name, y_parameter_name)

    def _extract_x_and_y_from_current_result_table(self) -> None:
        """Extracts the X, Y and error values from the currently selected result table and saves them in the context."""
        results_table_name = self.current_result_table_name
        if results_table_name is not None and check_if_workspace_exist(results_table_name):
            current_results_table = retrieve_ws(results_table_name)

            for i, column_name in enumerate(current_results_table.getColumnNames()):
                self._save_values_from_table_column(column_name, current_results_table.column(i), current_results_table.getPlotType(i))

            self._populate_empty_parameter_errors(current_results_table.rowCount())

    def _save_values_from_table_column(self, column_name: str, values: list, type: int) -> None:
        """Saves the values from a results table column in the correct location based on the column name."""
        if "Error" not in column_name:
            if column_name in ["run_start_seconds", "run_end_seconds"]:
                values = [value - values[0] for value in values]
                self.fitting_context.x_parameters[column_name] = [values, type]
                self.fitting_context.y_parameters[column_name] = [values, type]
            else:
                self.fitting_context.x_parameters[column_name] = [values, type]
                self.fitting_context.y_parameters[column_name] = [values, type]
        else:
            self.fitting_context.x_parameter_errors[column_name.replace("Error", "")] = values
            self.fitting_context.y_parameter_errors[column_name.replace("Error", "")] = values

    def _populate_empty_parameter_errors(self, number_of_data_points: int) -> None:
        """Populates the Y parameter errors with zeros if they are empty. For instance, workspace_name has no errors."""
        x_parameter_error_names = self.fitting_context.x_parameter_errors.keys()
        y_parameter_error_names = self.fitting_context.y_parameter_errors.keys()
        for parameter_name in self.fitting_context.y_parameters.keys():
            if parameter_name not in x_parameter_error_names:
                self.fitting_context.x_parameter_errors[parameter_name] = [0.0] * number_of_data_points
            if parameter_name not in y_parameter_error_names:
                self.fitting_context.y_parameter_errors[parameter_name] = [0.0] * number_of_data_points

    def _create_workspace_group_to_store_combination_workspaces(self) -> WorkspaceGroup:
        """Return the Workspace Group used to store the different parameter combination matrix workspaces."""
        group_name = self.parameter_combination_group_name()
        if group_name is None:
            return None

        if check_if_workspace_exist(group_name):
            workspace_group = retrieve_ws(group_name)
        else:
            workspace_group = WorkspaceGroup()
            add_ws_to_ads(group_name, workspace_group)
        return workspace_group

    def _create_matrix_workspaces_for_parameter_combination(
        self, workspace_group: WorkspaceGroup, x_parameter_name: str, y_parameter_name: str
    ) -> None:
        """Creates the matrix workspace for a specific x and y parameter, and adds it to the workspace group."""
        if x_parameter_name != y_parameter_name and x_parameter_name in self.x_parameters() and y_parameter_name in self.y_parameters():
            x_values = self._convert_str_column_values_to_int(self.fitting_context.x_parameters[x_parameter_name])
            x_errors = self.fitting_context.x_parameter_errors[x_parameter_name]
            y_values = self._convert_str_column_values_to_int(self.fitting_context.y_parameters[y_parameter_name])
            y_errors = self.fitting_context.y_parameter_errors[y_parameter_name]

            # Sort the data based on the x_values being in ascending order
            x_values, x_errors, y_values, y_errors = zip(*sorted(zip(x_values, x_errors, y_values, y_errors)))

            output_name = self.parameter_combination_workspace_name(x_parameter_name, y_parameter_name)
            if not self._parameter_combination_workspace_exists(output_name, x_values, y_values, y_errors):
                self._create_workspace(x_values, x_errors, x_parameter_name, y_values, y_errors, y_parameter_name, output_name)
                workspace_group.add(output_name)

    def _create_workspace(
        self, x_values: list, x_errors: list, x_parameter: str, y_values: list, y_errors: list, y_parameter: str, output_name: str
    ) -> None:
        """Creates a matrix workspace using the provided data. Uses UnitX if the parameter exists in the UnitFactory."""
        CreateWorkspace(DataX=x_values, Dx=x_errors, DataY=y_values, DataE=y_errors, OutputWorkspace=output_name)
        self._set_x_label(output_name, x_parameter)
        self._set_y_label(output_name, y_parameter)

    def _parameter_combination_workspace_exists(self, workspace_name: str, x_values: list, y_values: list, y_errors: list) -> bool:
        """Returns true if a parameter combination workspace exists and contains the same data."""
        if check_if_workspace_exist(workspace_name):
            workspace = retrieve_ws(workspace_name)
            return (
                self._lists_are_equal(workspace.dataX(0), x_values)
                and self._lists_are_equal(workspace.dataY(0), y_values)
                and self._lists_are_equal(workspace.dataE(0), y_errors)
            )
        return False

    @staticmethod
    def _lists_are_equal(list1: list, list2: list) -> bool:
        """Returns true if the two lists containing x, y or error data are equal to five decimal places."""
        if len(list1) != len(list2):
            return False
        return all([is_equal_to_n_decimals(i, j, 5) for i, j in zip(list1, list2)])

    @staticmethod
    def _convert_str_column_values_to_int(parameter_values: list) -> list:
        """Converts any str column values to an int so that they can be fitted."""
        if isinstance(parameter_values[0][0], str):
            return range(len(parameter_values[0]))
        else:
            return parameter_values[0]

    def _get_new_start_xs_and_end_xs_using_existing_datasets(self, new_dataset_names: list) -> tuple:
        """Returns the start and end Xs to use for the new datasets. It uses the limits of the new workspaces."""
        start_xs, end_xs = [], []
        for name in new_dataset_names:
            x_lower, x_upper = self.x_limits_of_workspace(name)
            start_xs.append(x_lower)
            end_xs.append(x_upper)
        return start_xs, end_xs

    def retrieve_first_good_data_from(self, workspace_name: str) -> float:
        """Returns the first good data value from a workspace."""
        x_lower, _ = self.x_limits_of_workspace(workspace_name)
        return x_lower

    def number_of_result_tables(self) -> int:
        """Returns the number of results tables stored by the fitting context."""
        return self.fitting_context.number_of_result_tables()

    def _reset_current_result_table_index(self) -> None:
        """Resets the current result table index stored by the context."""
        if self.fitting_context.number_of_result_tables() == 0:
            self.fitting_context.current_result_table_index = None
        elif (
            self.fitting_context.current_result_table_index is None
            or self.fitting_context.current_result_table_index >= self.fitting_context.number_of_result_tables()
        ):
            self.fitting_context.current_result_table_index = 0

    @staticmethod
    def _get_first_parameter_from_list_not(parameters: list, parameter: str) -> str:
        """Returns the first parameter in the list that is not the same as the parameter provided."""
        if parameter in parameters:
            parameters.remove(parameter)
        return parameters[0] if len(parameters) > 0 else None

    def _set_x_label(self, workspace_name: str, axis_label: str) -> None:
        """Sets the label and unit for the X axis of a workspace."""
        workspace = retrieve_ws(workspace_name)
        unit = self._get_parameter_unit(axis_label, 0)
        workspace.getAxis(0).setUnit("Label").setLabel(axis_label, unit)

    def _set_y_label(self, workspace_name: str, axis_label: str) -> None:
        """Sets the label and unit for the Y axis of a workspace."""
        workspace = retrieve_ws(workspace_name)
        unit = self._get_parameter_unit(axis_label, 1)
        workspace.setYUnit(f"{axis_label} ({unit})" if unit != "" else f"{axis_label}")

    def _get_parameter_unit(self, parameter_name: str, axis: int) -> str:
        """Returns the units of a parameter by searching the Dictionary, UnitFactory and Sample logs."""
        parameter_name = parameter_name.split(".")[-1]
        unit = self._get_unit_from_unit_dictionary(parameter_name, axis)
        if unit == "":
            unit = self._get_unit_from_sample_logs(parameter_name)
        if unit == "":
            unit = self._get_unit_from_unit_factory(parameter_name)
        return unit

    def _get_unit_from_unit_dictionary(self, parameter_name: str, axis: int) -> str:
        """Returns the units of a parameter if it exists in the model fitting unit dictionary."""
        if parameter_name in self.fitting_context.parameter_units:
            return self.fitting_context.parameter_units[parameter_name][axis]
        return ""

    def _get_unit_from_unit_factory(self, parameter_name: str) -> str:
        """Returns the units of a parameter if it exists in the UnitFactory."""
        if self._is_in_unit_factory(parameter_name):
            return UnitFactory.create(parameter_name).label()
        return ""

    def _get_unit_from_sample_logs(self, parameter_name: str) -> str:
        """Returns the units of a sample log if the parameter exists as a sample log."""
        workspace = self.context.data_context.current_workspace
        if workspace:
            run = workspace.run()
            return run.getLogData(parameter_name).units if run.hasProperty(parameter_name) else ""
        return ""

    @staticmethod
    def _is_in_unit_factory(parameter_name: str) -> bool:
        """Returns true of the provided parameter exists in the UnitFactory."""
        return parameter_name in UnitFactory.getKeys()

    def get_override_x_and_y_tick_labels(self, x_parameter_name: str, y_parameter_name: str) -> tuple:
        """Returns the override x and y tick labels to use when plotting."""
        override_x_tick_labels = self._get_override_tick_labels_if_str(x_parameter_name, self.fitting_context.x_parameters)
        override_y_tick_labels = self._get_override_tick_labels_if_str(y_parameter_name, self.fitting_context.y_parameters)
        return override_x_tick_labels, override_y_tick_labels

    @staticmethod
    def _get_override_tick_labels_if_str(parameter_name: str, parameters: dict) -> list:
        """Returns a list of tick labels to use as override labels if the parameter values are strings."""
        if parameter_name in parameters:
            parameter_values = parameters[parameter_name]
            return parameter_values[0] if all(isinstance(n, str) for n in parameter_values[0]) else []
        return []
