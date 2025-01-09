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
from unittest.mock import Mock

from mantid.dataobjects import PeaksWorkspace

from mantidqt.utils.testing.mocks.mock_mantid import MockWorkspaceGroup
from mantidqt.widgets.workspacedisplay.table.group_table_model import GroupTableModel
from mantidqt.widgets.workspacedisplay.table.group_model import GroupTableWorkspaceDisplayModel


def with_mock_workspace(func):
    @functools.wraps(func)
    def wrapper(self):
        group_ws = MockWorkspaceGroup("group_ws")
        model = GroupTableWorkspaceDisplayModel(group_ws)
        return func(self, model)

    return wrapper


class TableWorkspaceDisplayTableModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        GroupTableWorkspaceDisplayModel.ALLOWED_WORKSPACE_TYPES.append(MockWorkspaceGroup)

    @with_mock_workspace
    def setUp(self, model):
        self.model = GroupTableModel(data_model=model)
        self.model.beginResetModel = Mock()
        self.model.endResetModel = Mock()

    @with_mock_workspace
    def test_load_data(self, model):
        model.get_column_headers = Mock(return_value=["header1", "header2"])
        self.model._update_row_batch_size = Mock()
        self.model.load_data(model)
        self.assertEqual(["header1", "header2"], self.model._headers)
        self.assertEqual(0, self.model._row_count)
        self.model._update_row_batch_size.assert_called_once_with()

    def test_data(self):
        group_ws = MockWorkspaceGroup()

        cols = ["h", "k", "l"]
        peak_ws = Mock(spec=PeaksWorkspace)
        peak_ws.rowCount = Mock(return_value=10)
        peak_ws.columnCount = Mock(return_value=len(cols))
        peak_ws.__len__ = Mock(return_value=10)
        peak_ws.getColumnNames = Mock(return_value=cols)
        group_ws.append(peak_ws)

        data_model = GroupTableWorkspaceDisplayModel(group_ws)
        data_model.get_number_of_rows = Mock(return_value=10)
        data_model.get_cell = Mock(return_value=0)
        table_model = GroupTableModel(data_model=data_model)

        index = Mock()
        index.isValid = Mock(return_value=False)
        self.assertEqual(table_model.data(index), None)

        index.isValid = Mock(return_value=True)
        index.row = Mock(return_value=10)
        index.max_rows = Mock(return_value=5)
        self.assertEqual(table_model.data(index), None)

        index.isValid = Mock(return_value=True)
        index.row = Mock(return_value=0)
        index.column = Mock(return_value=10)
        table_model.data(index)
        data_model.get_cell.assert_called_once_with(0, 10)

    def test_setData(self):
        group_ws = MockWorkspaceGroup()

        cols = ["h", "k", "l"]
        peak_ws = Mock(spec=PeaksWorkspace)
        peak_ws.rowCount = Mock(return_value=10)
        peak_ws.columnCount = Mock(return_value=len(cols))
        peak_ws.__len__ = Mock(return_value=10)
        peak_ws.getColumnNames = Mock(return_value=cols)
        group_ws.append(peak_ws)

        data_model = GroupTableWorkspaceDisplayModel(group_ws)
        data_model.get_number_of_rows = Mock(return_value=10)
        data_model.set_cell = Mock(return_value=0)
        table_model = GroupTableModel(data_model=data_model)

        index = Mock()
        index.model = Mock(return_value=table_model)
        table_model.index = Mock(return_value=0)
        table_model.data = Mock(return_value=0)

        self.assertEqual(table_model.setData(index, 1, None), False)


if __name__ == "__main__":
    unittest.main()
