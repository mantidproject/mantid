# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat.mock import Mock
from mantidqt.widgets.observers.ads_observer import WorkspaceDisplayADSObserver


class MockWorkspaceDisplay:
    def __init__(self):
        self.close = Mock()
        self.force_close = Mock()
        self.replace_workspace = Mock()


class WorkspaceDisplayADSObserverTest(unittest.TestCase):
    def test_clearHandle(self):
        mock_wsd = MockWorkspaceDisplay()
        observer = WorkspaceDisplayADSObserver(mock_wsd)
        observer.clearHandle()
        mock_wsd.force_close.assert_called_once_with()

    def test_deleteHandle(self):
        mock_wsd = MockWorkspaceDisplay()
        observer = WorkspaceDisplayADSObserver(mock_wsd)
        expected_name = "adad"
        observer.deleteHandle(expected_name, None)
        mock_wsd.close.assert_called_once_with(expected_name)

    def test_replaceHandle(self):
        mock_wsd = MockWorkspaceDisplay()
        observer = WorkspaceDisplayADSObserver(mock_wsd)

        expected_name = "a"
        expected_parameter = 444555.158
        observer.replaceHandle(expected_name, expected_parameter)
        mock_wsd.replace_workspace.assert_called_once_with(expected_name, expected_parameter)
