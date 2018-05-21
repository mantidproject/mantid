from mantid.kernel import ErrorReporter, UsageService


class ErrorReporterPresenter():
    def __init__(self, view, exit_code):
        self._view = view
        self._exit_code = exit_code
        self._view.connect_signal(self.error_handler)
        self._view.show()

    def error_handler(self, continue_working, share, name, email):
        if share == 0:
            errorReporter = ErrorReporter(
                "mantidplot",UsageService.getUpTime(), self._exit_code,
                True, str(name), str(email))
            errorReporter.sendErrorReport()
        elif share == 1:
            errorReporter = ErrorReporter(
                "mantidplot",UsageService.getUpTime(), self._exit_code,
                False, str(name), str(email))
            errorReporter.sendErrorReport()
  
        if not continue_working:
            self._view.quit()
