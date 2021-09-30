# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from Muon.GUI.Common.seq_fitting_tab_widget.QSequentialTableModel import (QSequentialTableModel, default_table_columns,
                                                                          FIT_STATUS_COLUMN, FIT_QUALITY_COLUMN,
                                                                          GROUP_COLUMN, RUN_COLUMN, WORKSPACE_COLUMN)
from mantidqt.utils.testing.mocks.mock_sequentialtable import MockSequentialTableModel, MockSequentialTableView
from Muon.GUI.Common.seq_fitting_tab_widget.SequentialTableWidget import SequentialTableWidget
from mantidqt.utils.testing.mocks.mock_qt import MockQModelIndex
from unittest import mock
from qtpy.QtCore import Qt

failed_string = "Failed to converge after 500 iterations"
changes_too_small_string = "Changes in parameter value are too small"
success_string = "success"
no_fit_string = "No fit"
fit_status_strings = {"failed_string": failed_string, "changes_too_small_string": changes_too_small_string,
                      "success_string": success_string, "no_fit_string": no_fit_string}


class SequentialTableWidgetTest(unittest.TestCase):
    def setUp(self):
        self.view = MockSequentialTableView()
        self.model = MockSequentialTableModel()
        self.table_widget = SequentialTableWidget(parent=None, view=self.view, model=self.model)

    def test_set_parameters_and_values_correctly_queries_model(self):
        parameters = ['A', 'B', 'C']
        values = [[0.2, 0.3, 0.4]]

        self.table_widget.set_parameters_and_values(parameters, values)

        self.model.set_fit_parameters_and_values.assert_called_once_with(parameters, values)

    def test_set_parameters_and_values_does_nothing_if_dimensions_do_not_match(self):
        parameters = ['A', 'B', 'C']
        values = [[0.2, 0.3]]

        self.table_widget.set_parameters_and_values(parameters, values)

        self.model.set_fit_parameters_and_values.assert_not_called()

    def test_set_fit_quality_correctly_queries_model(self):
        status_string = fit_status_strings["failed_string"]
        row = 2
        quality = 1.212

        self.table_widget.set_fit_quality(row, status_string, quality)

        self.model.set_fit_quality.assert_called_once_with(row, "Failed", quality)

    def test_get_selected_rows_correctly_queries_view(self):
        # two rows are selected in different positions
        mock_indexes = [MockQModelIndex(0, 1), MockQModelIndex(3, 1)]
        self.view.mock_selection_model.selectedRows = mock.MagicMock(return_value=mock_indexes)

        return_rows = self.table_widget.get_selected_rows()

        self.assertEqual([0, 3], return_rows)

    def test_get_workspace_info_from_row_correctly_queries_model(self):
        row = 1
        self.model.rowCount.return_value = 3

        self.table_widget.get_workspace_info_from_row(row)

        self.model.get_run_information.assert_called_once_with(row)
        self.model.get_group_information.assert_called_once_with(row)

    def test_get_workspace_info_from_row_does_nothing_if_row_outside_table(self):
        row = 4
        self.model.rowCount.return_value = 3

        self.table_widget.get_workspace_info_from_row(row)

        self.model.get_run_information.assert_not_called()
        self.model.get_group_information.assert_not_called()

    def test_get_workspace_names_from_row_correctly_queries_model(self):
        row = 1

        self.table_widget.get_workspace_names_from_row(row)

        self.model.get_workspace_name_information.assert_called_once_with(row)


class SequentialTableModelTest(unittest.TestCase):
    def setUp(self):
        self.model = QSequentialTableModel()

        self.workspace_data = [["EMU2223; Group; bwd; MA", "2223", "bwd", "No Fit", 0],
                               ["EMU2223; Group; fwd; MA", "2223", "fwd", "No Fit", 0]]
        self.parameter_data = [[0, 1, 2], [3, 4, 5]]
        self.parameters = ["param1", "param2", "param3"]
        self.setup_test_data()

    def setup_test_data(self):
        self.model._defaultData = self.workspace_data.copy()
        self.model._parameterData = self.parameter_data.copy()
        self.model._parameterHeaders = self.parameters.copy()

    def get_column_data(self, column):
        data = self.model._defaultData + self.model._parameterData
        column_data = []
        for row in range(self.model.rowCount()):
            column_data += [data[row][column]]

        return column_data

    def test_rowcount_returns_correctly(self):
        rowCount = self.model.rowCount()
        self.assertEqual(rowCount, 2)

    def test_columncount_returns_correctly(self):
        columnCount = self.model.columnCount()
        self.assertEqual(columnCount, len(self.parameters) + len(default_table_columns))

    def test_data_returns_correctly_for_display_role(self):
        index = self.model.createIndex(1, len(default_table_columns) + 1)

        value = self.model.data(index, Qt.DisplayRole)

        self.assertEqual(value, 4)

    def test_set_data_correctly_sets_parameter_data(self):
        index = self.model.createIndex(1, len(default_table_columns) + 2)  # setting last parameter in final row
        new_value = 12

        self.model.setData(index, new_value, Qt.EditRole)

        self.assertEqual(self.model.data(index, Qt.DisplayRole), new_value)

    def test_header_data_correctly_returns_for_display_role(self):
        section = len(default_table_columns) + 1  # second parameter

        data = self.model.headerData(section, Qt.Horizontal, Qt.DisplayRole)

        self.assertEqual(self.parameters[1], data)

    def test_set_header_data_correctly_sets_parameter_header(self):
        section = len(default_table_columns) + 3  # insert parameter
        new_parameter = "new_param"

        self.model.setHeaderData(section, Qt.Horizontal, new_parameter)  # insert parameter

        self.assertEqual(self.model.headerData(section, Qt.Horizontal, Qt.DisplayRole), new_parameter)

    def test_insert_rows_correctly_adds_row(self):
        position = 2  #
        num_rows = 2  # add two new rows

        self.model.insertRows(position, num_rows)
        self.assertEqual(self.model.rowCount(), num_rows + len(self.workspace_data))

    def test_clear_fit_model_clears_table(self):
        self.model.clear_fit_parameters()

        self.assertEqual(self.model.columnCount(), len(default_table_columns))

    def test_clear_fit_workspaces_clears_workspaces_and_parameters(self):
        self.model.clear_fit_workspaces()

        self.assertEqual(self.model.columnCount(), len(default_table_columns))
        self.assertEqual(self.model.rowCount(), 0)

    def test_reset_fit_data_correctly_resets_to_default(self):
        for row in range(self.model.rowCount()):
            self.model._defaultData[row][FIT_STATUS_COLUMN] = 'test'

        self.model.reset_fit_quality()

        self.assertEqual(self.get_column_data(FIT_STATUS_COLUMN), ['No fit', 'No fit'])

    def test_set_fit_parameters(self):
        parameters = ["new_param1", "new_param2", "new_param3"]
        parameter_values = [[6, 7, 8], [9, 10, 11]]

        self.model.set_fit_parameters_and_values(parameters, parameter_values)

        self.assertEqual(self.model._parameterHeaders, parameters)
        self.assertEqual(self.model._parameterData, parameter_values)

    def test_set_fit_parameter_values_for_row(self):
        parameter_values = [12, 5, 7]
        row = 1

        self.model.set_fit_parameter_values_for_row(row, parameter_values)

        self.assertEqual(self.model._parameterData[row], parameter_values)

    def test_set_fit_parameter_values_for_column(self):
        parameter_value = 1.5
        column = 6

        self.model.set_fit_parameter_values_for_column(column, parameter_value)

        self.assertEqual(self.model._parameterData[0][1], parameter_value)
        self.assertEqual(self.model._parameterData[1][1], parameter_value)

    def test_set_fit_workspaces(self):
        new_workspaces = ["EMU2224; Group; top; MA", "EMU2224; bottom; bwd; MA"]
        new_runs = ['2224', '2224']
        new_groups = ['top', 'bottom']

        self.model.set_fit_workspaces(new_workspaces, new_runs, new_groups)

        for row in range(len(new_runs)):
            self.assertEqual(self.model.data(self.model.createIndex(row, WORKSPACE_COLUMN), Qt.DisplayRole),
                             new_workspaces[row])
            self.assertEqual(self.model.data(self.model.createIndex(row, RUN_COLUMN), Qt.DisplayRole), new_runs[row])
            self.assertEqual(self.model.data(self.model.createIndex(row, GROUP_COLUMN), Qt.DisplayRole), new_groups[row])
            self.assertEqual(self.model.data(self.model.createIndex(row, FIT_STATUS_COLUMN), Qt.DisplayRole), 'No fit')

    def test_get_workspace_name_information_returns_the_expected_data(self):
        new_workspaces = ["EMU2224; Group; top; MA", "EMU2224; bottom; bwd; MA"]
        new_runs = ['2224', '2224']
        new_groups = ['top', 'bottom']

        self.model.set_fit_workspaces(new_workspaces, new_runs, new_groups)

        for i in range(len(new_workspaces)):
            self.assertEqual(self.model.get_workspace_name_information(i), new_workspaces[i])

    def test_set_run_information(self):
        new_run = '2225'
        row = 1

        self.model.set_run_information(row, new_run)

        self.assertEqual(self.model.data(self.model.createIndex(row, RUN_COLUMN), Qt.DisplayRole), new_run)

    def test_set_group_information(self):
        new_group = 'long'
        row = 1

        self.model.set_group_information(row, new_group)

        self.assertEqual(self.model.data(self.model.createIndex(row, GROUP_COLUMN), Qt.DisplayRole), new_group)

    def test_set_fit_quality(self):
        fit_quality = 'success'
        chi_squared = 1.1215
        row = 1

        self.model.set_fit_quality(row, fit_quality, chi_squared)

        self.assertEqual(self.model.data(self.model.createIndex(row, FIT_STATUS_COLUMN), Qt.DisplayRole), fit_quality)
        self.assertEqual(self.model.data(self.model.createIndex(row, FIT_QUALITY_COLUMN), Qt.DisplayRole), chi_squared)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
