# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqt.dialogs.errorreports.presenter import ErrorReporterPresenter
from mantidqt.dialogs.errorreports.report import MAX_STACK_TRACE_LENGTH


class ErrorReportPresenterTest(unittest.TestCase):
    PRESENTER_CLS_PATH = "mantidqt.dialogs.errorreports.presenter"

    def setUp(self):
        self.logger_mock_instance = mock.MagicMock()
        logger_patcher = mock.patch(f"{self.PRESENTER_CLS_PATH}.Logger")
        self.addCleanup(logger_patcher.stop)
        self.logger_mock = logger_patcher.start()
        self.logger_mock.return_value = self.logger_mock_instance

        self.errorreport_mock_instance = mock.MagicMock()
        errorreport_patcher = mock.patch(f"{self.PRESENTER_CLS_PATH}.ErrorReporter")
        self.addCleanup(errorreport_patcher.stop)
        self.errorreport_mock = errorreport_patcher.start()
        self.errorreport_mock.return_value = self.errorreport_mock_instance

        self.view = mock.MagicMock()
        self.exit_code = 255
        self.app_name = "ErrorReportPresenterTest"
        self.error_report_presenter = ErrorReporterPresenter(self.view, self.exit_code, application=self.app_name, workbench_pid=None)
        self.view.CONTACT_INFO = "ContactInfo"
        self.view.NAME = "John Smith"
        self.view.EMAIL = "john.smith@example.com"

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
        name = "John Smith"
        email = "john.smith@example.com"
        text_box = "details of error"
        uptime = "time_string"
        self.errorreport_mock_instance.sendErrorReport.return_value = 201
        self.error_report_presenter._send_report_to_server(False, name=name, email=email, uptime=uptime, text_box=text_box)

        self.errorreport_mock.assert_called_once_with(self.app_name, uptime, self.exit_code, False, name, email, text_box, "", b"")
        self.errorreport_mock_instance.sendErrorReport.assert_called_once_with()

    def test_send_error_report_to_server_calls_ErrorReport_correctly_and_triggers_view_upon_failure(self):
        name = "John Smith"
        email = "john.smith@example.com"
        uptime = "time_string"
        text_box = "details of error"

        self.errorreport_mock_instance.sendErrorReport.return_value = 500
        self.error_report_presenter._send_report_to_server(True, name=name, email=email, uptime=uptime, text_box=text_box)

        self.errorreport_mock.assert_called_once_with(self.app_name, uptime, self.exit_code, True, name, email, text_box, "", b"")
        self.errorreport_mock_instance.sendErrorReport.assert_called_once_with()
        self.view.display_message_box.assert_called_once_with(
            "Error contacting server", ErrorReporterPresenter.SENDING_ERROR_MESSAGE, "http request returned with status 500"
        )

    def test_error_handler_share_all_sunny_day_case(self):
        name = "John Smith"
        email = "john.smith@example.com"
        text_box = "Details about error"
        continue_working = False
        share = True
        self.error_report_presenter._send_report_to_server = mock.MagicMock(return_value=201)
        self.error_report_presenter._handle_exit = mock.MagicMock()

        self.error_report_presenter.error_handler(continue_working, share, name, email, text_box)

        self.error_report_presenter._send_report_to_server.called_once_with(
            share_identifiable=True, name=name, email=email, uptime=mock.ANY, text_box=text_box
        )
        self.error_report_presenter._handle_exit.assert_called_once_with(False)

    def test_error_handler_share_nothing_sunny_day_case(self):
        name = "John Smith"
        email = "john.smith@example.com"
        text_box = "Details about error"
        continue_working = True
        share = False
        self.error_report_presenter._send_report_to_server = mock.MagicMock()
        self.error_report_presenter._handle_exit = mock.MagicMock()

        self.error_report_presenter.error_handler(continue_working, share, name, email, text_box)

        self.assertEqual(self.error_report_presenter._send_report_to_server.call_count, 0)
        self.error_report_presenter._handle_exit.assert_called_once_with(True)

    def test_cut_down_stacktrace(self):
        for n in [1, 11, 100]:
            self.error_report_presenter._traceback = "x" * (MAX_STACK_TRACE_LENGTH + 100)
            cut_down_stack_trace = self.error_report_presenter._cut_down_stacktrace()
            self.assertEqual(len(cut_down_stack_trace), MAX_STACK_TRACE_LENGTH)
            self.assertIn("\n...\n", cut_down_stack_trace)

    def test_cut_down_stacktrace_is_called(self):
        self.error_report_presenter._traceback = "x" * (MAX_STACK_TRACE_LENGTH + 100)
        self.error_report_presenter._cut_down_stacktrace = mock.MagicMock()
        self.errorreport_mock_instance.sendErrorReport.return_value = 201
        self.error_report_presenter._send_report_to_server()
        self.error_report_presenter._cut_down_stacktrace.assert_called_once()


if __name__ == "__main__":
    unittest.main()
