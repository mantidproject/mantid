# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import functools
import unittest
from unittest.mock import Mock

from mantidqt.utils.testing.mocks.mock_mantid import MockWorkspace
from mantidqt.widgets.workspacedisplay.table.table_model import TableModel
from mantidqt.widgets.workspacedisplay.table.model import TableWorkspaceDisplayModel


def with_mock_workspace(func):
    # type: (callable) -> callable
    @functools.wraps(func)
    def wrapper(self):
        ws = MockWorkspace()
        model = TableWorkspaceDisplayModel(ws)
        return func(self, model)

    return wrapper


class TableWorkspaceDisplayTableModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # Allow the MockWorkspace to work within the model
        TableWorkspaceDisplayModel.ALLOWED_WORKSPACE_TYPES.append(MockWorkspace)

    @with_mock_workspace
    def setUp(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        self.model = TableModel(data_model=model)
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


if __name__ == '__main__':
    unittest.main()
