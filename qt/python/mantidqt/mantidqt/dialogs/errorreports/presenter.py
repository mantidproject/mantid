# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import base64
import json
import os
import zlib
from typing import Optional
from qtpy.QtCore import QSettings

from mantid.kernel import ConfigService, ErrorReporter, Logger, UsageService
from mantid.kernel.environment import is_linux
from mantidqt.dialogs.errorreports.report import MAX_STACK_TRACE_LENGTH
from mantidqt.dialogs.errorreports.run_pystack import retrieve_thread_traces_from_coredump_file


class ErrorReporterPresenter(object):
    SENDING_ERROR_MESSAGE = "There was an error when sending the report.\nPlease contact mantid-help@mantidproject.org directly"

    def __init__(self, view, exit_code: str, application: str, workbench_pid: str, traceback: Optional[str] = None):
        """
        :param view: A reference to the view managed by this presenter
        :param exit_code: A string containing the exit_code of the failing application
        :param application: A string containing the failing application name
        :param traceback: An optional string containing a traceback dumped as JSON-encoded string
        """
        self.error_log = Logger("errorreports")
        self._view = view
        self._exit_code = exit_code
        self._application = application
        self._traceback = traceback if traceback else ""
        self._cpp_traces = b""
        self._view.set_report_callback(self.error_handler)
        self._view.moreDetailsButton.clicked.connect(self.show_more_details)

        if not traceback:
            traceback_file_path = os.path.join(ConfigService.getAppDataDirectory(), "{}_stacktrace.txt".format(application))
            try:
                if os.path.isfile(traceback_file_path):
                    with open(traceback_file_path, "r") as file:
                        self._traceback = file.readlines()
                    new_workspace_name = os.path.join(ConfigService.getAppDataDirectory(), "{}_stacktrace_sent.txt".format(application))
                    os.rename(traceback_file_path, new_workspace_name)
                elif is_linux():
                    self._cpp_traces = retrieve_thread_traces_from_coredump_file(workbench_pid)
            except OSError:
                pass

    def forget_contact_info(self):
        settings = QSettings()
        settings.beginGroup(self._view.CONTACT_INFO)
        settings.setValue(self._view.NAME, "")
        settings.setValue(self._view.EMAIL, "")
        settings.endGroup()

    def do_not_share(self, continue_working=True):
        self.error_log.notice("No information shared")
        self._handle_exit(continue_working)
        if not self._view.rememberContactInfoCheckbox.checkState():
            self.forget_contact_info()
        return -1

    def share_all_information(self, continue_working, new_name, new_email, text_box):
        uptime = UsageService.getUpTime()
        status = self._send_report_to_server(share_identifiable=True, uptime=uptime, name=new_name, email=new_email, text_box=text_box)
        self.error_log.notice("Sent full information")
        self._handle_exit(continue_working)

        # Remember name and email in QSettings
        if self._view.rememberContactInfoCheckbox.checkState():
            settings = QSettings()
            settings.beginGroup(self._view.CONTACT_INFO)
            settings.setValue(self._view.NAME, new_name)
            settings.setValue(self._view.EMAIL, new_email)
            settings.endGroup()
        else:
            self.forget_contact_info()
        return status

    def error_handler(self, continue_working, share, name, email, text_box):
        if share:
            status = self.share_all_information(continue_working, name, email, text_box)
        else:
            status = self.do_not_share(continue_working)

        self._view.close_reporter(status)

    def _handle_exit(self, continue_working):
        if not continue_working:
            self.error_log.error("Terminated by user.")
            self._view.quit()
        else:
            self.error_log.error("Continue working.")

    def _send_report_to_server(self, share_identifiable=False, name="", email="", uptime="", text_box=""):
        stacktrace = "".join(self._traceback)
        if len(stacktrace) > MAX_STACK_TRACE_LENGTH:
            difference = len(stacktrace) - MAX_STACK_TRACE_LENGTH
            stacktrace = self._cut_down_stacktrace()
            self.error_log.warning(
                f"The middle {difference + 5} characters of this stack trace has been removed"
                r" and replaced with \n...\n in order to"
                f" reduce it to {MAX_STACK_TRACE_LENGTH} characters"
            )

        errorReporter = ErrorReporter(
            self._application,
            uptime,
            self._exit_code,
            share_identifiable,
            str(name),
            str(email),
            str(text_box),
            stacktrace,
            self._cpp_traces,
        )

        status = errorReporter.sendErrorReport()

        if status != 201:
            self._view.display_message_box(
                "Error contacting server", self.SENDING_ERROR_MESSAGE, "http request returned with status {}".format(status)
            )
            self.error_log.error("Failed to send error report http request returned status {}".format(status))

        return status

    def _cut_down_stacktrace(self):
        # server has a max size for the stack trace, if exceeded will cause an error
        stacktrace = "".join(self._traceback)
        return stacktrace[: (MAX_STACK_TRACE_LENGTH // 2 - 2)] + "\n...\n" + stacktrace[-(MAX_STACK_TRACE_LENGTH // 2 - 3) :]

    def show_view(self):
        self._view.show()

    def show_view_blocking(self):
        self._view.exec_()

    def show_more_details(self):
        error_reporter = ErrorReporter(
            self._application,
            UsageService.getUpTime(),
            self._exit_code,
            True,
            str(self._view.input_name_line_edit.text()),
            str(self._view.input_email_line_edit.text()),
            str(self._view.input_free_text.toPlainText()),
            "",
            "",
        )

        error_message_json = json.loads(error_reporter.generateErrorMessage())
        if self._cpp_traces:
            stacktrace_text = zlib.decompress(base64.standard_b64decode(self._cpp_traces)).decode("utf-8")
        else:
            stacktrace_text = "".join(self._traceback)
        del error_message_json["stacktrace"]  # remove this entry so it doesn't appear twice.
        del error_message_json["cppCompressedTraces"]
        user_information = "".join("{}: {}\n".format(key, error_message_json[key]) for key in error_message_json)
        self._view.display_more_details(user_information, stacktrace_text)
