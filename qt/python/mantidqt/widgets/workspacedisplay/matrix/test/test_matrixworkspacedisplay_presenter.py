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

from qtpy.QtWidgets import QStatusBar

from mantid.py3compat.mock import Mock, patch
from mantidqt.utils.testing.mocks.mock_mantid import MockWorkspace
from mantidqt.utils.testing.mocks.mock_matrixworkspacedisplay import MockMatrixWorkspaceDisplayView
from mantidqt.utils.testing.mocks.mock_qt import MockQModelIndex, MockQTableView
from mantidqt.widgets.workspacedisplay.matrix.model import MatrixWorkspaceDisplayModel
from mantidqt.widgets.workspacedisplay.matrix.presenter import MatrixWorkspaceDisplay
from mantidqt.widgets.workspacedisplay.matrix.table_view_model import MatrixWorkspaceTableViewModelType
from mantidqt.widgets.workspacedisplay.status_bar_view import StatusBarView


def with_mock_presenter(func):
    def wrapper(self, *args):
        ws = MockWorkspace()
        view = MockMatrixWorkspaceDisplayView()
        mock_observer = Mock()
        container = Mock(spec=StatusBarView)
        container.status_bar = Mock(spec=QStatusBar)
        presenter = MatrixWorkspaceDisplay(ws, view=view, ads_observer=mock_observer, container=container)
        return func(self, ws, view, presenter, *args)

    return wrapper


class MatrixWorkspaceDisplayPresenterTest(unittest.TestCase):
    show_mouse_toast_package = 'mantidqt.widgets.workspacedisplay.user_notifier.UserNotifier.show_mouse_toast'
    copy_to_clipboard_package = 'mantidqt.widgets.workspacedisplay.data_copier.DataCopier.copy_to_clipboard'

    @classmethod
    def setUpClass(cls):
        # Allow the MockWorkspace to work within the model
        MatrixWorkspaceDisplayModel.ALLOWED_WORKSPACE_TYPES.append(MockWorkspace)

    def assertNotCalled(self, mock):
        self.assertEqual(0, mock.call_count)

    def test_setup_table(self):
        ws = MockWorkspace()
        view = MockMatrixWorkspaceDisplayView()
        container = Mock(spec=StatusBarView)
        container.status_bar = Mock(spec=QStatusBar)
        MatrixWorkspaceDisplay(ws, view=view, container=container)
        self.assertEqual(3, view.set_context_menu_actions.call_count)
        self.assertEqual(1, view.set_model.call_count)

    @patch(show_mouse_toast_package)
    @patch(copy_to_clipboard_package)
    @with_mock_presenter
    def test_action_copy_spectrum_values(self, ws, view, presenter, mock_copy, mock_show_mouse_toast):
        mock_table = MockQTableView()

        # two rows are selected in different positions
        mock_indexes = [MockQModelIndex(0, 1), MockQModelIndex(3, 1)]
        mock_table.mock_selection_model.selectedRows = Mock(return_value=mock_indexes)

        mock_read = Mock(return_value=[43, 99])
        presenter._get_ws_read_from_type = Mock(return_value=mock_read)
        expected_string = "43\t99\n43\t99"

        presenter.action_copy_spectrum_values(mock_table)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        mock_copy.assert_called_once_with(expected_string)
        mock_show_mouse_toast.assert_called_once_with(MatrixWorkspaceDisplay.COPY_SUCCESSFUL_MESSAGE)

        presenter._get_ws_read_from_type.assert_called_once_with(MatrixWorkspaceTableViewModelType.x)

    @patch(show_mouse_toast_package)
    @patch(copy_to_clipboard_package)
    @with_mock_presenter
    def test_action_copy_spectrum_values_no_selection(self, ws, view, presenter, mock_copy,
                                                      mock_show_mouse_toast):

        mock_table = MockQTableView()
        mock_table.mock_selection_model.hasSelection = Mock(return_value=False)
        mock_table.mock_selection_model.selectedRows = Mock()

        presenter.action_copy_spectrum_values(mock_table)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        # the action should never look for rows if there is no selection
        self.assertNotCalled(mock_table.mock_selection_model.selectedRows)
        self.assertNotCalled(mock_copy)
        mock_show_mouse_toast.assert_called_once_with(MatrixWorkspaceDisplay.NO_SELECTION_MESSAGE)

    @patch(show_mouse_toast_package)
    @patch(copy_to_clipboard_package)
    @with_mock_presenter
    def test_action_copy_bin_values(self, ws, view, presenter, mock_copy, mock_show_mouse_toast):
        mock_table = MockQTableView()

        # two columns are selected at different positions
        mock_indexes = [MockQModelIndex(0, 0), MockQModelIndex(0, 3)]
        mock_table.mock_selection_model.selectedColumns = Mock(return_value=mock_indexes)
        # change the mock ws to have 3 histograms
        ws.getNumberHistograms = Mock(return_value=3)

        mock_read = Mock(return_value=[83, 11, 33, 70])
        presenter._get_ws_read_from_type = Mock(return_value=mock_read)
        expected_string = "83\t70\n83\t70\n83\t70"

        presenter.action_copy_bin_values(mock_table)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        mock_copy.assert_called_once_with(expected_string)
        mock_show_mouse_toast.assert_called_once_with(MatrixWorkspaceDisplay.COPY_SUCCESSFUL_MESSAGE)
        presenter._get_ws_read_from_type.assert_called_once_with(MatrixWorkspaceTableViewModelType.x)

    @patch(show_mouse_toast_package)
    @patch(copy_to_clipboard_package)
    @with_mock_presenter
    def test_action_copy_bin_values_no_selection(self, ws, view, presenter, mock_copy, mock_show_mouse_toast):
        mock_table = MockQTableView()
        mock_table.mock_selection_model.hasSelection = Mock(return_value=False)
        mock_table.mock_selection_model.selectedColumns = Mock()

        presenter.action_copy_bin_values(mock_table)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        # the action should never look for rows if there is no selection
        self.assertNotCalled(mock_table.mock_selection_model.selectedColumns)
        self.assertNotCalled(mock_copy)
        mock_show_mouse_toast.assert_called_once_with(MatrixWorkspaceDisplay.NO_SELECTION_MESSAGE)

    @patch(show_mouse_toast_package)
    @patch(copy_to_clipboard_package)
    @with_mock_presenter
    def test_action_copy_cell(self, ws, view, presenter, mock_copy, mock_show_mouse_toast):
        mock_table = MockQTableView()

        # two columns are selected at different positions
        mock_index = MockQModelIndex(None, None)
        mock_table.mock_selection_model.currentIndex = Mock(return_value=mock_index)

        presenter.action_copy_cells(mock_table)

        mock_table.selectionModel.assert_called_once_with()
        self.assertEqual(1, mock_copy.call_count)
        self.assertEqual(9, mock_index.sibling.call_count)
        mock_show_mouse_toast.assert_called_once_with(MatrixWorkspaceDisplay.COPY_SUCCESSFUL_MESSAGE)

    @patch(show_mouse_toast_package)
    @patch(copy_to_clipboard_package)
    @with_mock_presenter
    def test_action_copy_cell_no_selection(self, ws, view, presenter, mock_copy, mock_show_mouse_toast):
        mock_table = MockQTableView()
        mock_table.mock_selection_model.hasSelection = Mock(return_value=False)

        presenter.action_copy_cells(mock_table)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        mock_show_mouse_toast.assert_called_once_with(MatrixWorkspaceDisplay.NO_SELECTION_MESSAGE)

        self.assertNotCalled(mock_copy)

    def common_setup_action_plot(self, table_has_selection=True):
        mock_ws = MockWorkspace()
        mock_view = MockMatrixWorkspaceDisplayView()
        mock_plotter = Mock()
        container = Mock(spec=StatusBarView)
        container.status_bar = Mock(spec=QStatusBar)
        presenter = MatrixWorkspaceDisplay(mock_ws, plot=mock_plotter, view=mock_view, container=container)

        # monkey-patch the spectrum plot label to count the number of calls
        presenter.model.get_spectrum_plot_label = Mock()
        presenter.model.get_bin_plot_label = Mock()

        mock_table = MockQTableView()
        # configure the mock return values
        mock_table.mock_selection_model.hasSelection = Mock(return_value=table_has_selection)
        return mock_plotter, mock_table, mock_view, presenter

    def setup_mock_selection(self, mock_table, num_selected_rows=None, num_selected_cols=None):
        """
        :type mock_table: MockQTableView
        :type num_selected_rows: int|None
        :type num_selected_cols: int|None
        """
        mock_selected = []
        if num_selected_rows is not None:
            for i in range(num_selected_rows):
                mock_selected.append(MockQModelIndex(i, 1))
            mock_table.mock_selection_model.selectedRows = Mock(return_value=mock_selected)
            mock_table.mock_selection_model.selectedColumns = Mock()
        elif num_selected_cols is not None:
            for i in range(num_selected_cols):
                mock_selected.append(MockQModelIndex(1, i))
            mock_table.mock_selection_model.selectedRows = Mock()
            mock_table.mock_selection_model.selectedColumns = Mock(return_value=mock_selected)
        else:
            mock_table.mock_selection_model.selectedRows = Mock()
            mock_table.mock_selection_model.selectedColumns = Mock()
        return mock_selected

    def test_action_plot_spectrum_plot_many_confirmed(self):
        mock_plot, mock_table, mock_view, presenter = self.common_setup_action_plot()
        num_selected_rows = MatrixWorkspaceDisplay.NUM_SELECTED_FOR_CONFIRMATION + 1

        self.setup_mock_selection(mock_table, num_selected_rows)

        # The a lot of things to plot message will show, set that the user will CONFIRM the plot
        # meaning the rest of the function will execute as normal
        mock_view.ask_confirmation = Mock(return_value=True)

        presenter.action_plot_spectrum(mock_table)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        mock_table.mock_selection_model.selectedRows.assert_called_once_with()

        mock_view.ask_confirmation.assert_called_once_with(
            MatrixWorkspaceDisplay.A_LOT_OF_THINGS_TO_PLOT_MESSAGE.format(num_selected_rows))

        self.assertNotCalled(mock_table.mock_selection_model.selectedColumns)
        self.assertEqual(1, mock_plot.call_count)

    def test_action_plot_spectrum_plot_many_denied(self):
        mock_plot, mock_table, mock_view, presenter = self.common_setup_action_plot()
        num_selected_rows = MatrixWorkspaceDisplay.NUM_SELECTED_FOR_CONFIRMATION + 1

        # return value unused as most of the function being tested is not executed
        self.setup_mock_selection(mock_table, num_selected_rows)

        # The a lot of things to plot message will show, set that the user will DENY the plot
        # meaning the rest of the function will NOT EXECUTE AT ALL
        mock_view.ask_confirmation = Mock(return_value=False)

        presenter.action_plot_spectrum(mock_table)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        mock_table.mock_selection_model.selectedRows.assert_called_once_with()

        mock_view.ask_confirmation.assert_called_once_with(
            MatrixWorkspaceDisplay.A_LOT_OF_THINGS_TO_PLOT_MESSAGE.format(num_selected_rows))

        self.assertNotCalled(mock_table.mock_selection_model.selectedColumns)
        self.assertNotCalled(mock_plot)

    @patch(show_mouse_toast_package)
    def test_action_plot_spectrum_no_selection(self, mock_show_mouse_toast):
        mock_plot, mock_table, mock_view, presenter = self.common_setup_action_plot(table_has_selection=False)

        mock_table.mock_selection_model.selectedRows = Mock()
        mock_table.mock_selection_model.selectedColumns = Mock()

        presenter.action_plot_spectrum(mock_table)

        mock_show_mouse_toast.assert_called_once_with(MatrixWorkspaceDisplay.NO_SELECTION_MESSAGE)
        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()

        self.assertNotCalled(mock_table.mock_selection_model.selectedRows)
        self.assertNotCalled(mock_table.mock_selection_model.selectedColumns)
        self.assertNotCalled(mock_plot)

    def test_action_plot_bin_plot_many_confirmed(self):
        mock_plot, mock_table, mock_view, presenter = self.common_setup_action_plot()
        num_selected_cols = MatrixWorkspaceDisplay.NUM_SELECTED_FOR_CONFIRMATION + 1
        self.setup_mock_selection(mock_table, num_selected_cols=num_selected_cols)
        mock_view.ask_confirmation = Mock(return_value=True)

        presenter.action_plot_bin(mock_table)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        mock_table.mock_selection_model.selectedColumns.assert_called_once_with()
        self.assertNotCalled(mock_table.mock_selection_model.selectedRows)

        mock_view.ask_confirmation.assert_called_once_with(
            MatrixWorkspaceDisplay.A_LOT_OF_THINGS_TO_PLOT_MESSAGE.format(num_selected_cols))
        self.assertEqual(1, mock_plot.call_count)

    def test_action_plot_bin_plot_many_denied(self):
        mock_plot, mock_table, mock_view, presenter = self.common_setup_action_plot()
        num_selected_cols = MatrixWorkspaceDisplay.NUM_SELECTED_FOR_CONFIRMATION + 1

        # return value unused as most of the function being tested is not executed
        self.setup_mock_selection(mock_table, num_selected_cols=num_selected_cols)

        mock_view.ask_confirmation = Mock(return_value=False)

        presenter.action_plot_bin(mock_table)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        mock_table.mock_selection_model.selectedColumns.assert_called_once_with()

        mock_view.ask_confirmation.assert_called_once_with(
            MatrixWorkspaceDisplay.A_LOT_OF_THINGS_TO_PLOT_MESSAGE.format(num_selected_cols))

        self.assertNotCalled(mock_table.mock_selection_model.selectedRows)
        self.assertNotCalled(mock_plot)

    @patch(show_mouse_toast_package)
    def test_action_plot_bin_no_selection(self, mock_show_mouse_toast):
        mock_plot, mock_table, mock_view, presenter = self.common_setup_action_plot(table_has_selection=False)
        self.setup_mock_selection(mock_table, num_selected_rows=None, num_selected_cols=None)

        presenter.action_plot_bin(mock_table)

        mock_show_mouse_toast.assert_called_once_with(MatrixWorkspaceDisplay.NO_SELECTION_MESSAGE)
        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()

        self.assertNotCalled(mock_table.mock_selection_model.selectedRows)
        self.assertNotCalled(mock_table.mock_selection_model.selectedColumns)
        self.assertNotCalled(mock_plot)

    @with_mock_presenter
    def test_close_incorrect_workspace(self, ws, view, presenter):
        presenter.close(ws.TEST_NAME + "123")
        self.assertNotCalled(presenter.container.emit_close)
        self.assertIsNotNone(presenter.ads_observer)

    @with_mock_presenter
    def test_close(self, ws, view, presenter):
        presenter.close(ws.TEST_NAME)
        presenter.container.emit_close.assert_called_once_with()
        self.assertIsNone(presenter.ads_observer)

    @with_mock_presenter
    def test_force_close_even_with_incorrect_name(self, _, view, presenter):
        # window always closes, regardless of the workspace
        presenter.force_close()
        presenter.container.emit_close.assert_called_once_with()
        self.assertIsNone(presenter.ads_observer)

    @with_mock_presenter
    def test_force_close(self, _, view, presenter):
        presenter.force_close()
        presenter.container.emit_close.assert_called_once_with()
        self.assertIsNone(presenter.ads_observer)

    @with_mock_presenter
    def test_replace_incorrect_workspace(self, ws, view, presenter):
        presenter.replace_workspace(ws.TEST_NAME + "123", ws)
        self.assertNotCalled(view.get_active_tab)
        self.assertNotCalled(view.mock_tab.viewport)
        self.assertNotCalled(view.mock_tab.mock_viewport.update)

    @with_mock_presenter
    def test_replace(self, ws, view, presenter):
        view.set_model.reset_mock()

        presenter.replace_workspace(ws.TEST_NAME, ws)

        self.assertEqual(3, view.set_context_menu_actions.call_count)
        self.assertEqual(1, view.set_model.call_count)


if __name__ == '__main__':
    unittest.main()
