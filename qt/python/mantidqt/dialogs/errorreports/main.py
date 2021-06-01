# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import argparse
import importlib
import sys
import os

import mantid
from qtpy import QT_VERSION
from qtpy.QtCore import QCoreApplication

from mantidqt.dialogs.errorreports.presenter import ErrorReporterPresenter
from mantidqt.dialogs.errorreports.report import CrashReportPage


def main() -> int:
    command_line_args = parse_commandline()
    exit_code_str = command_line_args.exit_code
    exit_code = int(exit_code_str)
    if mantid.config['usagereports.enabled'] != '1':
        return exit_code

    # On Windows/macOS the plugin locations need to be known before starting
    # QApplication
    if command_line_args.qtdir is not None:
        QCoreApplication.addLibraryPath(command_line_args.qtdir)

    # Qt resources must be imported before QApplication starts
    importlib.import_module(f'mantidqt.dialogs.errorreports.resources_qt{QT_VERSION[0]}')

    if sys.platform == 'darwin':
        # Force layer-backing on macOS >= Big Sur (11)
        # or the application hangs
        # A fix inside Qt is yet to be released:
        # https://codereview.qt-project.org/gitweb?p=qt/qtbase.git;a=commitdiff;h=c5d904639dbd690a36306e2b455610029704d821
        # A complication with Big Sur numbering means we check for 10.16 and 11:
        #   https://eclecticlight.co/2020/08/13/macos-version-numbering-isnt-so-simple/
        from distutils.version import LooseVersion
        import platform
        mac_vers = LooseVersion(platform.mac_ver()[0])
        if mac_vers >= '11' or mac_vers == '10.16':
            os.environ['QT_MAC_WANTS_LAYER'] = '1'
        print('############\nDANIEL WAS HERE\n##############\n')

    from qtpy.QtWidgets import QApplication
    app = QApplication(sys.argv)
    form = CrashReportPage(show_continue_terminate=False)
    presenter = ErrorReporterPresenter(form, exit_code_str, command_line_args.application)
    presenter.show_view()
    app.exec_()

    return exit_code


def parse_commandline() -> argparse.Namespace:
    """
    Parse the command line arguments and return them to the caller
    """
    parser = argparse.ArgumentParser(description='Pass in exit_code')
    parser.add_argument('--exitcode', dest='exit_code', default=0)
    parser.add_argument('--qtdir', dest='qtdir', default='')
    parser.add_argument('--application', dest='application')
    return parser.parse_args()


if __name__ == '__main__':  # if we're running file directly and not importing it
    sys.exit(main())  # run the main function
