# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import sys
import unittest

from qtpy.QtWidgets import QStatusBar
from qtpy.QtCore import QItemSelectionModel

from unittest.mock import Mock, call, patch
from mantid.simpleapi import CreateEmptyTableWorkspace
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.testing.mocks.mock_mantid import MockWorkspace
from mantidqt.utils.testing.mocks.mock_plotlib import MockAx, MockPlotLib
from mantidqt.utils.testing.mocks.mock_qt import MockQModelIndex, MockQSelectionModel
from mantidqt.widgets.workspacedisplay.status_bar_view import StatusBarView
from mantidqt.widgets.workspacedisplay.table.error_column import ErrorColumn
from mantidqt.widgets.workspacedisplay.table.model import TableWorkspaceDisplayModel
from mantidqt.widgets.workspacedisplay.table.plot_type import PlotType
from mantidqt.widgets.workspacedisplay.table.presenter import TableWorkspaceDisplay
from mantidqt.widgets.workspacedisplay.table.view import TableWorkspaceDisplayView
from mantidqt.widgets.workspacedisplay.table.tableworkspace_item import QStandardItem, RevertibleItem


class MockQTable:
    """
    Mocks the necessary functions to replace a QTableView on which data is being set.
    """

    def __init__(self):
        self.setItem = Mock()
        self.setRowCount = Mock()


def with_mock_presenter(add_selection_model=False, add_plot=False):
    """
    Decorators with Arguments are a load of callback fun. Sources that were used for reference:

    https://stackoverflow.com/a/5929165/2823526

    And an answer with a little more description of the logic behind it all
    https://stackoverflow.com/a/25827070/2823526

    :param add_selection_model: Adds a mock selection model to the presenter
    :param add_plot: Adds mock plotting to the presenter
    """

    def real_decorator(func, *args, **kwargs):
        def wrapper(self, *args):
            ws = MockWorkspace()
            ws.setPlotType = Mock()
            view = Mock(spec=TableWorkspaceDisplayView)
            container = Mock(spec=StatusBarView)
            container.status_bar = Mock(spec=QStatusBar)
            if add_selection_model:
                mock_selection_model = MockQSelectionModel(has_selection=True)
                mock_selection_model.selectedRows = Mock(return_value=[MockQModelIndex(1, 1), MockQModelIndex(2, 2), MockQModelIndex(3, 3)])
                mock_selection_model.selectedColumns = Mock(
                    return_value=[MockQModelIndex(1, 1), MockQModelIndex(2, 2), MockQModelIndex(3, 3)]
                )
                view.mock_selection_model = mock_selection_model
                view.selectionModel.return_value = mock_selection_model
            twd = TableWorkspaceDisplay(ws, view=view, container=container)
            if add_plot:
                twd.plot = MockPlotLib()
            return func(self, ws, view, twd, *args)

        return wrapper

    return real_decorator


@start_qapplication
class TableWorkspaceDisplayPresenterTest(unittest.TestCase):
    notify_no_selection_to_copy_package = "mantidqt.widgets.workspacedisplay.user_notifier.UserNotifier.notify_no_selection_to_copy"
    copy_cells_package = "mantidqt.widgets.workspacedisplay.data_copier.DataCopier.copy_cells"

    @classmethod
    def setUpClass(cls):
        # Allow the MockWorkspace to work within the model
        TableWorkspaceDisplayModel.ALLOWED_WORKSPACE_TYPES.append(MockWorkspace)
        # mock out the simpleapi calls. patch cannot be used as the imports calls are
        # within the functions to keep the module import light
        cls.mock_DeleteTableRows = Mock()
        cls.mock_StatisticsOfTableWorkspace = Mock()
        cls.mock_SortPeaksWorkspace = Mock()
        cls.mock_SortTableWorkspace = Mock()
        mock_simpleapi = Mock()
        mock_simpleapi.DeleteTableRows = cls.mock_DeleteTableRows
        mock_simpleapi.StatisticsOfTableWorkspace = cls.mock_StatisticsOfTableWorkspace
        mock_simpleapi.SortPeaksWorkspace = cls.mock_SortPeaksWorkspace
        mock_simpleapi.SortTableWorkspace = cls.mock_SortTableWorkspace
        sys.modules["mantid.simpleapi"] = mock_simpleapi

    def setUp(self):
        self.mock_DeleteTableRows.reset_mock()
        self.mock_StatisticsOfTableWorkspace.reset_mock()
        self.mock_SortPeaksWorkspace.reset_mock()
        self.mock_SortTableWorkspace.reset_mock()

    def assertNotCalled(self, mock):
        self.assertEqual(0, mock.call_count)

    def test_supports(self):
        ws = MockWorkspace()
        # the test will fail if the support check fails - an exception is raised
        TableWorkspaceDisplay.supports(ws)

    @with_mock_presenter
    def test_handleItemChanged(self, ws, view, twd):
        items = [Mock(spec=RevertibleItem), Mock(spec=QStandardItem)]
        for item in items:
            item.row.return_value = 5
            item.column.return_value = 5
            item.data.return_value = "magic parameter"

            twd.handleItemChanged(item)

            item.row.assert_called_once_with()
            item.column.assert_called_once_with()
            ws.setCell.assert_called_once_with(5, 5, "magic parameter", notify_replace=False)
            item.update.assert_called_once_with()
            item.reset.assert_called_once_with()

    @with_mock_presenter
    def test_handleItemChanged_raises_ValueError(self, ws, view, twd):
        items = [Mock(spec=RevertibleItem), Mock(spec=QStandardItem)]
        for item in items:
            item.row.return_value = 5
            item.column.return_value = 5
            item.data.return_value = "magic parameter"

            # setCell will throw an exception as a side effect
            ws.setCell.side_effect = ValueError

            twd.handleItemChanged(item)

            item.row.assert_called_once_with()
            item.column.assert_called_once_with()
            ws.setCell.assert_called_once_with(5, 5, "magic parameter")
            view.show_warning.assert_called_once_with(TableWorkspaceDisplay.ITEM_CHANGED_INVALID_DATA_MESSAGE)
            self.assertNotCalled(item.update)
            item.reset.assert_called_once_with()

    @with_mock_presenter
    def test_handleItemChanged_raises_Exception(self, ws, view, twd):
        items = [Mock(spec=RevertibleItem), Mock(spec=QStandardItem)]
        for item in items:
            item.row.return_value = ws.ROWS
            item.column.return_value = ws.COLS
            item.data.return_value = "magic parameter"
            item.is_v3d = False

            # setCell will throw an exception as a side effect
            error_message = "TEST_EXCEPTION_MESSAGE"
            ws.setCell.side_effect = Exception(error_message)

            twd.handleItemChanged(item)

            item.row.assert_called_once_with()
            item.column.assert_called_once_with()
            ws.setCell.assert_called_once_with(ws.ROWS, ws.COLS, "magic parameter")
            view.show_warning.assert_called_once_with(TableWorkspaceDisplay.ITEM_CHANGED_UNKNOWN_ERROR_MESSAGE.format(error_message))
            self.assertNotCalled(item.update)
            item.reset.assert_called_once_with()

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
        ws.columnCount.assert_called_once_with()
        # set item is called on every item of the table
        self.assertEqual(ws.ROWS * ws.COLS, mock_table.setItem.call_count)

    @patch(copy_cells_package)
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

    def _mock_clipboard(self, clip):
        """Used to replace the clipboard call when copying data, but retain the data in mock_clip"""
        self.mock_clip = clip

    def test_copy_cells(self):
        """Check that the data copied from cells is correct."""
        ws = CreateEmptyTableWorkspace()
        # Add some data to the table
        ws.addColumn(type="int", name="test column")
        test_value = 3
        ws.addRow([test_value])

        # Currently copying data only works if batch is true.
        table = TableWorkspaceDisplay(ws, batch=True)
        # Mock the copy_to_clipboard function with a side_effect that stores the clipboard content in a member variable.
        table.copy_to_clipboard = Mock(side_effect=self._mock_clipboard)

        selection = table.view.selectionModel()
        model_index = table.view.model().createIndex(0, 0)
        selection.select(model_index, QItemSelectionModel.ClearAndSelect)

        # Copy data to clipboard
        table.action_copy_cells()

        # Value copied to clipboard should be the same as value in workspace.
        self.assertEqual(str(test_value), self.mock_clip)

    @with_mock_presenter(add_selection_model=True)
    def test_action_delete_row(self, ws, view, twd):
        twd.action_delete_row()
        self.mock_DeleteTableRows.assert_called_once_with(ws, "1,2,3")
        view.mock_selection_model.hasSelection.assert_called_once_with()
        view.mock_selection_model.selectedRows.assert_called_once_with()

    @patch(notify_no_selection_to_copy_package)
    @with_mock_presenter(add_selection_model=True)
    def test_action_delete_row_no_selection(self, ws, view, twd, mock_no_selection_toast):
        view.mock_selection_model.hasSelection = Mock(return_value=False)
        twd.action_delete_row()
        view.mock_selection_model.hasSelection.assert_called_once_with()
        self.assertEqual(1, mock_no_selection_toast.call_count)
        self.assertNotCalled(view.mock_selection_model.selectedRows)

    @with_mock_presenter(add_selection_model=True)
    def test_get_selected_columns(self, ws, view, twd):
        result = twd._get_selected_columns()
        self.assertEqual([1, 2, 3], result)

    @patch(notify_no_selection_to_copy_package)
    @with_mock_presenter(add_selection_model=True)
    def test_get_selected_columns_no_selection(self, ws, view, twd, mock_no_selection_toast):
        view.mock_selection_model.hasSelection = Mock(return_value=False)
        self.assertRaises(ValueError, twd._get_selected_columns)
        self.assertEqual(1, mock_no_selection_toast.call_count)

    @with_mock_presenter(add_selection_model=True)
    def test_get_selected_columns_over_max_selected(self, ws, view, twd):
        mock_message = "Hi."
        self.assertRaises(ValueError, twd._get_selected_columns, max_selected=1, message_if_over_max=mock_message)
        view.show_warning.assert_called_once_with(mock_message)

    @patch(notify_no_selection_to_copy_package)
    @with_mock_presenter(add_selection_model=True)
    def test_get_selected_columns_has_selected_but_no_columns(self, ws, view, twd, mock_no_selection_toast):
        """
        There is a case where the user could have a selection (of cells or rows), but not columns.
        """
        view.mock_selection_model.selectedColumns = Mock(return_value=[])
        self.assertRaises(ValueError, twd._get_selected_columns)
        self.assertEqual(1, mock_no_selection_toast.call_count)
        view.mock_selection_model.selectedColumns.assert_called_once_with()

    @patch("mantidqt.widgets.workspacedisplay.table.presenter.TableWorkspaceDisplay", spec=TableWorkspaceDisplay)
    @with_mock_presenter(add_selection_model=True)
    def test_action_statistics_on_columns(self, ws, view, twd, mock_TableWorkspaceDisplay):
        twd.action_statistics_on_columns()

        self.mock_StatisticsOfTableWorkspace.assert_called_once_with(ws, [1, 2, 3])
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
        twd._action_set_as(mock_func, 1)

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
        twd.action_set_as_y_err(2)
        self.assertEqual(1, len(twd.model.marked_columns.as_y_err))
        err_col = twd.model.marked_columns.as_y_err[0]
        self.assertEqual(1, err_col.column)
        self.assertEqual(2, err_col.related_y_column)

    @with_mock_presenter(add_selection_model=True)
    def test_action_set_as_y_err_too_many_selected(self, ws, view, twd):
        twd.action_set_as_y_err(2)
        view.show_warning.assert_called_once_with(TableWorkspaceDisplay.TOO_MANY_TO_SET_AS_Y_ERR_MESSAGE)

    @with_mock_presenter(add_selection_model=True)
    def test_action_set_as_y_err_failed_to_create_ErrorColumn(self, ws, view, twd):
        view.mock_selection_model.selectedColumns = Mock(return_value=[MockQModelIndex(1, 1)])
        # this will fail as we're trying to set an YErr column for itself -> (try to set col 1 to be YERR for col 1)
        twd.action_set_as_y_err(1)
        view.show_warning.assert_called_once_with(ErrorColumn.CANNOT_SET_Y_TO_BE_OWN_YERR_MESSAGE)

    @with_mock_presenter(add_selection_model=True)
    def test_action_sort_table_ws(self, ws, view, twd):
        view.mock_selection_model.selectedColumns = Mock(return_value=[MockQModelIndex(0, 0)])
        ascending = True
        twd.action_sort(ascending)
        self.mock_SortTableWorkspace.assert_called_once_with(
            InputWorkspace=twd.model.ws, OutputWorkspace=twd.model.ws, Columns="col0", Ascending=ascending
        )

    @with_mock_presenter(add_selection_model=True)
    def test_action_sort_peaks_ws(self, ws, view, twd):
        view.mock_selection_model.selectedColumns = Mock(return_value=[MockQModelIndex(0, 0)])
        ascending = True
        with patch(
            "mantidqt.widgets.workspacedisplay.table.model.TableWorkspaceDisplayModel.is_peaks_workspace", return_value=True
        ) as mock_is_peaks_workspace:
            twd.action_sort(ascending)
            self.mock_SortPeaksWorkspace.assert_called_once_with(
                InputWorkspace=twd.model.ws, OutputWorkspace=twd.model.ws, ColumnNameToSortBy="col0", SortAscending=ascending
            )
            mock_is_peaks_workspace.assert_called_once_with()

    @with_mock_presenter(add_selection_model=True)
    def test_action_sort_too_many(self, ws, view, twd):
        twd.action_sort(1)
        # by default we have more than 1 column selected
        view.show_warning.assert_called_once_with(TableWorkspaceDisplay.TOO_MANY_SELECTED_TO_SORT)

    @with_mock_presenter(add_selection_model=True)
    def test_get_plot_function_from_type(self, ws, view, twd):
        mock_ax = MockAx()
        result = twd._get_plot_function_from_type(mock_ax, PlotType.LINEAR)
        self.assertEqual(result, mock_ax.plot)

        mock_ax = MockAx()
        result = twd._get_plot_function_from_type(mock_ax, PlotType.SCATTER)
        self.assertEqual(result, mock_ax.scatter)

        mock_ax = MockAx()
        result = twd._get_plot_function_from_type(mock_ax, PlotType.LINE_AND_SYMBOL)
        # the function created for LINE_AND_SYMBOL is a decorated ax.plot
        self.assertTrue("functools.partial" in str(type(result)))
        self.assertIsNotNone(result)

        mock_ax = MockAx()
        result = twd._get_plot_function_from_type(mock_ax, PlotType.LINEAR_WITH_ERR)
        self.assertEqual(result, mock_ax.errorbar)

        invalid_plot_type = 48903479
        self.assertRaises(ValueError, twd._get_plot_function_from_type, None, invalid_plot_type)

    @patch(notify_no_selection_to_copy_package)
    @with_mock_presenter(add_selection_model=True)
    def test_action_plot_no_selected_columns(self, ws, view, twd, mock_notify_no_selection_to_copy):
        view.mock_selection_model.selectedColumns.return_value = []
        twd.action_plot(PlotType.LINEAR)
        mock_notify_no_selection_to_copy.assert_called_once_with()

    @with_mock_presenter(add_selection_model=True)
    def test_action_plot_more_than_one_x(self, ws, view, twd):
        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, 1), MockQModelIndex(1, 2)]
        twd.action_set_as_x()
        twd.action_plot(PlotType.LINEAR)
        view.mock_selection_model.selectedColumns.assert_has_calls([call(), call()])
        view.show_warning.assert_called_once_with(TableWorkspaceDisplay.TOO_MANY_SELECTED_FOR_X)

    @patch("mantidqt.widgets.workspacedisplay.table.presenter.TableWorkspaceDisplay._do_plot")
    @with_mock_presenter(add_selection_model=True)
    def test_action_plot_x_in_selection(self, ws, view, twd, mock_do_plot):
        """
        Test that the plot is successful if there is an X in the selection,
        and an unmarked column: which is used as the Y data
        """
        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, 1)]
        # set only the first column to be X
        twd.action_set_as_x()
        # add a second selected column, that should be used for Y
        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, 1), MockQModelIndex(1, 2)]
        twd.action_plot(PlotType.LINEAR)
        mock_do_plot.assert_called_once_with([2], 1, PlotType.LINEAR)

    @patch("mantidqt.widgets.workspacedisplay.table.presenter.TableWorkspaceDisplay._do_plot")
    @with_mock_presenter(add_selection_model=True)
    def test_action_plot_x_marked_but_not_selected(self, ws, view, twd, mock_do_plot):
        """
        Test that the plot is successful if there is no X in the selection, but a column is marked X.
        The selection contains only an unmarked column, which is used as the Y data
        """
        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, 1)]
        # set only the first column to be X
        twd.action_set_as_x()
        # change the selection to a second column, that should be used for Y, but is not marked as anything
        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, 2)]
        twd.action_plot(PlotType.LINEAR)
        mock_do_plot.assert_called_once_with([2], 1, PlotType.LINEAR)

    @patch("mantidqt.widgets.workspacedisplay.table.presenter.TableWorkspaceDisplay._do_plot")
    @with_mock_presenter(add_selection_model=True)
    def test_action_plot_selection_without_x(self, ws, view, twd, mock_do_plot):
        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, 1)]
        twd.action_plot(PlotType.LINEAR)
        view.show_warning.assert_called_once_with(TableWorkspaceDisplay.NO_COLUMN_MARKED_AS_X)

    @with_mock_presenter(add_selection_model=True)
    def test_action_plot_column_against_itself(self, ws, view, twd):
        """
        For example: mark a column as X and then try to do Right click -> plot -> line on it, using it as Y
        this will fail as it's the same column
        """
        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, 1)]
        # set only the first column to be X
        twd.action_set_as_x()
        # change the selection to a second column, that should be used for Y, but is not marked as anything
        twd.action_plot(PlotType.LINEAR)
        view.show_warning.assert_called_once_with(TableWorkspaceDisplay.CANNOT_PLOT_AGAINST_SELF_MESSAGE)

    @with_mock_presenter(add_selection_model=True)
    def test_do_action_plot_with_errors_missing_yerr_for_y_column(self, ws, view, twd):
        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, 1)]
        twd.action_set_as_x()

        y_column_index = 2
        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, y_column_index)]
        twd.action_set_as_y()

        twd.action_plot(PlotType.LINEAR_WITH_ERR)
        view.show_warning.assert_called_once_with(
            TableWorkspaceDisplay.NO_ASSOCIATED_YERR_FOR_EACH_Y_MESSAGE.format(ws._column_names[y_column_index])
        )

    @patch("mantidqt.widgets.workspacedisplay.table.presenter.logger.error")
    @with_mock_presenter(add_selection_model=True, add_plot=True)
    def test_do_action_plot__plot_func_throws_error(self, ws, view, twd, mock_logger_error):
        mock_plot_function = Mock()
        error_message = "See bottom of keyboard for HEALTH WARNING"
        mock_plot_function.side_effect = ValueError(error_message)
        with patch(
            "mantidqt.widgets.workspacedisplay.table.presenter.TableWorkspaceDisplay._get_plot_function_from_type"
        ) as mock_get_plot_function_from_type:
            mock_get_plot_function_from_type.return_value = mock_plot_function
            view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, 1)]
            twd.action_set_as_x()

            view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, 2)]
            twd.action_plot(PlotType.LINEAR)

            view.show_warning.assert_called_once_with(
                TableWorkspaceDisplay.PLOT_FUNCTION_ERROR_MESSAGE.format(error_message), TableWorkspaceDisplay.INVALID_DATA_WINDOW_TITLE
            )
            mock_logger_error.assert_called_once_with(TableWorkspaceDisplay.PLOT_FUNCTION_ERROR_MESSAGE.format(error_message))
        self.assertNotCalled(twd.plot.mock_fig.show)
        self.assertNotCalled(twd.plot.mock_ax.legend)

    @with_mock_presenter(add_selection_model=True, add_plot=True)
    def test_do_action_plot_success(self, ws, view, twd):
        col_as_x = 1
        col_as_y = 2
        expected_x_data = twd.model.get_column(col_as_x)
        expected_y_data = twd.model.get_column(col_as_y)

        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, col_as_x)]
        twd.action_set_as_x()

        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, col_as_y)]
        twd.action_plot(PlotType.LINEAR)

        twd.plot.subplots.assert_called_once_with(subplot_kw={"projection": "mantid"})
        twd.plot.mock_fig.canvas.manager.set_window_title.assert_called_once_with(twd.model.get_name())
        twd.plot.mock_ax.set_xlabel.assert_called_once_with(twd.model.get_column_header(col_as_x))
        col_y_name = twd.model.get_column_header(col_as_y)
        twd.plot.mock_ax.set_ylabel.assert_called_once_with(col_y_name)
        twd.plot.mock_ax.plot.assert_called_once_with(
            expected_x_data, expected_y_data, label=TableWorkspaceDisplay.COLUMN_DISPLAY_LABEL.format(col_y_name)
        )
        twd.plot.mock_fig.show.assert_called_once_with()
        twd.plot.mock_ax.legend.assert_called_once_with()

    @with_mock_presenter(add_selection_model=True, add_plot=True)
    def test_do_action_plot_multiple_y_success(self, ws, view, twd):
        col_as_x = 1
        col_as_y1 = 2
        col_as_y2 = 3
        expected_x_data = twd.model.get_column(col_as_x)
        expected_y1_data = twd.model.get_column(col_as_y1)
        expected_y2_data = twd.model.get_column(col_as_y2)

        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, col_as_x)]
        twd.action_set_as_x()

        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, col_as_y1), MockQModelIndex(1, col_as_y2)]
        twd.action_plot(PlotType.LINEAR)

        twd.plot.subplots.assert_called_once_with(subplot_kw={"projection": "mantid"})
        twd.plot.mock_fig.canvas.manager.set_window_title.assert_called_once_with(twd.model.get_name())
        twd.plot.mock_ax.set_xlabel.assert_called_once_with(twd.model.get_column_header(col_as_x))

        col_y1_name = twd.model.get_column_header(col_as_y1)
        col_y2_name = twd.model.get_column_header(col_as_y2)
        twd.plot.mock_ax.set_ylabel.assert_has_calls([call(col_y1_name), call(col_y2_name)])

        twd.plot.mock_ax.plot.assert_has_calls(
            [
                call(expected_x_data, expected_y1_data, label=TableWorkspaceDisplay.COLUMN_DISPLAY_LABEL.format(col_y1_name)),
                call(expected_x_data, expected_y2_data, label=TableWorkspaceDisplay.COLUMN_DISPLAY_LABEL.format(col_y2_name)),
            ]
        )
        twd.plot.mock_fig.show.assert_called_once_with()
        twd.plot.mock_ax.legend.assert_called_once_with()

    @with_mock_presenter(add_selection_model=True, add_plot=True)
    def test_do_action_plot_linear_error_plot(self, ws, view, twd):
        """
        Test for _Linear_ plotting (with errors) a single Y column in selection, which has an associated Y error column
        """
        self.do_test_plot_single_y_with_error(view, twd, PlotType.LINEAR_WITH_ERR)

    @with_mock_presenter(add_selection_model=True, add_plot=True)
    def test_do_action_plot_scatter_error_plot(self, ws, view, twd):
        """
        Test for _Scatter_ plotting (with errors) a single Y column in selection, which has an associated Y error column
        """
        self.do_test_plot_single_y_with_error(view, twd, PlotType.SCATTER_WITH_ERR, {"fmt": "o"})

    @with_mock_presenter(add_selection_model=True, add_plot=True)
    def test_do_action_plot_linear_error_plot_append_yerr(self, ws, view, twd):
        """
        Test for _Linear_ plotting (with errors) a single Y column in selection, which has an associated Y error column.

        This tests the case where the Y Error column is part of the user's selection
        """
        self.do_test_plot_single_y_with_error(view, twd, PlotType.LINEAR_WITH_ERR, append_yerr_to_selection=True)

    @with_mock_presenter(add_selection_model=True, add_plot=True)
    def test_do_action_plot_scatter_error_plot_append_yerr(self, ws, view, twd):
        """
        Test for _Scatter_ plotting (with errors) a single Y column in selection, which has an associated Y error column.

        This tests the case where the Y Error column is part of the user's selection
        """
        self.do_test_plot_single_y_with_error(view, twd, PlotType.SCATTER_WITH_ERR, {"fmt": "o"}, append_yerr_to_selection=True)

    def do_test_plot_single_y_with_error(self, view, twd, plot_type, extra_errorbar_assert_kwargs=None, append_yerr_to_selection=False):
        """
        Does the test for plotting with a single Y column that has an associated error
        :param twd: The presenter
        :param view: The mock view
        :param plot_type: The type of the plot
        :param extra_errorbar_assert_kwargs: Extra arguments expanded in the assertion for the errorbar call
        :return:
        """
        if extra_errorbar_assert_kwargs is None:
            extra_errorbar_assert_kwargs = {}
        col_as_x = 1
        col_as_y = 2
        col_as_y_err = 3
        expected_x_data = twd.model.get_column(col_as_x)
        expected_y_data = twd.model.get_column(col_as_y)
        expected_y_err_data = twd.model.get_column(col_as_y_err)

        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, col_as_x)]
        twd.action_set_as_x()
        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, col_as_y_err)]
        twd.action_set_as_y_err(col_as_y)
        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, col_as_y)]
        if append_yerr_to_selection:
            view.mock_selection_model.selectedColumns.return_value.append(MockQModelIndex(1, col_as_y_err))

        twd.action_plot(plot_type)

        twd.plot.subplots.assert_called_once_with(subplot_kw={"projection": "mantid"})

        twd.plot.mock_fig.canvas.manager.set_window_title.assert_called_once_with(twd.model.get_name())
        twd.plot.mock_ax.set_xlabel.assert_called_once_with(twd.model.get_column_header(col_as_x))
        col_y_name = twd.model.get_column_header(col_as_y)
        twd.plot.mock_ax.set_ylabel.assert_called_once_with(col_y_name)
        twd.plot.mock_ax.errorbar.assert_called_once_with(
            expected_x_data,
            expected_y_data,
            label=TableWorkspaceDisplay.COLUMN_DISPLAY_LABEL.format(col_y_name),
            yerr=expected_y_err_data,
            **extra_errorbar_assert_kwargs,
        )
        twd.plot.mock_fig.show.assert_called_once_with()
        twd.plot.mock_ax.legend.assert_called_once_with()

    @with_mock_presenter(add_selection_model=True, add_plot=True)
    def test_do_action_plot_multiple_y_linear_error_plot(self, ws, view, twd):
        """
        Test for _Linear_ plotting (with errors) multiple Y columns in selection,
        each of which has an associated Y error column
        """
        self.do_test_action_plot_multiple_y_error_plot(view, twd, PlotType.LINEAR_WITH_ERR)

    @with_mock_presenter(add_selection_model=True, add_plot=True)
    def test_do_action_plot_multiple_y_scatter_error_plot(self, ws, view, twd):
        """
        Test for _Scatter_ plotting (with errors) multiple Y columns in selection,
        each of which has an associated Y error column
        """
        self.do_test_action_plot_multiple_y_error_plot(view, twd, PlotType.SCATTER_WITH_ERR, {"fmt": "o"})

    @with_mock_presenter(add_selection_model=True, add_plot=True)
    def test_do_action_plot_multiple_y_linear_error_plot_append_yerrs(self, ws, view, twd):
        """
        Test for _Scatter_ plotting (with errors) multiple Y columns in selection,
        each of which has an associated Y error column

        This tests the case where the Y Error columns are part of the user's selection
        """
        self.do_test_action_plot_multiple_y_error_plot(view, twd, PlotType.LINEAR_WITH_ERR, append_yerr_to_selection=True)

    @with_mock_presenter(add_selection_model=True, add_plot=True)
    def test_do_action_plot_multiple_y_scatter_error_plot_append_yerrs(self, ws, view, twd):
        self.do_test_action_plot_multiple_y_error_plot(view, twd, PlotType.SCATTER_WITH_ERR, {"fmt": "o"}, append_yerr_to_selection=True)

    def do_test_action_plot_multiple_y_error_plot(
        self, view, twd, plot_type, extra_errorbar_assert_kwargs=None, append_yerr_to_selection=False
    ):
        """
        Does the test for plotting with multiple Y columns. Each of them has an associated error
        :param twd: The presenter
        :param view: The mock view
        :param plot_type: The type of the plot
        :param extra_errorbar_assert_kwargs: Extra arguments expanded in the assertion for the errorbar call
        :return:
        """
        if extra_errorbar_assert_kwargs is None:
            extra_errorbar_assert_kwargs = dict()
        col_as_x = 0
        col_as_y1 = 1
        col_as_y1_err = 2
        col_as_y2 = 3
        col_as_y2_err = 4

        expected_x_data = twd.model.get_column(col_as_x)
        expected_y1_data = twd.model.get_column(col_as_y1)
        expected_y1_err_data = twd.model.get_column(col_as_y1_err)
        expected_y2_data = twd.model.get_column(col_as_y2)
        expected_y2_err_data = twd.model.get_column(col_as_y2_err)

        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, col_as_x)]
        twd.action_set_as_x()

        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, col_as_y1_err)]
        twd.action_set_as_y_err(col_as_y1)
        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, col_as_y2_err)]
        twd.action_set_as_y_err(col_as_y2)

        view.mock_selection_model.selectedColumns.return_value = [MockQModelIndex(1, col_as_y1), MockQModelIndex(1, col_as_y2)]
        if append_yerr_to_selection:
            view.mock_selection_model.selectedColumns.return_value.extend(
                [MockQModelIndex(1, col_as_y1_err), MockQModelIndex(1, col_as_y2_err)]
            )
        twd.action_plot(plot_type)

        twd.plot.subplots.assert_called_once_with(subplot_kw={"projection": "mantid"})
        twd.plot.mock_fig.canvas.manager.set_window_title.assert_called_once_with(twd.model.get_name())
        twd.plot.mock_ax.set_xlabel.assert_called_once_with(twd.model.get_column_header(col_as_x))

        col_y1_name = twd.model.get_column_header(col_as_y1)
        col_y2_name = twd.model.get_column_header(col_as_y2)
        twd.plot.mock_ax.set_ylabel.assert_has_calls([call(col_y1_name), call(col_y2_name)])

        twd.plot.mock_ax.errorbar.assert_has_calls(
            [
                call(
                    expected_x_data,
                    expected_y1_data,
                    label=TableWorkspaceDisplay.COLUMN_DISPLAY_LABEL.format(col_y1_name),
                    yerr=expected_y1_err_data,
                    **extra_errorbar_assert_kwargs,
                ),
                call(
                    expected_x_data,
                    expected_y2_data,
                    label=TableWorkspaceDisplay.COLUMN_DISPLAY_LABEL.format(col_y2_name),
                    yerr=expected_y2_err_data,
                    **extra_errorbar_assert_kwargs,
                ),
            ]
        )
        twd.plot.mock_fig.show.assert_called_once_with()
        twd.plot.mock_ax.legend.assert_called_once_with()

    @with_mock_presenter()
    def test_close_incorrect_workspace(self, ws, view, presenter):
        presenter.close(ws.TEST_NAME + "123")
        self.assertNotCalled(presenter.container.emit_close)
        self.assertIsNotNone(presenter.ads_observer)

    @with_mock_presenter()
    def test_close(self, ws, view, presenter):
        presenter.close(ws.TEST_NAME)
        presenter.container.emit_close.assert_called_once_with()
        self.assertIsNone(presenter.ads_observer)

    @with_mock_presenter()
    def test_force_close_even_with_incorrect_name(self, _, view, presenter):
        # window always closes, regardless of the workspace
        presenter.force_close()
        presenter.container.emit_close.assert_called_once_with()
        self.assertIsNone(presenter.ads_observer)

    @with_mock_presenter()
    def test_force_close(self, _, view, presenter):
        presenter.force_close()
        presenter.container.emit_close.assert_called_once_with()
        self.assertIsNone(presenter.ads_observer)

    @with_mock_presenter()
    def test_replace_incorrect_workspace(self, ws, view, presenter):
        with patch("mantidqt.widgets.workspacedisplay.table.presenter.TableWorkspaceDataPresenterStandard.load_data") as mock_load_data:
            presenter.replace_workspace(ws.TEST_NAME + "123", ws)
            self.assertNotCalled(mock_load_data)
            self.assertNotCalled(view.emit_repaint)

    @with_mock_presenter()
    def test_replace(self, ws, view, presenter):
        # patch this out after the constructor of the presenter has finished,
        # so that we reset any calls it might have made
        presenter.model.block_model_replace = False
        with patch("mantidqt.widgets.workspacedisplay.table.presenter.TableWorkspaceDataPresenterStandard.load_data") as mock_load_data:
            presenter.replace_workspace(ws.TEST_NAME, ws)
            mock_load_data.assert_called_once_with(view)


if __name__ == "__main__":
    unittest.main()
