# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import argparse
import importlib
import sys
from typing import Sequence

import mantid
from qtpy import QT_VERSION
from qtpy.QtCore import QCoreApplication, QSettings

from mantidqt.dialogs.errorreports.presenter import ErrorReporterPresenter
from mantidqt.dialogs.errorreports.report import CrashReportPage
import mantidqt.utils.qt as qtutils


def main(argv: Sequence[str] = None) -> int:
    argv = argv if argv is not None else sys.argv

    command_line_args = parse_commandline(argv)
    exit_code_str = command_line_args.exit_code
    exit_code = int(exit_code_str)
    if mantid.config["usagereports.enabled"] != "1":
        return exit_code

    # On Windows/macOS the plugin locations need to be known before starting
    # QApplication
    if command_line_args.qtdir is not None:
        QCoreApplication.addLibraryPath(command_line_args.qtdir)

    # Qt resources must be imported before QApplication starts
    importlib.import_module(f"mantidqt.dialogs.errorreports.resources_qt{QT_VERSION[0]}")

    if sys.platform == "darwin":
        qtutils.force_layer_backing_BigSur()

    from qtpy.QtWidgets import QApplication

    app = QApplication(argv)
    # The strings APPNAME, ORG_DOMAIN, ORGANIZATION are duplicated from workbench.config
    app.setOrganizationName(command_line_args.org_name)
    app.setOrganizationDomain(command_line_args.org_domain)
    app.setApplicationName(command_line_args.application)
    QSettings.setDefaultFormat(QSettings.IniFormat)
    form = CrashReportPage(show_continue_terminate=False)
    presenter = ErrorReporterPresenter(form, exit_code_str, command_line_args.application, command_line_args.workbench_pid)
    presenter.show_view()
    app.exec_()

    return exit_code


def parse_commandline(argv: Sequence[str]) -> argparse.Namespace:
    """
    Parse the command line arguments and return them to the caller
    """
    parser = argparse.ArgumentParser(description="Pass in exit_code")
    parser.add_argument("--exitcode", dest="exit_code", default="1")
    parser.add_argument("--qtdir", dest="qtdir", default="")
    parser.add_argument("--orgname", dest="org_name", default="unknown")
    parser.add_argument("--orgdomain", dest="org_domain", default="unknown")
    parser.add_argument("--application", dest="application", default="unknown")
    parser.add_argument("--workbench_pid", dest="workbench_pid", default="")
    return parser.parse_args(argv)


if __name__ == "__main__":  # if we're running file directly and not importing it
    sys.exit(main())  # run the main function
