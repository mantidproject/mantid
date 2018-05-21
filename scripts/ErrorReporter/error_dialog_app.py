import sys
import argparse

parser = argparse.ArgumentParser(description='Pass in exit_code')
parser.add_argument('--exitcode', dest='exit_code')
parser.add_argument('--directory', dest='directory')
command_line_args = parser.parse_args()

sys.path.insert(0, command_line_args.directory)

from PyQt4 import QtGui, QtCore
import mantid

from ErrorReporter.error_report_presenter import ErrorReporterPresenter
from ErrorReporter.errorreport import CrashReportPage

def main():
    command_line_args = parser.parse_args()
    app = QtGui.QApplication(sys.argv)
    form = CrashReportPage(show_continue_terminate=False)
    presenter = ErrorReporterPresenter(form, command_line_args.exit_code)
    app.exec_()


if __name__ == '__main__':              # if we're running file directly and not importing it
    main()                              # run the main function