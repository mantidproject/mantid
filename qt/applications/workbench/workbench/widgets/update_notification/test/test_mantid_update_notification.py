import unittest
from unittest.mock import MagicMock, patch

from mantidqt.utils.qt.testing import start_qapplication
from workbench.widgets.update_notification.presenter import UpdateNotificationPresenter

MODULE = "workbench.widgets.update_notification.presenter"


@start_qapplication
class ShowPromptTest(unittest.TestCase):
    def setUp(self):
        self.main_window = MagicMock()
        self.update_notif_presenter = UpdateNotificationPresenter(self.main_window)

    @staticmethod
    def _make_mock_box(clicked_button_label):
        """Build a MagicMock standing in for the QMessageBox instance,
        wired up so clickedButton() returns whichever button choosen."""
        mock_box = MagicMock()
        buttons_by_label = {}

        def _add_button(label, role):
            btn = buttons_by_label.setdefault(label, MagicMock(name=f"{label}_btn"))
            return btn

        mock_box.addButton.side_effect = _add_button
        mock_box.clickedButton.side_effect = lambda: buttons_by_label[clicked_button_label]
        return mock_box, buttons_by_label

    # ------------------------------------------------------------------
    # "Update now" path
    # ------------------------------------------------------------------
    @patch(f"{MODULE}.QMessageBox")
    @patch(f"{MODULE}.ConfigService")
    @patch("qtpy.QtCore.QUrl")
    @patch("qtpy.QtGui.QDesktopServices")
    def test_update_now_opens_url_and_closes_parent(self, mock_qdesktop, mock_qurl_cls, mock_config, mock_msgbox_cls):
        mock_box, buttons_by_label = self._make_mock_box(clicked_button_label="Update now")
        mock_msgbox_cls.return_value = mock_box
        mock_msgbox_cls.AcceptRole = "accept-role"
        mock_msgbox_cls.RejectRole = "reject-role"
        mantid_download_url = "this/is/a/dummy/mantid/download/url"
        mock_config.getString.return_value = mantid_download_url

        self.update_notif_presenter._show_prompt("7.0.0")

        mock_config.getString.assert_called_once_with("CheckMantidVersion.DownloadURL")
        self.update_notif_presenter._parent.close.assert_called_once()
        mock_qurl_cls.assert_called_once_with(mantid_download_url)
        mock_qdesktop.openUrl.assert_called_once_with(mock_qurl_cls.return_value)
        mock_box.setWindowTitle.assert_called_once_with("Update available")
        mock_box.setText.assert_called_once_with("Mantid Workbench 7.0.0 is available!")

    # ------------------------------------------------------------------
    # "Remind me later" path
    # ------------------------------------------------------------------
    @patch(f"{MODULE}.QMessageBox")
    @patch(f"{MODULE}.ConfigService")
    @patch("qtpy.QtCore.QUrl")
    @patch("qtpy.QtGui.QDesktopServices")
    def test_remind_me_later_does_not_close_or_open_url(self, mock_qdesktop, mock_qurl_cls, mock_config, mock_msgbox_cls):
        mock_box, _ = self._make_mock_box(clicked_button_label="Remind me later")
        mock_msgbox_cls.return_value = mock_box

        self.update_notif_presenter._show_prompt("7.0.0")

        mock_config.getString.assert_not_called()
        self.update_notif_presenter._parent.close.assert_not_called()
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
    def test_open_url_failure_is_logged_and_does_not_raise(self, mock_qdesktop, mock_qurl_cls, mock_config, mock_msgbox_cls, mock_logger):
        mock_box, _ = self._make_mock_box(clicked_button_label="Update now")
        mock_msgbox_cls.return_value = mock_box
        mantid_download_url = "this/is/a/dummy/mantid/download/url"
        mock_config.getString.return_value = mantid_download_url
        openUrlError = "browser error"
        mock_qdesktop.openUrl.side_effect = RuntimeError(openUrlError)

        self.update_notif_presenter._show_prompt("6.16.0")

        self.update_notif_presenter._parent.close.assert_called_once()  # ran before openUrl, unaffected
        mock_logger.error.assert_any_call(f"Failed to open download URL: {openUrlError}")

    @patch(f"{MODULE}.logger")
    @patch(f"{MODULE}.QMessageBox")
    @patch(f"{MODULE}.ConfigService")
    @patch("qtpy.QtCore.QUrl")
    @patch("qtpy.QtGui.QDesktopServices")
    def test_close_failure_is_logged_but_open_url_still_attempted(
        self, mock_qdesktop, mock_qurl_cls, mock_config, mock_msgbox_cls, mock_logger
    ):
        mock_box, _ = self._make_mock_box(clicked_button_label="Update now")
        mock_msgbox_cls.return_value = mock_box
        mantid_download_url = "this/is/a/dummy/mantid/download/url"
        mock_config.getString.return_value = mantid_download_url
        self.update_notif_presenter._parent.close.side_effect = RuntimeError("boom")

        self.update_notif_presenter._show_prompt("7.0.0")

        mock_logger.error.assert_any_call("Failed to close Workbench: boom")
        mock_qdesktop.openUrl.assert_called_once()


if __name__ == "__main__":
    unittest.main()
