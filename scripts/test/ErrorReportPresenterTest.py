# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from ErrorReporter.error_report_presenter import ErrorReporterPresenter
from mantid.py3compat import mock


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

        zip_recovery_patcher = mock.patch('ErrorReporter.error_report_presenter.zip_recovery_directory')
        self.addCleanup(zip_recovery_patcher.stop)
        self.zip_recovery_mock = zip_recovery_patcher.start()
        self.zip_recovery_mock.return_value = ('zipped_file', 'file_hash')

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
        text_box = 'details of error'
        uptime = 'time_string'
        self.errorreport_mock_instance.sendErrorReport.return_value = 201
        self.error_report_presenter._send_report_to_server(False, name=name, email=email,
                                                           file_hash=file_hash,
                                                           uptime=uptime, text_box=text_box)

        self.errorreport_mock.assert_called_once_with('mantidplot', uptime, self.exit_code, False, name,
                                                      email, text_box, file_hash)
        self.errorreport_mock_instance.sendErrorReport.assert_called_once_with()

    def test_send_error_report_to_server_calls_ErrorReport_correctly_and_triggers_view_upon_failure(self):
        name = 'John Smith'
        email = 'john.smith@email.com'
        file_hash = '91df56ab7a2de7264052a7a7125dcfa18b4f59de'
        uptime = 'time_string'
        text_box = 'details of error'

        self.errorreport_mock_instance.sendErrorReport.return_value = 500
        self.error_report_presenter._send_report_to_server(True, name=name, email=email,
                                                           file_hash=file_hash,
                                                           uptime=uptime, text_box=text_box)

        self.errorreport_mock.assert_called_once_with('mantidplot', uptime, self.exit_code, True, name,
                                                      email, text_box, file_hash)
        self.errorreport_mock_instance.sendErrorReport.assert_called_once_with()
        self.view.display_message_box.assert_called_once_with('Error contacting server',
                                                              ErrorReporterPresenter.SENDING_ERROR_MESSAGE,
                                                              'http request returned with status 500')

    def test_error_handler_share_all_sunny_day_case(self):
        name = 'John Smith'
        email = 'john.smith@email.com'
        text_box = 'Details about error'
        continue_working = False
        share = 0
        self.error_report_presenter._send_report_to_server = mock.MagicMock(return_value=201)
        self.error_report_presenter._upload_recovery_file = mock.MagicMock()
        self.error_report_presenter._handle_exit = mock.MagicMock()

        self.error_report_presenter.error_handler(continue_working, share, name, email, text_box)

        self.error_report_presenter._send_report_to_server.called_once_with(share_identifiable=True, name=name,
                                                                            email=email,
                                                                            file_hash=mock.ANY, uptime=mock.ANY,
                                                                            text_box=text_box)
        self.assertEqual(self.error_report_presenter._upload_recovery_file.call_count, 1)
        self.error_report_presenter._handle_exit.assert_called_once_with(False)

    def test_error_handler_share_non_id_sunny_day_case(self):
        name = 'John Smith'
        email = 'john.smith@email.com'
        text_box = 'Details about error'
        continue_working = True
        share = 1
        self.error_report_presenter._send_report_to_server = mock.MagicMock()
        self.error_report_presenter._upload_recovery_file = mock.MagicMock()
        self.error_report_presenter._handle_exit = mock.MagicMock()

        self.error_report_presenter.error_handler(continue_working, share, name, email, text_box)

        self.error_report_presenter._send_report_to_server.called_once_with(share_identifiable=False, uptime=mock.ANY)
        self.assertEqual(self.error_report_presenter._upload_recovery_file.call_count, 0)
        self.error_report_presenter._handle_exit.assert_called_once_with(True)

    def test_error_handler_share_nothing_sunny_day_case(self):
        name = 'John Smith'
        email = 'john.smith@email.com'
        text_box = 'Details about error'
        continue_working = True
        share = 2
        self.error_report_presenter._send_report_to_server = mock.MagicMock()
        self.error_report_presenter._upload_recovery_file = mock.MagicMock()
        self.error_report_presenter._handle_exit = mock.MagicMock()

        self.error_report_presenter.error_handler(continue_working, share, name, email, text_box)

        self.assertEqual(self.error_report_presenter._send_report_to_server.call_count, 0)
        self.assertEqual(self.error_report_presenter._upload_recovery_file.call_count, 0)
        self.error_report_presenter._handle_exit.assert_called_once_with(True)


if __name__ == '__main__':
    unittest.main()
