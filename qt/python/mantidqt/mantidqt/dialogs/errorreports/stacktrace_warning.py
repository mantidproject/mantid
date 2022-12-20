# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtCore
from mantidqt.utils.qt import load_ui

stacktraceWarningUIBase, stacktraceWarningUI = load_ui(__file__, 'stacktrace_warning.ui')


class StacktraceWarning(stacktraceWarningUIBase, stacktraceWarningUI):

    def __int__(self, parent=None):
        super(self.__class__, self).__init__(parent)
        self.setupUi(self)
        self.pushButtonOk.clicked.connect(self.hide)
        self.setWindowFlags(QtCore.Qt.Dialog | QtCore.Qt.Window | QtCore.Qt.WindowTitleHint
                            | QtCore.Qt.CustomizeWindowHint)

    def set_stacktrace_text(self, stacktrace_text):
        self.stacktraceText.setText(stacktrace_text)
