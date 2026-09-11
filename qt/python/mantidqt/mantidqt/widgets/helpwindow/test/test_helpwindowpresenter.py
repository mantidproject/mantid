# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import call, Mock, patch

from mantidqt.widgets.helpwindow.helpwindowpresenter import HelpWindowPresenter
from qtpy.QtCore import QUrl

LOG_PATH = "mantidqt.widgets.helpwindow.helpwindowpresenter.log"
OPEN_URL_PATH = "mantidqt.widgets.helpwindow.helpwindowpresenter.QDesktopServices.openUrl"


class TestHelpWindowModelConfigService(unittest.TestCase):
    @patch(LOG_PATH)
    def test_show_help_page_handles_model_failure(self, mock_log):
        help_presenter = HelpWindowPresenter()
        help_presenter.model = None
        help_presenter.show_help_page("dummy")
        mock_log.error.assert_called_once_with("Cannot show help page, model is not available.")

    @patch(OPEN_URL_PATH)
    @patch(LOG_PATH)
    def test_show_help_page_opens_url(self, mock_log, mock_open_url):
        help_presenter = HelpWindowPresenter()
        help_presenter.model = Mock()

        doc_url = QUrl("file:///path/to/index.html")
        help_presenter.model.build_help_url.return_value = doc_url
        mock_open_url.return_value = True

        help_presenter.show_help_page("index.html")

        help_presenter.model.build_help_url.assert_called_once_with("index.html")
        mock_open_url.assert_called_once_with(doc_url)
        mock_log.debug.assert_called_once_with("Opening help page in system browser: 'index.html'")
        mock_log.error.assert_not_called()

    @patch(OPEN_URL_PATH, return_value=False)
    @patch(LOG_PATH)
    def test_show_help_page_logs_error_when_open_url_fails(self, mock_log, mock_open_url):
        help_presenter = HelpWindowPresenter()
        help_presenter.model = Mock()

        doc_url = QUrl("file:///path/to/index.html")
        help_presenter.model.build_help_url.return_value = doc_url
        help_presenter.show_help_page("index.html")

        mock_log.error.assert_called_once_with(f"Failed to open URL in system browser: {doc_url.toString()}")

    @patch(OPEN_URL_PATH, return_value=True)
    @patch(LOG_PATH)
    def test_show_help_page_fallback(self, mock_log, mock_open_url):
        help_presenter = HelpWindowPresenter()
        help_presenter.model = Mock()
        help_presenter.model.ONLINE_BASE_URL = "https://docs.mantidproject.org"

        error = FileNotFoundError("Local help file not found: file:///path/to/index.html")
        help_presenter.model.build_help_url.side_effect = error
        help_presenter.show_help_page("index.html")
        help_presenter.model.build_help_url.assert_called_once_with("index.html")

        mock_open_url.assert_called_once()
        fallback_url = mock_open_url.call_args.args[0]
        self.assertEqual(fallback_url.toString(), "https://docs.mantidproject.org/index.html")
        mock_log.error.assert_called_once_with(f"Documentation file not found: {error}")
        mock_log.debug.assert_any_call("Opening help page in system browser: 'index.html'")
        mock_log.debug.assert_any_call(f"Attempting fallback to online docs: {fallback_url.toString()}")

    @patch(OPEN_URL_PATH, return_value=False)
    @patch(LOG_PATH)
    def test_show_help_page_fallback_fails(self, mock_log, mock_open_url):
        help_presenter = HelpWindowPresenter()
        help_presenter.model = Mock()
        help_presenter.model.ONLINE_BASE_URL = "https://docs.mantidproject.org"

        error = FileNotFoundError("Local help file not found: file:///path/to/index.html")
        help_presenter.model.build_help_url.side_effect = error
        help_presenter.show_help_page("index.html")
        help_presenter.model.build_help_url.assert_called_once_with("index.html")

        mock_open_url.assert_called_once()
        fallback_url = mock_open_url.call_args.args[0]
        self.assertEqual(fallback_url.toString(), "https://docs.mantidproject.org/index.html")
        mock_log.error.assert_has_calls(
            [call(f"Documentation file not found: {error}"), call(f"Failed to open fallback URL: {fallback_url.toString()}")]
        )
        mock_log.debug.assert_any_call("Opening help page in system browser: 'index.html'")
        mock_log.debug.assert_any_call(f"Attempting fallback to online docs: {fallback_url.toString()}")

    @patch(LOG_PATH)
    def test_show_help_page_logs_error_on_unexpected_exception(self, mock_log):
        help_presenter = HelpWindowPresenter()
        help_presenter.model = Mock()

        error = RuntimeError("Something unexpected happened")
        help_presenter.model.build_help_url.side_effect = error

        help_presenter.show_help_page("index.html")

        help_presenter.model.build_help_url.assert_called_once_with("index.html")
        mock_log.error.assert_called_once_with(f"Error opening help page 'index.html': {error}")


if __name__ == "__main__":
    unittest.main()
