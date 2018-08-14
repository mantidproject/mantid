from PyQt4 import QtGui, QtCore
import ui_errorreport
from PyQt4.QtCore import pyqtSignal
from mantidqtpython import MantidQt


class CrashReportPage(QtGui.QWidget, ui_errorreport.Ui_Errorreport):
    action = pyqtSignal(bool, int, str, str, str)
    quit_signal = pyqtSignal()

    def __init__(self, parent=None, show_continue_terminate=False):
        super(self.__class__, self).__init__(parent)
        self.setupUi(self)
        if not show_continue_terminate:
            self.continue_terminate_frame.hide()
            self.adjustSize()
        self.setFixedSize(self.width(), self.height())

        self.quit_signal.connect(QtGui.QApplication.instance().quit)

        #self.icon.setPixmap(QtGui.QPixmap(":/crying_mantid.png"))

        self.requestTextBrowser.anchorClicked.connect(MantidQt.API.MantidDesktopServices.openUrl)

        self.input_name_line_edit.textChanged.connect(self.set_button_status)
        self.input_email_line_edit.textChanged.connect(self.set_button_status)
        self.input_free_text.textChanged.connect(self.set_button_status)

#  The options on what to do after closing the window (exit/continue)
        self.radioButtonContinue.setChecked(True)     # Set continue to be checked by default

#  These are the options along the bottom
        self.fullShareButton.clicked.connect(self.fullShare)
        self.nonIDShareButton.clicked.connect(self.nonIDShare)
        self.noShareButton.clicked.connect(self.noShare)

        self.setWindowFlags(QtCore.Qt.CustomizeWindowHint | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowStaysOnTopHint)
        self.setWindowModality(QtCore.Qt.ApplicationModal)

    def quit (self):
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

    def get_plain_text_edit_field(self, expected_type, text_edit):
        gui_element = getattr(self, text_edit)
        value_as_string = gui_element.toPlainText()
        return expected_type(value_as_string) if value_as_string else ''

    def set_button_status(self):
        if not self.input_name and not self.input_email:
            self.nonIDShareButton.setEnabled(True)
        else:
            self.nonIDShareButton.setEnabled(False)

    @property
    def input_name(self):
        return self.get_simple_line_edit_field(line_edit="input_name_line_edit", expected_type=str)

    @property
    def input_email(self):
        return self.get_simple_line_edit_field(line_edit="input_email_line_edit", expected_type=str)

    @property
    def input_text(self):
        return self.get_plain_text_edit_field(text_edit="input_free_text", expected_type=str)

    @property
    def continue_working(self):
        return self.radioButtonContinue.isChecked()
