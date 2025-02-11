# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import functools
import unittest
import sys

from mantid.dataobjects import PeaksWorkspace
from unittest.mock import Mock
from mantidqt.utils.testing.mocks.mock_mantid import MockWorkspaceGroup
from mantidqt.utils.testing.strict_mock import StrictMock
from mantidqt.widgets.workspacedisplay.table.group_model import (
    GroupTableWorkspaceDisplayModel,
)
from mantidqt.widgets.workspacedisplay.table.model import TableWorkspaceColumnTypeMapping


def with_mock_workspace(func):
    @functools.wraps(func)
    def wrapper(self):
        group_ws = MockWorkspaceGroup("group_ws")
        model = GroupTableWorkspaceDisplayModel(group_ws)
        return func(self, model)

    return wrapper


class GroupTableWorkspaceDisplayModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        GroupTableWorkspaceDisplayModel.ALLOWED_WORKSPACE_TYPES.append(MockWorkspaceGroup)

    def test_get_name(self):
        ws = MockWorkspaceGroup()
        expected_name = "TEST_WORKSPACE"
        ws.name = Mock(return_value=expected_name)
        model = GroupTableWorkspaceDisplayModel(ws)

        self.assertEqual(expected_name, model.get_name())

    def test_raises_with_unsupported_workspace(self):
        self.assertRaises(ValueError, lambda: GroupTableWorkspaceDisplayModel([]))
        self.assertRaises(ValueError, lambda: GroupTableWorkspaceDisplayModel(1))
        self.assertRaises(ValueError, lambda: GroupTableWorkspaceDisplayModel("test_string"))

    @with_mock_workspace
    def test_set_cell_data_hkl(self, model):
        peak_ws = Mock(spec=PeaksWorkspace)
        peak_ws.rowCount.return_value = 10
        peak_ws.columnCount.read_return = 3
        peak_ws.getColumnNames.return_value = ["h", "k", "l"]
        mock_peak = Mock()
        peak_ws.getPeak.return_value = mock_peak
        model.ws.append(peak_ws)

        row = (0, 0)
        HKL = [5, 10, 15]
        for col in range(0, len(HKL)):
            model.set_cell_data(row, col + 2, HKL[col], False)

        mock_peak.setH.assert_called_once_with(HKL[0])
        mock_peak.setK.assert_called_once_with(HKL[1])
        mock_peak.setL.assert_called_once_with(HKL[2])

    def test_no_raise_with_supported_workspace(self):
        from mantid.simpleapi import CreateEmptyTableWorkspace, GroupWorkspaces, mtd

        ws = MockWorkspaceGroup()
        expected_name = "TEST_WORKSPACE"
        ws.name = Mock(return_value=expected_name)

        # no need to assert anything - if the constructor raises the test will fail
        GroupTableWorkspaceDisplayModel(ws)

        ws = CreateEmptyTableWorkspace()
        GroupWorkspaces([ws], OutputWorkspace="group_ws")
        group_ws = mtd["group_ws"]

        GroupTableWorkspaceDisplayModel(group_ws)

    def test_initialise_marked_columns_one_of_each_type(self):
        num_of_repeated_columns = 10

        ws = MockWorkspaceGroup()
        peak_ws = Mock(spec=PeaksWorkspace)
        peak_ws.rowCount = StrictMock(return_value=10)
        peak_ws.columnCount = StrictMock(return_value=num_of_repeated_columns * 3)
        peak_ws.getColumnNames = StrictMock(return_value=["col"] * (num_of_repeated_columns * 3))
        peak_ws.__len__ = StrictMock(return_value=10)

        mock_column_types = [
            TableWorkspaceColumnTypeMapping.X,
            TableWorkspaceColumnTypeMapping.Y,
            TableWorkspaceColumnTypeMapping.YERR,
        ] * num_of_repeated_columns

        peak_ws.getPlotType = lambda i: mock_column_types[i]
        peak_ws.getLinkedYCol = lambda i: i - 1
        ws.append(peak_ws)
        model = GroupTableWorkspaceDisplayModel(ws)

        self.assertEqual(num_of_repeated_columns, len(model.marked_columns.as_x))
        self.assertEqual(num_of_repeated_columns, len(model.marked_columns.as_y))
        self.assertEqual(num_of_repeated_columns, len(model.marked_columns.as_y_err))

        for i, col in enumerate(range(4, 30, 3)):
            self.assertEqual(col - 1, model.marked_columns.as_y_err[i].related_y_column)

    def test_initialise_marked_columns_multiple_y_before_yerr(self):
        ws = MockWorkspaceGroup()

        mock_column_types = [
            TableWorkspaceColumnTypeMapping.X,
            TableWorkspaceColumnTypeMapping.Y,
            TableWorkspaceColumnTypeMapping.YERR,
            TableWorkspaceColumnTypeMapping.Y,
            TableWorkspaceColumnTypeMapping.Y,
            TableWorkspaceColumnTypeMapping.YERR,
            TableWorkspaceColumnTypeMapping.YERR,
            TableWorkspaceColumnTypeMapping.X,
        ]

        peak_ws = Mock(spec=PeaksWorkspace)
        peak_ws.rowCount = StrictMock(return_value=10)
        peak_ws.columnCount = StrictMock(return_value=len(mock_column_types))
        peak_ws.getColumnNames = StrictMock(return_value=["col"] * len(mock_column_types))
        peak_ws.__len__ = StrictMock(return_value=10)
        peak_ws.getPlotType = lambda i: mock_column_types[i]
        peak_ws.getLinkedYCol = StrictMock(side_effect=[1, 3, 4])
        ws.append(peak_ws)

        model = GroupTableWorkspaceDisplayModel(ws)

        self.assertEqual(2, len(model.marked_columns.as_x))
        self.assertEqual(3, len(model.marked_columns.as_y))
        self.assertEqual(3, len(model.marked_columns.as_y_err))

        self.assertEqual(3, model.marked_columns.as_y_err[0].related_y_column)
        self.assertEqual(5, model.marked_columns.as_y_err[1].related_y_column)
        self.assertEqual(6, model.marked_columns.as_y_err[2].related_y_column)

    def test_is_editable_column(self):
        group_ws = MockWorkspaceGroup()

        cols = ["h", "k", "l"]
        peak_ws = Mock(spec=PeaksWorkspace)
        peak_ws.rowCount = StrictMock(return_value=10)
        peak_ws.columnCount = StrictMock(return_value=len(cols))
        peak_ws.__len__ = StrictMock(return_value=10)
        peak_ws.getColumnNames = StrictMock(return_value=cols)
        group_ws.append(peak_ws)

        self.assertTrue(group_ws.size() == 1)

        model = GroupTableWorkspaceDisplayModel(group_ws)

        self.assertListEqual([False, False, True, True, True], [model.is_editable_column(i) for i in range(len(cols) + 2)])

    def test_get_cell(self):
        group_ws = MockWorkspaceGroup()

        cols = ["h", "k", "l"]
        peak_ws = Mock(spec=PeaksWorkspace)
        peak_ws.rowCount = StrictMock(return_value=10)
        peak_ws.columnCount = StrictMock(return_value=len(cols))
        peak_ws.__len__ = StrictMock(return_value=10)
        peak_ws.getColumnNames = StrictMock(return_value=cols)
        peak_ws.cell = Mock()
        group_ws.append(peak_ws)

        peak_ws2 = Mock(spec=PeaksWorkspace)
        peak_ws2.rowCount = StrictMock(return_value=10)
        peak_ws2.columnCount = StrictMock(return_value=len(cols))
        peak_ws2.__len__ = StrictMock(return_value=10)
        peak_ws2.getColumnNames = StrictMock(return_value=cols)
        peak_ws2.cell = Mock()
        group_ws.append(peak_ws2)

        model = GroupTableWorkspaceDisplayModel(group_ws)

        model.get_cell(0, 1)
        peak_ws.cell.assert_not_called()

        model.get_cell(0, 0)
        peak_ws.cell.assert_not_called()

        model.get_cell(0, 2)
        peak_ws.cell.assert_called_once_with(0, 0)

        model.get_cell(12, 6)
        peak_ws2.cell.assert_called_once_with(2, 4)

    def test_set_column_type(self):
        group_ws = MockWorkspaceGroup()

        cols = ["h", "k", "l"]
        peak_ws = Mock(spec=PeaksWorkspace)
        peak_ws.rowCount = StrictMock(return_value=10)
        peak_ws.columnCount = StrictMock(return_value=len(cols))
        peak_ws.__len__ = StrictMock(return_value=10)
        peak_ws.getColumnNames = StrictMock(return_value=cols)
        peak_ws.setPlotType = Mock()
        group_ws.append(peak_ws)

        model = GroupTableWorkspaceDisplayModel(group_ws)

        model.set_column_type(2, 0)
        peak_ws.setPlotType.assert_called_with(0, 0, -1)

        model.set_column_type(2, 1, 2)
        peak_ws.setPlotType.assert_called_with(0, 1, 0)

    def test_get_column(self):
        group_ws = MockWorkspaceGroup()

        cols = ["h", "k", "l"]
        peak_ws = Mock(spec=PeaksWorkspace)
        peak_ws.rowCount = StrictMock(return_value=10)
        peak_ws.columnCount = StrictMock(return_value=len(cols))
        peak_ws.__len__ = StrictMock(return_value=10)
        peak_ws.getColumnNames = StrictMock(return_value=cols)
        peak_ws.setPlotType = Mock()
        peak_ws.column = StrictMock(return_value=[])
        group_ws.append(peak_ws)

        model = GroupTableWorkspaceDisplayModel(group_ws)

        self.assertListEqual(model.get_column(0), [0] * peak_ws.rowCount())
        self.assertListEqual(model.get_column(1), [i for i in range(peak_ws.rowCount())])

        model.get_column(2)
        peak_ws.column.assert_called_once_with(0)

    def test_get_statistics(self):
        group_ws = MockWorkspaceGroup()

        cols = ["h", "k", "l"]
        peak_ws = Mock(spec=PeaksWorkspace)
        peak_ws.rowCount = StrictMock(return_value=10)
        peak_ws.columnCount = StrictMock(return_value=len(cols))
        peak_ws.__len__ = StrictMock(return_value=10)
        peak_ws.getColumnNames = StrictMock(return_value=cols)
        group_ws.append(peak_ws)

        model = GroupTableWorkspaceDisplayModel(group_ws)

        mock_simpleapi = Mock()
        mock_simpleapi.StatisticsOfTableWorkspace = Mock()
        sys.modules["mantid.simpleapi"] = mock_simpleapi

        model.get_statistics([0, 1, 2])
        mock_simpleapi.StatisticsOfTableWorkspace.assert_called_once_with(group_ws, [0, 1, 2])

        del sys.modules["mantid.simpleapi"]

    def test_sort(self):
        group_ws = MockWorkspaceGroup()

        cols = ["h", "k", "l"]
        peak_ws = Mock(spec=PeaksWorkspace)
        peak_ws.rowCount = StrictMock(return_value=10)
        peak_ws.columnCount = StrictMock(return_value=len(cols))
        peak_ws.__len__ = StrictMock(return_value=10)
        peak_ws.getColumnNames = StrictMock(return_value=cols)
        group_ws.append(peak_ws)

        model = GroupTableWorkspaceDisplayModel(group_ws)

        mock_simpleapi = Mock()
        mock_simpleapi.SortPeaksWorkspace = Mock()
        sys.modules["mantid.simpleapi"] = mock_simpleapi

        model.sort(2, True)
        mock_simpleapi.SortPeaksWorkspace.assert_called_once_with(
            InputWorkspace=group_ws, OutputWorkspace=group_ws, ColumnNameToSortBy="h", SortAscending=True
        )

        del sys.modules["mantid.simpleapi"]

    def test_delete_rows(self):
        group_ws = MockWorkspaceGroup()

        cols = ["h", "k", "l"]
        peak_ws = Mock(spec=PeaksWorkspace)
        peak_ws.rowCount = StrictMock(return_value=10)
        peak_ws.columnCount = StrictMock(return_value=len(cols))
        peak_ws.__len__ = StrictMock(return_value=10)
        peak_ws.getColumnNames = StrictMock(return_value=cols)
        group_ws.append(peak_ws)

        model = GroupTableWorkspaceDisplayModel(group_ws)

        mock_simpleapi = Mock()
        mock_simpleapi.DeleteTableRows = Mock()
        sys.modules["mantid.simpleapi"] = mock_simpleapi

        model.delete_rows([(0, 1), (0, 2), (0, 3), (0, 4)])
        mock_simpleapi.DeleteTableRows.assert_called_once_with(peak_ws, "1,2,3,4")

        del sys.modules["mantid.simpleapi"]


if __name__ == "__main__":
    unittest.main()
