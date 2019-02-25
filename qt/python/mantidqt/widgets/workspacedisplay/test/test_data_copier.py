# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from unittest import TestCase

from mantid.py3compat.mock import Mock, patch
from qtpy.QtWidgets import QMessageBox

from mantidqt.widgets.workspacedisplay.data_copier import DataCopier
from mantidqt.utils.testing.mocks.mock_qt import MockQClipboard, MockQModelIndex, \
    MockQSelectionModel, MockQStatusBar, MockQTableView
from mantidqt.widgets.workspacedisplay.user_notifier import UserNotifier
from mantidqt.widgets.workspacedisplay.table.view import TableWorkspaceDisplayView


class DataCopierTest(TestCase):
    show_mouse_toast_package = 'mantidqt.widgets.workspacedisplay.user_notifier.UserNotifier.show_mouse_toast'
    copy_to_clipboard_package = 'mantidqt.widgets.workspacedisplay.data_copier.DataCopier.copy_to_clipboard'

    def assertNotCalled(self, mock):
        self.assertEqual(0, mock.call_count)

    def setUp(self):
        self.mock_status_bar = MockQStatusBar()
        self.mock_clipboard = MockQClipboard()
        self.mock_clipboard.setText = Mock()
        self.data_copier = DataCopier(self.mock_status_bar)

        mock_selection_model = MockQSelectionModel(has_selection=True)
        mock_selection_model.selectedRows = Mock(
            return_value=[MockQModelIndex(1, 1), MockQModelIndex(2, 2), MockQModelIndex(3, 3)])
        mock_selection_model.selectedColumns = Mock(
            return_value=[MockQModelIndex(1, 1), MockQModelIndex(2, 2), MockQModelIndex(3, 3)])
        self.table = Mock(spec=TableWorkspaceDisplayView)
        self.table.mock_selection_model = mock_selection_model

    @patch(show_mouse_toast_package)
    @patch(copy_to_clipboard_package)
    def test_action_copy_spectrum_values(self, mock_copy, mock_show_mouse_toast):
        mock_table = MockQTableView()

        # two rows are selected in different positions
        mock_indexes = [MockQModelIndex(0, 1), MockQModelIndex(3, 1)]
        mock_table.mock_selection_model.selectedRows = Mock(return_value=mock_indexes)

        mock_read = Mock(return_value=[43, 99])
        expected_string = "43\t99\n43\t99"

        self.data_copier.copy_spectrum_values(mock_table, mock_read)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        mock_copy.assert_called_once_with(expected_string)
        mock_show_mouse_toast.assert_called_once_with(UserNotifier.COPY_SUCCESSFUL_MESSAGE)

    @patch(show_mouse_toast_package)
    @patch(copy_to_clipboard_package)
    def test_action_copy_spectrum_values_no_selection(self, mock_copy,
                                                      mock_show_mouse_toast):
        mock_table = MockQTableView()
        mock_table.mock_selection_model.hasSelection = Mock(return_value=False)
        mock_table.mock_selection_model.selectedRows = Mock()

        self.data_copier.copy_spectrum_values(mock_table, ws_read=None)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        # the action should never look for rows if there is no selection
        self.assertNotCalled(mock_table.mock_selection_model.selectedRows)
        self.assertNotCalled(mock_copy)
        mock_show_mouse_toast.assert_called_once_with(UserNotifier.NO_SELECTION_MESSAGE)

    @patch(show_mouse_toast_package)
    @patch(copy_to_clipboard_package)
    def test_action_copy_bin_values(self, mock_copy, mock_show_mouse_toast):
        mock_table = MockQTableView()

        # two columns are selected at different positions
        mock_indexes = [MockQModelIndex(0, 0), MockQModelIndex(0, 3)]
        mock_table.mock_selection_model.selectedColumns = Mock(return_value=mock_indexes)
        # change the mock ws to have 3 histograms
        num_hist = 3

        mock_read = Mock(return_value=[83, 11, 33, 70])
        expected_string = "83\t70\n83\t70\n83\t70"

        self.data_copier.copy_bin_values(mock_table, mock_read, num_hist)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        mock_copy.assert_called_once_with(expected_string)
        mock_show_mouse_toast.assert_called_once_with(UserNotifier.COPY_SUCCESSFUL_MESSAGE)

    @patch(show_mouse_toast_package)
    @patch(copy_to_clipboard_package)
    def test_action_copy_bin_values_no_selection(self, mock_copy, mock_show_mouse_toast):
        mock_table = MockQTableView()
        mock_table.mock_selection_model.hasSelection = Mock(return_value=False)
        mock_table.mock_selection_model.selectedColumns = Mock()

        self.data_copier.copy_bin_values(mock_table, None, None)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        # the action should never look for rows if there is no selection
        self.assertNotCalled(mock_table.mock_selection_model.selectedColumns)
        self.assertNotCalled(mock_copy)
        mock_show_mouse_toast.assert_called_once_with(UserNotifier.NO_SELECTION_MESSAGE)

    @patch(show_mouse_toast_package)
    @patch(copy_to_clipboard_package)
    def test_copy_cells_no_selection(self, mock_copy, mock_show_mouse_toast):
        mock_table = MockQTableView()
        mock_table.mock_selection_model.hasSelection = Mock(return_value=False)

        self.data_copier.copy_cells(mock_table)

        mock_table.selectionModel.assert_called_once_with()
        mock_table.mock_selection_model.hasSelection.assert_called_once_with()
        mock_show_mouse_toast.assert_called_once_with(UserNotifier.NO_SELECTION_MESSAGE)

        self.assertNotCalled(mock_copy)

    @patch(show_mouse_toast_package)
    @patch(copy_to_clipboard_package)
    def test_copy_cells(self, mock_copy, mock_show_mouse_toast):
        mock_table = MockQTableView()

        # two columns are selected at different positions
        mock_index = MockQModelIndex(None, None)
        mock_table.mock_selection_model.currentIndex = Mock(return_value=mock_index)

        self.data_copier.copy_cells(mock_table)

        mock_table.selectionModel.assert_called_once_with()
        self.assertEqual(1, mock_copy.call_count)
        self.assertEqual(9, mock_index.sibling.call_count)
        mock_show_mouse_toast.assert_called_once_with(UserNotifier.COPY_SUCCESSFUL_MESSAGE)

    @patch('qtpy.QtWidgets.QMessageBox.question', return_value=QMessageBox.Yes)
    def test_ask_confirmation(self, mock_question):
        message = "Hello"
        title = "Title"
        reply = self.data_copier.ask_confirmation(message, title)
        mock_question.assert_called_once_with(self.data_copier, title, message, QMessageBox.Yes, QMessageBox.No)
        self.assertEqual(reply, True)
