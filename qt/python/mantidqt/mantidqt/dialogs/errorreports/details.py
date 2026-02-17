# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtCore
from mantidqt.utils.qt import load_ui

DEFAULT_STACKTRACE_TEXT = "Stacktrace unavailable"

moreDetailsUIBase, moreDetailsUI = load_ui(__file__, "moredetails.ui")


class MoreDetailsDialog(moreDetailsUIBase, moreDetailsUI):
    def __init__(self, parent=None):
        super(self.__class__, self).__init__(parent)
        self.setupUi(self)
        self.pushButtonOk.clicked.connect(self.hide)
        self.setWindowFlags(QtCore.Qt.Dialog | QtCore.Qt.Window | QtCore.Qt.WindowTitleHint | QtCore.Qt.CustomizeWindowHint)

    def set_user_text(self, mantid_user_text):
        self.user_text.setText(mantid_user_text)

    def set_stacktrace_text(self, text):
        text = text or DEFAULT_STACKTRACE_TEXT
        self.stacktrace_text.setText(text)
