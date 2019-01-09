# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)

import unittest

from mock import Mock, call, patch

from mantidqt.widgets.matrixworkspacedisplay.test_helpers.matrixworkspacedisplay_common import MockQModelIndex, \
    MockWorkspace
from mantidqt.widgets.matrixworkspacedisplay.test_helpers.mock_matrixworkspacedisplay import MockQSelectionModel
from mantidqt.widgets.tableworkspacedisplay.error_column import ErrorColumn
from mantidqt.widgets.tableworkspacedisplay.model import TableWorkspaceDisplayModel
from mantidqt.widgets.tableworkspacedisplay.plot_type import PlotType
from mantidqt.widgets.tableworkspacedisplay.presenter import TableWorkspaceDisplay
from mantidqt.widgets.tableworkspacedisplay.test_helpers.mock_qtable import MockQTable
from mantidqt.widgets.tableworkspacedisplay.view import TableWorkspaceDisplayView
from mantidqt.widgets.tableworkspacedisplay.workbench_table_widget_item import WorkbenchTableWidgetItem


def with_mock_presenter(add_selection_model=False):
    """
    Decorators with Arguments are a load of callback fun. Sources that were used for reference:

    https://stackoverflow.com/a/5929165/2823526

    And an answer with a little more description of the logic behind it all
    https://stackoverflow.com/a/25827070/2823526

    :param add_selection_model:
    """

    def real_decorator(func, *args, **kwargs):
        def wrapper(self, *args):
            ws = MockWorkspace()
            view = Mock(spec=TableWorkspaceDisplayView)
            if add_selection_model:
                mock_selection_model = MockQSelectionModel(has_selection=True)
                mock_selection_model.selectedRows = Mock(
                    return_value=[MockQModelIndex(1, 1), MockQModelIndex(2, 2), MockQModelIndex(3, 3)])
                mock_selection_model.selectedColumns = Mock(
                    return_value=[MockQModelIndex(1, 1), MockQModelIndex(2, 2), MockQModelIndex(3, 3)])
                view.mock_selection_model = mock_selection_model
                view.selectionModel.return_value = mock_selection_model
            twd = TableWorkspaceDisplay(ws, view=view)
            return func(self, ws, view, twd, *args)

        return wrapper

    return real_decorator


class TableWorkspaceDisplayPresenterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Allow the MockWorkspace to work within the model
        TableWorkspaceDisplayModel.ALLOWED_WORKSPACE_TYPES.append(MockWorkspace)

    def test_supports(self):
        ws = MockWorkspace()
        # the test will fail if the support check fails - an exception is raised
        TableWorkspaceDisplay.supports(ws)

    def test_handleItemChanged(self):
        ws = MockWorkspace()
        view = Mock(spec=TableWorkspaceDisplayView)
        twd = TableWorkspaceDisplay(ws, view=view)
        item = Mock(spec=WorkbenchTableWidgetItem)
        item.row.return_value = 5
        item.column.return_value = 5
        item.data.return_value = "magic parameter"
        item.is_v3d = False

        twd.handleItemChanged(item)

        item.row.assert_called_once()
        item.column.assert_called_once()
        ws.setCell.assert_called_once_with(5, 5, "magic parameter")
        item.update.assert_called_once()
        item.reset.assert_called_once()

    @with_mock_presenter
    def test_handleItemChanged_raises_ValueError(self, ws, view, twd):
        item = Mock(spec=WorkbenchTableWidgetItem)
        item.row.return_value = 5
        item.column.return_value = 5
        item.data.return_value = "magic parameter"
        item.is_v3d = False

        # setCell will throw an exception as a side effect
        ws.setCell.side_effect = ValueError

        twd.handleItemChanged(item)

        item.row.assert_called_once()
        item.column.assert_called_once()
        ws.setCell.assert_called_once_with(5, 5, "magic parameter")
        view.show_warning.assert_called_once_with(TableWorkspaceDisplay.ITEM_CHANGED_INVALID_DATA_MESSAGE)
        item.update.assert_not_called()
        item.reset.assert_called_once()

    @with_mock_presenter
    def test_handleItemChanged_raises_Exception(self, ws, view, twd):
        item = Mock(spec=WorkbenchTableWidgetItem)

        item.row.return_value = ws.ROWS
        item.column.return_value = ws.COLS
        item.data.return_value = "magic parameter"
        item.is_v3d = False

        # setCell will throw an exception as a side effect
        error_message = "TEST_EXCEPTION_MESSAGE"
        ws.setCell.side_effect = Exception(error_message)

        twd.handleItemChanged(item)

        item.row.assert_called_once()
        item.column.assert_called_once()
        ws.setCell.assert_called_once_with(ws.ROWS, ws.COLS, "magic parameter")
        view.show_warning.assert_called_once_with(
            TableWorkspaceDisplay.ITEM_CHANGED_UNKNOWN_ERROR_MESSAGE.format(error_message))
        item.update.assert_not_called()
        item.reset.assert_called_once()

    @with_mock_presenter
    def test_update_column_headers(self, ws, view, twd):
        twd.update_column_headers()

        # setColumnCount is done twice - once in the TableWorkspaceDisplay initialisation, and once in the call
        # to update_column_headers above
        view.setColumnCount.assert_has_calls([call(ws.ROWS), call(ws.ROWS)])

    @with_mock_presenter
    def test_load_data(self, ws, _, twd):
        mock_table = MockQTable()
        twd.load_data(mock_table)

        mock_table.setRowCount.assert_called_once_with(ws.ROWS)
        ws.columnCount.assert_called_once()
        # set item is called on every item of the table
        self.assertEqual(ws.ROWS * ws.COLS, mock_table.setItem.call_count)

    @patch('mantidqt.widgets.tableworkspacedisplay.presenter.copy_cells')
    @with_mock_presenter
    def test_action_copying(self, ws, view, twd, mock_copy_cells):
        twd.action_copy_cells()
        self.assertEqual(1, mock_copy_cells.call_count)
        twd.action_copy_bin_values()
        self.assertEqual(2, mock_copy_cells.call_count)
        twd.action_copy_spectrum_values()
        self.assertEqual(3, mock_copy_cells.call_count)
        twd.action_keypress_copy()
        self.assertEqual(4, mock_copy_cells.call_count)

    @patch('mantidqt.widgets.tableworkspacedisplay.presenter.DeleteTableRows')
    @with_mock_presenter(add_selection_model=True)
    def test_action_delete_row(self, ws, view, twd, mock_DeleteTableRows):
        twd.action_delete_row()
        mock_DeleteTableRows.assert_called_once_with(ws, "1,2,3")
        view.mock_selection_model.hasSelection.assert_called_once()
        view.mock_selection_model.selectedRows.assert_called_once()

    @patch('mantidqt.widgets.tableworkspacedisplay.presenter.show_no_selection_to_copy_toast')
    @with_mock_presenter(add_selection_model=True)
    def test_action_delete_row_no_selection(self, ws, view, twd, mock_no_selection_toast):
        view.mock_selection_model.hasSelection = Mock(return_value=False)
        twd.action_delete_row()
        view.mock_selection_model.hasSelection.assert_called_once()
        mock_no_selection_toast.assert_called_once()
        view.mock_selection_model.selectedRows.assert_not_called()

    @with_mock_presenter(add_selection_model=True)
    def test_get_selected_columns(self, ws, view, twd):
        result = twd._get_selected_columns()
        self.assertEqual([1, 2, 3], result)

    @patch('mantidqt.widgets.tableworkspacedisplay.presenter.show_no_selection_to_copy_toast')
    @with_mock_presenter(add_selection_model=True)
    def test_get_selected_columns_no_selection(self, ws, view, twd, mock_no_selection_toast):
        view.mock_selection_model.hasSelection = Mock(return_value=False)
        self.assertRaises(ValueError, twd._get_selected_columns)
        mock_no_selection_toast.assert_called_once()

    @with_mock_presenter(add_selection_model=True)
    def test_get_selected_columns_over_max_selected(self, ws, view, twd):
        mock_message = "Hi."
        self.assertRaises(ValueError, twd._get_selected_columns, max_selected=1, message_if_over_max=mock_message)
        view.show_warning.assert_called_once_with(mock_message)

    @patch('mantidqt.widgets.tableworkspacedisplay.presenter.show_no_selection_to_copy_toast')
    @with_mock_presenter(add_selection_model=True)
    def test_get_selected_columns_has_selected_but_no_columns(self, ws, view, twd, mock_no_selection_toast):
        """
        There is a case where the user could have a selection (of cells or rows), but not columns.
        """
        view.mock_selection_model.selectedColumns = Mock(return_value=[])
        self.assertRaises(ValueError, twd._get_selected_columns)
        mock_no_selection_toast.assert_called_once()
        view.mock_selection_model.selectedColumns.assert_called_once()

    @patch('mantidqt.widgets.tableworkspacedisplay.presenter.TableWorkspaceDisplay')
    @patch('mantidqt.widgets.tableworkspacedisplay.presenter.StatisticsOfTableWorkspace')
    @with_mock_presenter(add_selection_model=True)
    def test_action_statistics_on_columns(self, ws, view, twd, mock_StatisticsOfTableWorkspace,
                                          mock_TableWorkspaceDisplay):
        twd.action_statistics_on_columns()

        mock_StatisticsOfTableWorkspace.assert_called_once_with(ws, [1, 2, 3])
        # check that there was an attempt to make a new TableWorkspaceDisplay window
        self.assertEqual(1, mock_TableWorkspaceDisplay.call_count)

    @with_mock_presenter(add_selection_model=True)
    def test_action_hide_selected(self, ws, view, twd):
        twd.action_hide_selected()
        view.hideColumn.assert_has_calls([call(1), call(2), call(3)])

    @with_mock_presenter(add_selection_model=True)
    def test_action_show_all_columns(self, ws, view, twd):
        view.columnCount = Mock(return_value=15)
        twd.action_show_all_columns()
        self.assertEqual(15, view.showColumn.call_count)

    @with_mock_presenter(add_selection_model=True)
    def test_action_set_as(self, ws, view, twd):
        mock_func = Mock()
        twd._action_set_as(mock_func)

        self.assertEqual(3, mock_func.call_count)

    @with_mock_presenter(add_selection_model=True)
    def test_action_set_as_x(self, ws, view, twd):
        twd.action_set_as_x()

        self.assertEqual(3, len(twd.model.marked_columns.as_x))

    @with_mock_presenter(add_selection_model=True)
    def test_action_set_as_y(self, ws, view, twd):
        twd.action_set_as_y()

        self.assertEqual(3, len(twd.model.marked_columns.as_y))

    @with_mock_presenter(add_selection_model=True)
    def test_action_set_as_none(self, ws, view, twd):
        twd.action_set_as_none()

        self.assertEqual(0, len(twd.model.marked_columns.as_x))
        self.assertEqual(0, len(twd.model.marked_columns.as_y))
        self.assertEqual(0, len(twd.model.marked_columns.as_y_err))

    @with_mock_presenter(add_selection_model=True)
    def test_action_set_as_y_err(self, ws, view, twd):
        view.mock_selection_model.selectedColumns = Mock(return_value=[MockQModelIndex(1, 1)])
        twd.action_set_as_y_err(2, "0")
        self.assertEqual(1, len(twd.model.marked_columns.as_y_err))
        err_col = twd.model.marked_columns.as_y_err[0]
        self.assertEqual(1, err_col.column)
        self.assertEqual(2, err_col.related_y_column)
        self.assertEqual("0", err_col.label_index)

    @with_mock_presenter(add_selection_model=True)
    def test_action_set_as_y_err_too_many_selected(self, ws, view, twd):
        twd.action_set_as_y_err(2, "0")
        view.show_warning.assert_called_once_with(TableWorkspaceDisplay.TOO_MANY_TO_SET_AS_Y_ERR_MESSAGE)

    @with_mock_presenter(add_selection_model=True)
    def test_action_set_as_y_err_failed_to_create_ErrorColumn(self, ws, view, twd):
        view.mock_selection_model.selectedColumns = Mock(return_value=[MockQModelIndex(1, 1)])
        # this will fail as we're trying to set an YErr column for itself -> (try to set col 1 to be YERR for col 1)
        twd.action_set_as_y_err(1, "0")
        view.show_warning.assert_called_once_with(ErrorColumn.CANNOT_SET_Y_TO_BE_OWN_YERR_MESSAGE)

    @with_mock_presenter(add_selection_model=True)
    def test_action_sort(self, ws, view, twd):
        view.mock_selection_model.selectedColumns = Mock(return_value=[MockQModelIndex(0, 4444)])
        order = 1
        twd.action_sort(order)
        view.sortByColumn.assert_called_once_with(4444, order)

    @with_mock_presenter(add_selection_model=True)
    def test_action_sort_too_many(self, ws, view, twd):
        twd.action_sort(1)
        view.show_warning.assert_called_once_with(TableWorkspaceDisplay.TOO_MANY_SELECTED_TO_SORT)

    @with_mock_presenter(add_selection_model=True)
    def test_get_plot_function_from_type(self, ws, view, twd):
        class MockAx:
            def __init__(self):
                self.plot = Mock()
                self.scatter = Mock()
                self.errorbar = Mock()

        mock_ax = MockAx()
        twd._get_plot_function_from_type(mock_ax, PlotType.LINEAR)
        mock_ax.plot.assert_called_once()

        mock_ax = MockAx()
        twd._get_plot_function_from_type(mock_ax, PlotType.SCATTER)
        mock_ax.scatter.assert_called_once()

        mock_ax = MockAx()
        twd._get_plot_function_from_type(mock_ax, PlotType.LINE_AND_SYMBOL)
        mock_ax.plot.assert_called_once()

        mock_ax = MockAx()
        twd._get_plot_function_from_type(mock_ax, PlotType.LINEAR_WITH_ERR)
        mock_ax.errorbar.assert_called_once()

        self.assertRaises(ValueError, twd._get_plot_function_from_type, None, 48903479)

    @with_mock_presenter(add_selection_model=True)
    def test_action_plot(self, ws, view, twd):
        self.fail("Not implemetned")


if __name__ == '__main__':
    unittest.main()
