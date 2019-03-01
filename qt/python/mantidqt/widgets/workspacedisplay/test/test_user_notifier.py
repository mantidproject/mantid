# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import sys
import unittest

from mantid.py3compat.mock import patch

from mantidqt.utils.testing.mocks.mock_qt import MockQStatusBar
from mantidqt.widgets.workspacedisplay.user_notifier import UserNotifier


def with_user_notifier(func):
    def decorated(self, *args, **kwargs):
        mock_status_bar = MockQStatusBar()
        user_notifier = UserNotifier(mock_status_bar)
        return func(self, user_notifier, *args, **kwargs)

    return decorated


class UserNotifierTest(unittest.TestCase):
    def assertNotCalled(self, mock):
        self.assertEqual(0, mock.call_count)

    def setUp(self):
        self.mock_status_bar = MockQStatusBar()
        self.user_notifier = UserNotifier(self.mock_status_bar)

    @patch('qtpy.QtWidgets.QToolTip.showText')
    def test_show_mouse_toast(self, mock_showText):
        message = "123123"
        mouse_position = 14
        with patch('mantidqt.widgets.workspacedisplay.user_notifier.UserNotifier._get_mouse_position',
                   return_value=mouse_position) as mock_get_mouse_position:
            self.user_notifier.show_mouse_toast(message)
            if sys.platform == "win32":
                mock_get_mouse_position.assert_called_once_with()
                mock_showText.assert_called_once_with(mouse_position, message)
            else:
                self.assertNotCalled(mock_showText)
                self.assertNotCalled(mock_get_mouse_position)

    @patch('qtpy.QtWidgets.QToolTip.showText')
    def test_no_mouse_toast_if_platform_not_windows(self, mock_showText):
        platform = "definitely_not_win32"
        with patch("sys.platform", return_value=platform):
            self.user_notifier.show_mouse_toast("")
            self.assertNotCalled(mock_showText)

    def test_show_status_message(self):
        message = "5555555"
        self.user_notifier.show_status_message(message)
        self.user_notifier._status_bar.showMessage.assert_called_once_with(message, self.user_notifier.DEFAULT_TIMEOUT)

    @patch('mantidqt.widgets.workspacedisplay.user_notifier.UserNotifier.show_mouse_toast')
    def test_notify_no_selection_to_copy(self, mock_show_mouse_toast):
        self.user_notifier.notify_no_selection_to_copy()
        mock_show_mouse_toast.assert_called_once_with(self.user_notifier.NO_SELECTION_MESSAGE)
        self.user_notifier._status_bar.showMessage.assert_called_once_with(self.user_notifier.NO_SELECTION_MESSAGE,
                                                                           self.user_notifier.DEFAULT_TIMEOUT)

    @patch('mantidqt.widgets.workspacedisplay.user_notifier.UserNotifier.show_mouse_toast')
    def test_notify_successful_copy(self, mock_show_mouse_toast):
        self.user_notifier.notify_successful_copy()
        mock_show_mouse_toast.assert_called_once_with(self.user_notifier.COPY_SUCCESSFUL_MESSAGE)
        self.user_notifier._status_bar.showMessage.assert_called_once_with(self.user_notifier.COPY_SUCCESSFUL_MESSAGE,
                                                                           self.user_notifier.DEFAULT_TIMEOUT)

    @patch('mantidqt.widgets.workspacedisplay.user_notifier.UserNotifier.show_mouse_toast')
    def test_notify_working(self, mock_show_mouse_toast):
        self.user_notifier.notify_working()
        self.assertNotCalled(mock_show_mouse_toast)
        self.user_notifier._status_bar.showMessage.assert_called_once_with(self.user_notifier.WORKING_MESSAGE,
                                                                           self.user_notifier.DEFAULT_TIMEOUT)
