from mantid.kernel import ErrorReporter, UsageService
from ErrorReporter.errorreport import CrashReportPage


class ErrorReporterPresenter():
    def __init__(self, view):
        self._view = view
        self._view.connect_signal(self.error_handler)
        self._view.show()

    def error_handler(self, continue_working, share, name, email):
        if share == 0:
            errorReporter = ErrorReporter(
                "mantidplot",UsageService.getUpTime(), "",
                True, str(name), str(email))
            errorReporter.sendErrorReport()
        elif share == 1:
            errorReporter = ErrorReporter(
                "mantidplot",UsageService.getUpTime(), "",
                False, str(name), str(email))
            errorReporter.sendErrorReport()
  
        if not continue_working:
            self._view.quit()
