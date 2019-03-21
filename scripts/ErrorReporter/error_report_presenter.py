# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, print_function)

import os

import requests

from ErrorReporter.retrieve_recovery_files import zip_recovery_directory
from mantid.kernel import ConfigService, ErrorReporter, Logger, UsageService


class ErrorReporterPresenter(object):
    SENDING_ERROR_MESSAGE = 'There was an error when sending the report.\nPlease contact mantid-help@mantidproject.org directly'

    def __init__(self, view, exit_code):
        self.error_log = Logger("error")
        self._view = view
        self._exit_code = exit_code
        self._view.set_report_callback(self.error_handler)

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
        try:
            recovery_archive, file_hash = zip_recovery_directory()
        except Exception as exc:
            self.error_log.information("Error creating recovery archive: {}. No recovery information will be sent")
            recovery_archive, file_hash = None, ""
        status = self._send_report_to_server(share_identifiable=True, uptime=uptime, name=name, email=email,
                                             file_hash=file_hash, text_box=text_box)
        self.error_log.notice("Sent full information")
        if status == 201 and recovery_archive:
            self._upload_recovery_file(recovery_archive=recovery_archive)
            try:
                os.remove(recovery_archive)
            except OSError as exc:
                self.error_log.information("Unable to remove zipped recovery information: {}".format(str(exc)))

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

    def _upload_recovery_file(self, recovery_archive):
        url = ConfigService['errorreports.rooturl']
        url = '{}/api/recovery'.format(url)
        self.error_log.notice("Sending recovery file to address: {}".format(url))
        files = {'file': open('{}'.format(recovery_archive), 'rb')}
        try:
            # timeout after 20 seconds to match the C++ error reporter timeout
            response = requests.post(url, files=files, timeout=20)
        except Exception as e:
            self.error_log.error(
                "Failed to send recovery data. Could not establish connection to URL: {}.\n\nFull trace:\n\n{}".format(
                    url, e))
            return

        # if this is reached, the connection was successful and some response was received
        if response.status_code == 201:
            self.error_log.notice("Uploaded recovery file to server. HTTP response {}".format(response.status_code))
        elif response.status_code == 413:
            self.error_log.notice(
                "Data was too large, and was not accepted by the server. HTTP response {}".format(response.status_code))
        else:
            self.error_log.error("Failed to send recovery data. HTTP response {}".format(response.status_code))

    def _send_report_to_server(self, share_identifiable=False, name='', email='', file_hash='', uptime='', text_box=''):
        errorReporter = ErrorReporter(
            "mantidplot", uptime, self._exit_code, share_identifiable, str(name), str(email), str(text_box),
            str(file_hash))
        status = errorReporter.sendErrorReport()

        if status != 201:
            self._view.display_message_box('Error contacting server', self.SENDING_ERROR_MESSAGE,
                                           'http request returned with status {}'.format(status))
            self.error_log.error("Failed to send error report http request returned status {}".format(status))

        return status

    def show_view(self):
        self._view.show()

    def show_view_blocking(self):
        self._view.exec_()
