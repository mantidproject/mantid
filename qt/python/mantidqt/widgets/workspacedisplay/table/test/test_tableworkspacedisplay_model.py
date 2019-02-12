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

import functools
import unittest

from mock import Mock

from mantid.kernel import V3D
from mantidqt.widgets.workspacedisplay.test_mocks.mock_mantid import MockWorkspace
from mantidqt.widgets.workspacedisplay.table.model import TableWorkspaceDisplayModel


def with_mock_model(func):
    # type: (callable) -> callable
    @functools.wraps(func)
    def wrapper(self):
        ws = MockWorkspace()
        model = TableWorkspaceDisplayModel(ws)
        return func(self, model)

    return wrapper


class TableWorkspaceDisplayModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Allow the MockWorkspace to work within the model
        TableWorkspaceDisplayModel.ALLOWED_WORKSPACE_TYPES.append(MockWorkspace)

    def test_get_name(self):
        ws = MockWorkspace()
        expected_name = "TEST_WORKSPACE"
        ws.name = Mock(return_value=expected_name)
        model = TableWorkspaceDisplayModel(ws)

        self.assertEqual(expected_name, model.get_name())

    def test_raises_with_unsupported_workspace(self):
        self.assertRaises(ValueError, lambda: TableWorkspaceDisplayModel([]))
        self.assertRaises(ValueError, lambda: TableWorkspaceDisplayModel(1))
        self.assertRaises(ValueError, lambda: TableWorkspaceDisplayModel("test_string"))

    @with_mock_model
    def test_get_v3d_from_str(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        self.assertEqual(V3D(1, 2, 3), model._get_v3d_from_str("1,2,3"))
        self.assertEqual(V3D(4, 5, 6), model._get_v3d_from_str("[4,5,6]"))

    @with_mock_model
    def test_set_cell_data_non_v3d(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        test_data = 4444

        expected_col = 1111
        expected_row = 1

        model.set_cell_data(expected_row, expected_col, test_data, False)

        # check that the correct conversion function was retrieved
        # -> the one for the column for which the data is being set
        model.ws.setCell.assert_called_once_with(expected_row, expected_col, test_data, notify_replace=False)

    @with_mock_model
    def test_set_cell_data_v3d(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        test_data = "[1,2,3]"

        expected_col = 1111
        expected_row = 1

        model.set_cell_data(expected_row, expected_col, test_data, True)

        # check that the correct conversion function was retrieved
        # -> the one for the column for which the data is being set
        model.ws.setCell.assert_called_once_with(expected_row, expected_col, V3D(1, 2, 3), notify_replace=False)

    def test_no_raise_with_supported_workspace(self):
        from mantid.simpleapi import CreateEmptyTableWorkspace
        ws = MockWorkspace()
        expected_name = "TEST_WORKSPACE"
        ws.name = Mock(return_value=expected_name)

        # no need to assert anything - if the constructor raises the test will fail
        TableWorkspaceDisplayModel(ws)

        ws = CreateEmptyTableWorkspace()
        TableWorkspaceDisplayModel(ws)


if __name__ == '__main__':
    unittest.main()
