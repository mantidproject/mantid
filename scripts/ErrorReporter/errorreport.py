# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, print_function)

import qtpy  # noqa

if qtpy.PYQT5:
    from ErrorReporter import resources_qt5  # noqa
elif qtpy.PYQT4:
    from ErrorReporter import resources_qt4  # noqa
else:
    raise RuntimeError("Unknown QT version: {}".format(qtpy.QT_VERSION))

from qtpy import QtCore, QtGui, QtWidgets # noqa: E402
from qtpy.QtCore import Signal # noqa: E402
from qtpy.QtWidgets import QMessageBox # noqa: E402
from mantidqt.utils.qt import load_ui # noqa: E402

ErrorReportUIBase, ErrorReportUI = load_ui(__file__, 'errorreport.ui')


class CrashReportPage(ErrorReportUIBase, ErrorReportUI):
    action = Signal(bool, int, str, str, str)
    quit_signal = Signal()

    def __init__(self, parent=None, show_continue_terminate=False):
        super(self.__class__, self).__init__(parent)
        self.setupUi(self)
        self.input_text = ""
        if not show_continue_terminate:
            self.continue_terminate_frame.hide()
            self.adjustSize()
        self.setFixedSize(self.width(), self.height())

        self.quit_signal.connect(QtWidgets.QApplication.instance().quit)

        self.icon.setPixmap(QtGui.QPixmap(":/crying_mantid.png"))

        self.requestTextBrowser.anchorClicked.connect(QtGui.QDesktopServices.openUrl)

        self.input_name_line_edit.textChanged.connect(self.set_button_status)
        self.input_email_line_edit.textChanged.connect(self.set_button_status)
        self.input_free_text.textChanged.connect(self.set_button_status)
        self.input_free_text.textChanged.connect(self.set_plain_text_edit_field)

        #  The options on what to do after closing the window (exit/continue)
        self.radioButtonContinue.setChecked(True)  # Set continue to be checked by default

        #  These are the options along the bottom
        self.fullShareButton.clicked.connect(self.fullShare)
        self.nonIDShareButton.clicked.connect(self.nonIDShare)
        self.noShareButton.clicked.connect(self.noShare)

        self.setWindowFlags(QtCore.Qt.CustomizeWindowHint | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowStaysOnTopHint)
        self.setWindowModality(QtCore.Qt.ApplicationModal)

    def quit(self):
        self.quit_signal.emit()

    def fullShare(self):
        self.action.emit(self.continue_working, 0, self.input_name, self.input_email, self.input_text)
        self.close()

    def nonIDShare(self):
        self.action.emit(self.continue_working, 1, self.input_name, self.input_email, self.input_text)
        self.close()

    def noShare(self):
        self.action.emit(self.continue_working, 2, self.input_name, self.input_email, self.input_text)
        self.close()

    def get_simple_line_edit_field(self, expected_type, line_edit):
        gui_element = getattr(self, line_edit)
        value_as_string = gui_element.text()
        return expected_type(value_as_string) if value_as_string else ''

    def set_plain_text_edit_field(self):
        self.input_text = self.get_plain_text_edit_field(text_edit="input_free_text", expected_type=str)

    def get_plain_text_edit_field(self, text_edit, expected_type):
        gui_element = getattr(self, text_edit)
        value_as_string = gui_element.toPlainText()

        return expected_type(value_as_string) if value_as_string else ''

    def set_button_status(self):
        if self.input_text == '' and not self.input_name and not self.input_email:
            self.nonIDShareButton.setEnabled(True)
        else:
            self.nonIDShareButton.setEnabled(False)

    def display_message_box(self, title, message, details):
        msg = QMessageBox(self)
        msg.setIcon(QMessageBox.Warning)
        msg.setText(message)
        msg.setWindowTitle(title)
        msg.setDetailedText(details)
        msg.setStandardButtons(QMessageBox.Ok)
        msg.setDefaultButton(QMessageBox.Ok)
        msg.setEscapeButton(QMessageBox.Ok)
        msg.exec_()

    def set_report_callback(self, callback):
        self.action.connect(callback)

    @property
    def input_name(self):
        return self.get_simple_line_edit_field(line_edit="input_name_line_edit", expected_type=str)

    @property
    def input_email(self):
        return self.get_simple_line_edit_field(line_edit="input_email_line_edit", expected_type=str)

    @property
    def continue_working(self):
        return self.radioButtonContinue.isChecked()
