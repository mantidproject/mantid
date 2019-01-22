from __future__ import (absolute_import, division, print_function)

import unittest

from mock import Mock

from mantidqt.widgets.common.workspacedisplay_ads_observer import WorkspaceDisplayADSObserver


class MockWorkspaceDisplay:
    def __init__(self):
        self.close = Mock()
        self.replace_workspace = Mock()


class WorkspaceDisplayADSObserverTest(unittest.TestCase):
    def test_clearHandle(self):
        mock_wsd = MockWorkspaceDisplay()
        observer = WorkspaceDisplayADSObserver(mock_wsd)
        observer.clearHandle()
        mock_wsd.close.assert_called_once_with()

    def test_deleteHandle(self):
        mock_wsd = MockWorkspaceDisplay()
        observer = WorkspaceDisplayADSObserver(mock_wsd)
        observer.deleteHandle("a", None)
        mock_wsd.close.assert_called_once_with()

    def test_replaceHandle(self):
        mock_wsd = MockWorkspaceDisplay()
        observer = WorkspaceDisplayADSObserver(mock_wsd)

        expected_parameter = 444555.158
        observer.replaceHandle("a", expected_parameter)
        mock_wsd.replace_workspace.assert_called_once_with(expected_parameter)
