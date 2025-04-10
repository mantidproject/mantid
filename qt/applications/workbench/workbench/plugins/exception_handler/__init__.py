# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import traceback

from mantid.kernel import UsageService, logger
from mantidqt.dialogs.errorreports.presenter import ErrorReporterPresenter
from mantidqt.dialogs.errorreports.report import CrashReportPage
from workbench.plugins.exception_handler.error_messagebox import WorkbenchErrorMessageBox


def exception_logger(main_window, exc_type, exc_value, exc_traceback):
    """
    Captures ALL EXCEPTIONS.
    Prevents the Workbench from crashing silently, instead it logs the error on ERROR level.

    :param main_window: A reference to the main window, that will be used to close it in case of the user
                        choosing to terminate the execution.
    :param exc_type: The type of the exception
    :param exc_value: Value of the exception, typically contains the error message.
    :param exc_traceback: Stack trace of the exception.
    """
    logger.error("".join(traceback.format_exception(exc_type, exc_value, exc_traceback)))

    if UsageService.isEnabled():
        page = CrashReportPage(show_continue_terminate=True)
        presenter = ErrorReporterPresenter(page, "", "workbench", None, traceback.format_exception(exc_type, exc_value, exc_traceback))
        presenter.show_view_blocking()
        if not page.continue_working:
            main_window.close()
    else:
        # show the exception message without the traceback
        WorkbenchErrorMessageBox(main_window, "".join(traceback.format_exception_only(exc_type, exc_value))).exec_()
