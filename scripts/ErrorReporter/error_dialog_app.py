# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, print_function)

import argparse
import sys

parser = argparse.ArgumentParser(description='Pass in exit_code')
parser.add_argument('--exitcode', dest='exit_code')
parser.add_argument('--directory', dest='directory')
parser.add_argument('--qtdir', dest='qtdir')

command_line_args = parser.parse_args()

sys.path.insert(0, command_line_args.directory)

import mantid  # noqa

import qtpy  # noqa
if qtpy.PYQT5:
    from ErrorReporter import resources_qt5  # noqa
elif qtpy.PYQT4:
    from ErrorReporter import resources_qt4  # noqa
else:
    raise RuntimeError("Unknown QT version: {}".format(qtpy.QT_VERSION))

from qtpy import QtWidgets  # noqa

from ErrorReporter.error_report_presenter import ErrorReporterPresenter  # noqa
from ErrorReporter.errorreport import CrashReportPage  # noqa

# Set path to look for package qt libraries
if command_line_args.qtdir is not None:
    from qtpy.QtCore import QCoreApplication

    QCoreApplication.addLibraryPath(command_line_args.qtdir)


def main():
    if mantid.config['usagereports.enabled'] != '1':
        return int(command_line_args.exit_code)
    app = QtWidgets.QApplication(sys.argv)
    form = CrashReportPage(show_continue_terminate=False)
    presenter = ErrorReporterPresenter(form, command_line_args.exit_code)
    presenter.show_view()
    app.exec_()
    return int(command_line_args.exit_code)


if __name__ == '__main__':  # if we're running file directly and not importing it
    sys.exit(main())  # run the main function
