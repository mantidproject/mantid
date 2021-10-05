# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import json
import os
from typing import Optional

from mantid.kernel import ConfigService, ErrorReporter, Logger, UsageService


class ErrorReporterPresenter(object):
    SENDING_ERROR_MESSAGE = 'There was an error when sending the report.\nPlease contact mantid-help@mantidproject.org directly'

    def __init__(self, view, exit_code: str, application: str, traceback: Optional[str] = None):
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
        self._traceback = traceback if traceback else ''
        self._view.set_report_callback(self.error_handler)
        self._view.moreDetailsButton.clicked.connect(self.show_more_details)

        if not traceback:
            traceback_file_path = os.path.join(ConfigService.getAppDataDirectory(),
                                               '{}_stacktrace.txt'.format(application))
            try:
                if os.path.isfile(traceback_file_path):
                    with open(traceback_file_path, 'r') as file:
                        self._traceback = file.readlines()
                    new_workspace_name = os.path.join(ConfigService.getAppDataDirectory(),
                                                      '{}_stacktrace_sent.txt'.format(application))
                    os.rename(traceback_file_path, new_workspace_name)
            except OSError:
                pass

    def do_not_share(self, continue_working=True):
        self.error_log.notice("No information shared")
        self._handle_exit(continue_working)
        return -1

    def share_non_identifiable_information(self, continue_working):
        uptime = UsageService.getUpTime()
        status = self._send_report_to_server(share_identifiable=False, uptime=uptime)
        self.error_log.notice("Sent non-identifiable information")
        self._handle_exit(continue_working)
        return status

    def share_all_information(self, continue_working, name, email, text_box):
        uptime = UsageService.getUpTime()
        status = self._send_report_to_server(share_identifiable=True,
                                             uptime=uptime,
                                             name=name,
                                             email=email,
                                             text_box=text_box)
        self.error_log.notice("Sent full information")

        self._handle_exit(continue_working)
        return status

    def error_handler(self, continue_working, share, name, email, text_box):
        if share == 0:
            status = self.share_all_information(continue_working, name, email, text_box)
        elif share == 1:
            status = self.share_non_identifiable_information(continue_working)
        elif share == 2:
            status = self.do_not_share(continue_working)
        else:
            self.error_log.error("Unrecognised signal in errorreporter exiting")
            self._handle_exit(continue_working)
            status = -2

        return status

    def _handle_exit(self, continue_working):
        if not continue_working:
            self.error_log.error("Terminated by user.")
            self._view.quit()
        else:
            self.error_log.error("Continue working.")

    def _send_report_to_server(self,
                               share_identifiable=False,
                               name='',
                               email='',
                               uptime='',
                               text_box=''):
        errorReporter = ErrorReporter(self._application, uptime,
                                      self._exit_code, share_identifiable, str(name), str(email),
                                      str(text_box), "".join(self._traceback))
        status = errorReporter.sendErrorReport()

        if status != 201:
            self._view.display_message_box('Error contacting server', self.SENDING_ERROR_MESSAGE,
                                           'http request returned with status {}'.format(status))
            self.error_log.error(
                "Failed to send error report http request returned status {}".format(status))

        return status

    def show_view(self):
        self._view.show()

    def show_view_blocking(self):
        self._view.exec_()

    def show_more_details(self):
        error_reporter = ErrorReporter(self._application, UsageService.getUpTime(), self._exit_code,
                                       True, str(self._view.input_name_line_edit.text()),
                                       str(self._view.input_email_line_edit.text()),
                                       str(self._view.input_free_text.toPlainText()),
                                       "".join(self._traceback))

        error_message_json = json.loads(error_reporter.generateErrorMessage())
        stacktrace_text = error_message_json["stacktrace"]
        del error_message_json["stacktrace"]  # remove this entry so it doesn't appear twice.
        user_information = ''.join('{}: {}\n'.format(key, error_message_json[key])
                                   for key in error_message_json)
        self._view.display_more_details(user_information, stacktrace_text)
