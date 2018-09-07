import unittest
import sys
if sys.version_info.major > 2:
    from unittest import mock
else:
    import mock
from ErrorReporter.error_report_presenter import ErrorReporterPresenter

import math

class ErrorReportPresenterTest(unittest.TestCase):
    def setUp(self):
        self.logger_mock_instance = mock.MagicMock()
        logger_patcher = mock.patch('ErrorReporter.error_report_presenter.Logger')
        self.addCleanup(logger_patcher.stop)
        self.logger_mock = logger_patcher.start()
        self.logger_mock.return_value = self.logger_mock_instance

        self.errorreport_mock_instance = mock.MagicMock()
        errorreport_patcher = mock.patch('ErrorReporter.error_report_presenter.ErrorReporter')
        self.addCleanup(errorreport_patcher.stop)
        self.errorreport_mock = errorreport_patcher.start()
        self.errorreport_mock.return_value = self.errorreport_mock_instance

        self.view = mock.MagicMock()
        self.exit_code = 255
        self.error_report_presenter = ErrorReporterPresenter(self.view, self.exit_code)

    def test_sets_logger_view_and_exit_code_upon_construction(self):
        self.assertEqual(self.error_report_presenter._exit_code, self.exit_code)
        self.assertEqual(self.error_report_presenter._view, self.view)
        self.assertEqual(self.error_report_presenter.error_log, self.logger_mock_instance)

    def test_do_not_share_with_continue_performs_appropriate_logging(self):
        self.error_report_presenter.do_not_share(continue_working=True)

        self.logger_mock_instance.notice.assert_called_once_with("No information shared")
        self.logger_mock_instance.error.assert_called_once_with("Continue working.")
        self.assertEqual(self.view.quit.call_count, 0)

    def test_do_not_share_without_continue_performs_appropriate_logging(self):
        self.error_report_presenter.do_not_share(continue_working=False)

        self.logger_mock_instance.notice.assert_called_once_with("No information shared")
        self.logger_mock_instance.error.assert_called_once_with("Terminated by user.")
        self.assertEqual(self.view.quit.call_count, 1)

    def test_send_error_report_to_server_calls_ErrorReport_correctly(self):
        name = 'John Smith'
        email = 'john.smith@email.com'
        file_hash = '91df56ab7a2de7264052a7a7125dcfa18b4f59de'
        uptime = 'time_string'
        self.errorreport_mock_instance.sendErrorReport.return_value = 201
        self.error_report_presenter._send_report_to_server(False, name=name, email=email,
                                                           file_hash=file_hash,
                                                           uptime=uptime)

        self.errorreport_mock.assert_called_once_with('mantidplot', uptime, self.exit_code, False, name,
                                                      email, file_hash)
        self.errorreport_mock_instance.sendErrorReport.assert_called_once_with()

    def test_send_error_report_to_server_calls_ErrorReport_correctly_and_triggers_view_upon_failure(self):
        name = 'John Smith'
        email = 'john.smith@email.com'
        file_hash = '91df56ab7a2de7264052a7a7125dcfa18b4f59de'
        uptime = 'time_string'
        self.errorreport_mock_instance.sendErrorReport.return_value = 500
        self.error_report_presenter._send_report_to_server(True, name=name, email=email,
                                                           file_hash=file_hash,
                                                           uptime=uptime)

        self.errorreport_mock.assert_called_once_with('mantidplot', uptime, self.exit_code, True, name,
                                                      email, file_hash)
        self.errorreport_mock_instance.sendErrorReport.assert_called_once_with()
        self.view.display_message_box.assert_called_once_with('Error contacting server',
                                                              'There was an error when sending the report.'
                                                              'Please contact mantid-help@mantidproject.org directly',
                                                              'http request returned with status 500')

    def test_error_handler_share_all_sunny_day_case(self):
        name = 'John Smith'
        email = 'john.smith@email.com'
        continue_working = False
        share = 0

        self.error_report_presenter.error_handler(continue_working, share, name, email)







if __name__ == '__main__':
    unittest.main()