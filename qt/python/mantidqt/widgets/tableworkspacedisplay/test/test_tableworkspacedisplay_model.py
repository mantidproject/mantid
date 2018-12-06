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

from mantidqt.widgets.matrixworkspacedisplay.test_helpers.matrixworkspacedisplay_common import \
    MockWorkspace
from mantidqt.widgets.tableworkspacedisplay.model import TableWorkspaceDisplayModel


# from mantid.simpleapi import CreateSampleWorkspace


class TableWorkspaceDisplayModelTest(unittest.TestCase):

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
 
    # def test_no_raise_with_supported_workspace(self):
    #     ws = MockWorkspace()
    #     expected_name = "TEST_WORKSPACE"
    #     ws.name = Mock(return_value=expected_name)
    #
    #     # no need to assert anything - if the constructor raises the test will fail
    #     TableWorkspaceDisplayModel(ws)
    #
    #     ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10)
    #     TableWorkspaceDisplayModel(ws)


if __name__ == '__main__':
    unittest.main()
