"""
Unit tests for CheckMantidVersion._show_prompt.

NOTE: Update the two lines below to match wherever your class actually
lives. Everything else in this file works as-is once that's fixed:

    from workbench.utils.checkmantidversion import CheckMantidVersion
    MODULE = "workbench.utils.checkmantidversion"

`QMessageBox`, `ConfigService`, and `logger` are assumed to be imported
at the top of that module (so they're patched via MODULE.<name>).
`QDesktopServices` and `QUrl` are imported *inside* the method body, so
they must be patched at their true source (qtpy.QtGui / qtpy.QtCore)
rather than via MODULE — patching a name that's imported locally on
each call only works if you patch where it's looked up *from*.
"""

import unittest
from unittest.mock import MagicMock, patch

from mantidqt.utils.qt.testing import start_qapplication

from workbench.utils.checkmantidversion import CheckMantidVersion

MODULE = "workbench.widgets.update_notification.presenter"


@start_qapplication
class ShowPromptTest(unittest.TestCase):
    def setUp(self):
        self.checker = CheckMantidVersion()
        self.checker._parent = MagicMock()

    @staticmethod
    def _make_mock_box(clicked_is_update_btn):
        """Build a MagicMock standing in for the QMessageBox instance,
        wired up so clickedButton() returns whichever button we choose."""
        mock_box = MagicMock()
        update_btn = MagicMock(name="update_btn")
        later_btn = MagicMock(name="later_btn")

        # addButton() is called twice in _show_prompt: "Update now" first,
        # then "Remind me later" — side_effect returns them in that order.
        mock_box.addButton.side_effect = [update_btn, later_btn]
        mock_box.clickedButton.return_value = update_btn if clicked_is_update_btn else later_btn
        return mock_box, update_btn, later_btn

    # ------------------------------------------------------------------
    # "Update now" path
    # ------------------------------------------------------------------

    @patch(f"{MODULE}.QMessageBox")
    @patch(f"{MODULE}.ConfigService")
    @patch("qtpy.QtCore.QUrl")
    @patch("qtpy.QtGui.QDesktopServices")
    def test_update_now_opens_url_and_closes_parent(self, mock_qdesktop, mock_qurl_cls, mock_config, mock_msgbox_cls):
        mock_box, update_btn, _ = self._make_mock_box(clicked_is_update_btn=True)
        mock_msgbox_cls.return_value = mock_box
        mock_msgbox_cls.Information = "information-icon"
        mock_msgbox_cls.AcceptRole = "accept-role"
        mock_msgbox_cls.RejectRole = "reject-role"
        mock_config.getString.return_value = "http://download.mantidproject.org"

        self.checker._show_prompt("6.16.0")

        mock_config.getString.assert_called_once_with("CheckMantidVersion.DownloadURL")
        self.checker._parent.close.assert_called_once()
        mock_qurl_cls.assert_called_once_with("http://download.mantidproject.org")
        mock_qdesktop.openUrl.assert_called_once_with(mock_qurl_cls.return_value)

    # ------------------------------------------------------------------
    # "Remind me later" path
    # ------------------------------------------------------------------

    @patch(f"{MODULE}.QMessageBox")
    @patch(f"{MODULE}.ConfigService")
    @patch("qtpy.QtCore.QUrl")
    @patch("qtpy.QtGui.QDesktopServices")
    def test_remind_me_later_does_not_close_or_open_url(self, mock_qdesktop, mock_qurl_cls, mock_config, mock_msgbox_cls):
        mock_box, _, _ = self._make_mock_box(clicked_is_update_btn=False)
        mock_msgbox_cls.return_value = mock_box

        self.checker._show_prompt("6.16.0")

        mock_config.getString.assert_not_called()
        self.checker._parent.close.assert_not_called()
        mock_qurl_cls.assert_not_called()
        mock_qdesktop.openUrl.assert_not_called()

    # ------------------------------------------------------------------
    # Exception handling
    # ------------------------------------------------------------------

    @patch(f"{MODULE}.logger")
    @patch(f"{MODULE}.QMessageBox")
    @patch(f"{MODULE}.ConfigService")
    @patch("qtpy.QtCore.QUrl")
    @patch("qtpy.QtGui.QDesktopServices")
    def test_close_failure_is_logged_but_open_url_still_attempted(
        self, mock_qdesktop, mock_qurl_cls, mock_config, mock_msgbox_cls, mock_logger
    ):
        mock_box, _, _ = self._make_mock_box(clicked_is_update_btn=True)
        mock_msgbox_cls.return_value = mock_box
        mock_config.getString.return_value = "http://download.mantidproject.org"
        self.checker._parent.close.side_effect = RuntimeError("boom")

        # Should not raise, even though close() blew up.
        self.checker._show_prompt("6.16.0")

        mock_logger.error.assert_any_call("Failed to close Workbench: boom")
        mock_qdesktop.openUrl.assert_called_once()

    @patch(f"{MODULE}.logger")
    @patch(f"{MODULE}.QMessageBox")
    @patch(f"{MODULE}.ConfigService")
    @patch("qtpy.QtCore.QUrl")
    @patch("qtpy.QtGui.QDesktopServices")
    def test_open_url_failure_is_logged_and_does_not_raise(self, mock_qdesktop, mock_qurl_cls, mock_config, mock_msgbox_cls, mock_logger):
        mock_box, _, _ = self._make_mock_box(clicked_is_update_btn=True)
        mock_msgbox_cls.return_value = mock_box
        mock_config.getString.return_value = "http://download.mantidproject.org"
        mock_qdesktop.openUrl.side_effect = RuntimeError("no browser")

        try:
            self.checker._show_prompt("6.16.0")
        except Exception:
            self.fail("_show_prompt should not propagate exceptions from openUrl")

        self.checker._parent.close.assert_called_once()  # ran before openUrl, unaffected
        mock_logger.error.assert_any_call("Failed to open download URL: no browser")

    @patch(f"{MODULE}.logger")
    @patch(f"{MODULE}.QMessageBox")
    @patch(f"{MODULE}.ConfigService")
    @patch("qtpy.QtCore.QUrl")
    @patch("qtpy.QtGui.QDesktopServices")
    def test_config_service_failure_is_not_caught_here(self, mock_qdesktop, mock_qurl_cls, mock_config, mock_msgbox_cls, mock_logger):
        """ConfigService.getString() itself isn't wrapped in try/except in the
        current implementation, so a failure there should propagate. This
        test documents that behaviour — flip to assertRaises if that's not
        actually what you want, and wrap the getString() call accordingly."""
        mock_box, _, _ = self._make_mock_box(clicked_is_update_btn=True)
        mock_msgbox_cls.return_value = mock_box
        mock_config.getString.side_effect = RuntimeError("missing config key")

        with self.assertRaises(RuntimeError):
            self.checker._show_prompt("6.16.0")

    # ------------------------------------------------------------------
    # Dialog configuration
    # ------------------------------------------------------------------

    @patch(f"{MODULE}.QMessageBox")
    @patch(f"{MODULE}.ConfigService")
    @patch("qtpy.QtCore.QUrl")
    @patch("qtpy.QtGui.QDesktopServices")
    def test_message_box_is_configured_correctly(self, mock_qdesktop, mock_qurl_cls, mock_config, mock_msgbox_cls):
        mock_box, _, _ = self._make_mock_box(clicked_is_update_btn=False)
        mock_msgbox_cls.return_value = mock_box
        mock_msgbox_cls.Information = "information-icon"

        self.checker._show_prompt("6.16.0")

        mock_msgbox_cls.assert_called_once_with(self.checker._parent)
        mock_box.setIcon.assert_called_once_with(mock_msgbox_cls.Information)
        mock_box.setWindowTitle.assert_called_once_with("Update available")
        mock_box.setText.assert_called_once_with("Mantid Workbench 6.16.0 is available.")
        self.assertEqual(mock_box.addButton.call_count, 2)
        mock_box.exec.assert_called_once()


if __name__ == "__main__":
    unittest.main()
