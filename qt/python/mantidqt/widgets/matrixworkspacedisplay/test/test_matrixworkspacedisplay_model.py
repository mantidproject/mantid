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

from mantidqt.widgets.matrixworkspacedisplay.model import MatrixWorkspaceDisplayModel
from mantidqt.widgets.matrixworkspacedisplay.table_view_model import MatrixWorkspaceTableViewModelType
from mantidqt.widgets.matrixworkspacedisplay.test_helpers.matrixworkspacedisplay_common import \
    MockWorkspace


class MatrixWorkspaceDisplayModelTest(unittest.TestCase):

    def test_get_name(self):
        ws = MockWorkspace()
        expected_name = "TEST_WORKSPACE"
        ws.name = Mock(return_value=expected_name)
        model = MatrixWorkspaceDisplayModel(ws)

        self.assertEqual(expected_name, model.get_name())

    def test_get_item_model(self):
        ws = MockWorkspace()
        expected_name = "TEST_WORKSPACE"
        ws.name = Mock(return_value=expected_name)
        model = MatrixWorkspaceDisplayModel(ws)

        x_model, y_model, e_model = model.get_item_model()

        self.assertEqual(x_model.type, MatrixWorkspaceTableViewModelType.x)
        self.assertEqual(y_model.type, MatrixWorkspaceTableViewModelType.y)
        self.assertEqual(e_model.type, MatrixWorkspaceTableViewModelType.e)


if __name__ == '__main__':
    unittest.main()
