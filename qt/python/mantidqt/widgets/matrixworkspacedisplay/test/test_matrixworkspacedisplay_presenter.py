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

from mock import Mock

from mantidqt.widgets.matrixworkspacedisplay.presenter import MatrixWorkspaceDisplay
from mantidqt.widgets.matrixworkspacedisplay.test_helpers.matrixworkspacedisplay_common import MockWorkspace, \
    MockQModelIndex, MockQModelIndexSibling
from mantidqt.widgets.matrixworkspacedisplay.test_helpers.mock_matrixworkspacedisplay_view import \
    MockMatrixWorkspaceDisplayView, MockQTableView


class MatrixWorkspaceDisplayPresenterTest(unittest.TestCase):

    def test_setup_table(self):
        ws = MockWorkspace()
        view = MockMatrixWorkspaceDisplayView()
        presenter = MatrixWorkspaceDisplay(ws, view=view)
        self.assertEqual(3, view.set_context_menu_actions.call_count)
        self.assertEqual(1, view.set_model.call_count)

    def test_action_copy_spectrum_values(self):
        ws = MockWorkspace()
        view = MockMatrixWorkspaceDisplayView()
        presenter = MatrixWorkspaceDisplay(ws, view=view)

        mock_table = MockQTableView()

        # two rows are selected in different positions
        mock_indexes = [MockQModelIndex(0, 1), MockQModelIndex(3, 1)]
        mock_table.mock_selection_model.selectedRows = Mock(return_value=mock_indexes)
        mock_read = Mock(return_value=[43, 99])
        expected_string = "43 99\n43 99"

        presenter.action_copy_spectrum_values(mock_table, mock_read)

        view.copy_to_clipboard.assert_called_once_with(expected_string)

    def test_action_copy_spectrum_values_no_selection(self):
        ws = MockWorkspace()
        view = MockMatrixWorkspaceDisplayView()
        presenter = MatrixWorkspaceDisplay(ws, view=view)

        mock_table = MockQTableView()
        mock_table.mock_selection_model.hasSelection = Mock(return_value=False)
        mock_table.mock_selection_model.selectedRows = Mock()

        presenter.action_copy_spectrum_values(mock_table, None)

        # the action should never look for rows if there is no selection
        self.assertEqual(0, mock_table.mock_selection_model.selectedRows.call_count)

    def test_action_copy_bin_values(self):
        ws = MockWorkspace()
        view = MockMatrixWorkspaceDisplayView()
        presenter = MatrixWorkspaceDisplay(ws, view=view)
        mock_table = MockQTableView()

        # two columns are selected at different positions
        mock_indexes = [MockQModelIndex(0, 0), MockQModelIndex(0, 3)]
        mock_table.mock_selection_model.selectedColumns = Mock(return_value=mock_indexes)
        # change the mock ws to have 3 histograms
        ws.getNumberHistograms = Mock(return_value=3)

        mock_read = Mock(return_value=[83, 11, 33, 70])
        expected_string = "83 70\n83 70\n83 70"

        presenter.action_copy_bin_values(mock_table, mock_read)

        view.copy_to_clipboard.assert_called_once_with(expected_string)

    def test_action_copy_bin_values_no_selection(self):
        self.skipTest("Not Implemented")

    def test_action_copy_cell(self):
        ws = MockWorkspace()
        view = MockMatrixWorkspaceDisplayView()
        presenter = MatrixWorkspaceDisplay(ws, view=view)
        mock_table = MockQTableView()

        # two columns are selected at different positions
        mock_table.mock_selection_model.currentIndex = Mock(return_value=MockQModelIndex(0, 2))
        # change the mock ws to have 3 histograms
        ws.getNumberHistograms = Mock(return_value=3)
        presenter.action_copy_cell(mock_table)

        view.copy_to_clipboard.assert_called_once_with(MockQModelIndexSibling.TEST_SIBLING_DATA)


if __name__ == '__main__':
    unittest.main()
