from __future__ import (absolute_import, division, print_function)
import unittest
import sys
from ErrorReporter.error_report_presenter import ErrorReporterPresenter
if sys.version_info.major > 2:
    from unittest import mock
else:
    import mock


class ErrorReportPresenterTest(unittest.TestCase):

    def test_that_can_initialise(self):
        view = mock.MagicMock()
        exit_code = 0
        presenter = ErrorReporterPresenter(view, exit_code)

        self.assertEqual(view, presenter._view)
        self.assertEqual(exit_code, presenter._exit_code)
        presenter._view.connect_signal.called_once_with(presenter.error_handler)
        presenter._view.show.called_once_with()

    @mock.patch('ErrorReporter.error_report_presenter.ErrorReporter')
    def test_error_handler_sends_no_report_for_no_share(self, error_reporter_mock):
        error_reporter_mock_instance = mock.MagicMock()
        error_reporter_mock_instance.sendErrorReport.return_value = 201
        error_reporter_mock.return_value = error_reporter_mock_instance
        share = 2
        continue_working = True
        name = ''
        email = ''
        view = mock.MagicMock()
        exit_code = 0
        presenter = ErrorReporterPresenter(view, exit_code)

        presenter.error_handler(continue_working, share, name, email)

        self.assertEqual(error_reporter_mock.call_count, 0)
        self.assertEqual(presenter._view.quit.call_count, 0)

    @mock.patch('ErrorReporter.error_report_presenter.ErrorReporter')
    @mock.patch('ErrorReporter.error_report_presenter.UsageService')
    def test_error_handler_sends_report_for_share_all(self, usage_service, error_reporter_mock):
        usage_service.getUpTime.return_value = 'up_time'
        error_reporter_mock_instance = mock.MagicMock()
        error_reporter_mock_instance.sendErrorReport.return_value = 201
        error_reporter_mock.return_value = error_reporter_mock_instance
        share = 0
        continue_working = False
        name = ''
        email = ''
        view = mock.MagicMock()
        exit_code = 0
        presenter = ErrorReporterPresenter(view, exit_code)

        presenter.error_handler(continue_working, share, name, email)

        self.assertEqual(error_reporter_mock.call_count, 1)
        error_reporter_mock_instance.sendErrorReport.asser_called_once_with(
            "mantidplot", 'up_time', exit_code, False, str(name), str(email))
        presenter._view.quit.assert_called_once_with()

    @mock.patch('ErrorReporter.error_report_presenter.ErrorReporter')
    @mock.patch('ErrorReporter.error_report_presenter.UsageService')
    def test_error_handler_sends_report_for_share_non_id(self, usage_service, error_reporter_mock):
        usage_service.getUpTime.return_value = 'up_time'
        error_reporter_mock_instance = mock.MagicMock()
        error_reporter_mock_instance.sendErrorReport.return_value = 201
        error_reporter_mock.return_value = error_reporter_mock_instance
        share = 1
        continue_working = False
        name = ''
        email = ''
        view = mock.MagicMock()
        exit_code = 0
        presenter = ErrorReporterPresenter(view, exit_code)

        presenter.error_handler(continue_working, share, name, email)

        self.assertEqual(error_reporter_mock.call_count, 1)
        error_reporter_mock_instance.sendErrorReport.asser_called_once_with(
            "mantidplot", 'up_time', exit_code, True, str(name), str(email))
        presenter._view.quit.assert_called_once_with()

    @mock.patch('ErrorReporter.error_report_presenter.ErrorReporter')
    @mock.patch('ErrorReporter.error_report_presenter.UsageService')
    def test_error_handler_reports_if_incorrect_http_response_sent(self, usage_service, error_reporter_mock):
        usage_service.getUpTime.return_value = 'up_time'
        error_reporter_mock_instance = mock.MagicMock()
        error_reporter_mock_instance.sendErrorReport.return_value = 500
        error_reporter_mock.return_value = error_reporter_mock_instance
        share = 0
        continue_working = False
        name = ''
        email = ''
        view = mock.MagicMock()
        exit_code = 0
        presenter = ErrorReporterPresenter(view, exit_code)

        presenter.error_handler(continue_working, share, name, email)

        self.assertEqual(error_reporter_mock.call_count, 1)
        error_reporter_mock_instance.sendErrorReport.asser_called_once_with(
            "mantidplot", 'up_time', exit_code, False, str(name), str(email))
        presenter._view.display_message_box.assert_called_once_with('Error contacting server',
                                                                    'There was an error when sending the report.'
                                                                    'Please contact mantid-help@mantidproject.org directly',
                                                                    'http request returned with status 500')
        presenter._view.quit.assert_called_once_with()


if __name__ == '__main__':
    unittest.main()
