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
from mantidqt.widgets.matrixworkspacedisplay.test_helpers.matrixworkspacedisplay_common import \
    MockWorkspace
from mantidqt.widgets.tableworkspacedisplay.model import TableWorkspaceDisplayModel


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
    def test_get_bool_from_str(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        self.assertEqual(model._get_bool_from_str(True), True)
        self.assertEqual(model._get_bool_from_str(False), False)

        self.assertEqual(model._get_bool_from_str("True"), True)
        self.assertEqual(model._get_bool_from_str("true"), True)

        self.assertEqual(model._get_bool_from_str("False"), False)
        self.assertEqual(model._get_bool_from_str("false"), False)

    @with_mock_model
    def test_get_bool_from_str_raises_invalid_str(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        self.assertRaisesRegexp(ValueError, "not a valid bool string", model._get_bool_from_str, "fadawd")
        self.assertRaisesRegexp(ValueError, "cannot be converted to bool", model._get_bool_from_str, [12])

    @with_mock_model
    def test_get_v3d_from_str(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        self.assertEqual(V3D(1, 2, 3), model._get_v3d_from_str("1,2,3"))
        self.assertEqual(V3D(4, 5, 6), model._get_v3d_from_str("[4,5,6]"))

    @with_mock_model
    def test_map_from_type_name(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        conversion_funcs = model.map_from_type_names(
            ['class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >', 'double',
             'double', 'int', '__int64', 'float', 'unsigned __int64', 'struct Mantid::API::Boolean',
             'class Mantid::Kernel::V3D', 'unsigned int'])

        self.assertEqual(
            [str, float, float, int, int, float, int, model._get_bool_from_str, model._get_v3d_from_str, int],
            conversion_funcs)

    @with_mock_model
    def test_map_from_type_name_raises_for_unknown_type(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        self.assertRaisesRegexp(ValueError, "Trying to set data for unknown column type", model.map_from_type_names,
                                ["grizzly", "bears"])

    @with_mock_model
    def test_set_cell_data(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        test_data = 4444

        expected_col = 1111
        model.conversion_functions = Mock()
        model.conversion_functions.__getitem__ = Mock(return_value=lambda x: int(x))

        model.set_cell_data(1, expected_col, test_data)

        # check that the correct conversion function was retrieved
        # -> the one for the column for which the data is being set
        model.conversion_functions.__getitem__.assert_called_once_with(expected_col)
        model.ws.setCell.assert_called_once_with(1, expected_col, test_data)

    class TableWorkspaceDisplayModelFrameworkTest(unittest.TestCase):
        @classmethod
        def setUpClass(cls):
            # Allow the MockWorkspace to work within the model
            TableWorkspaceDisplayModel.ALLOWED_WORKSPACE_TYPES.append(MockWorkspace)

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
