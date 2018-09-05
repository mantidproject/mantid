from mantid.kernel import ErrorReporter, UsageService
from mantid.kernel import Logger


class ErrorReporterPresenter(object):
    def __init__(self, view, exit_code):
        self.error_log = Logger("error")
        self._view = view
        self._exit_code = exit_code
        self._view.action.connect(self.error_handler)

    def error_handler(self, continue_working, share, name, email):
        status = -1
        if share == 0:
            errorReporter = ErrorReporter(
                "mantidplot", UsageService.getUpTime(), self._exit_code, True, str(name), str(email))
            status = errorReporter.sendErrorReport()
        elif share == 1:
            errorReporter = ErrorReporter(
                "mantidplot", UsageService.getUpTime(), self._exit_code, False, str(name), str(email))
            status = errorReporter.sendErrorReport()

        if status != 201:
            self._view.display_message_box('Error contacting server','There was an error when sending the report.'
                                           'Please contact mantid-help@mantidproject.org directly',
                                           'http request returned with status {}'.format(status))
            self.error_log.error("Failed to send error report http request returned status {}".format(status))

        if not continue_working:
            self.error_log.error("Terminated by user.")
            self._view.quit()
        else:
            self.error_log.error("Continue working.")

    def show_view(self):
        self._view.show()
