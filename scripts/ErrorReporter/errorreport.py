import sys
from PyQt4 import QtGui, QtCore
import ui_errorreport
from PyQt4.QtCore import pyqtSignal
from mantidqtpython import MantidQt


class CrashReportPage(QtGui.QWidget, ui_errorreport.Ui_Errorreport):
    action = pyqtSignal(bool, int, str, str)

    def __init__(self, parent=None):
        super(self.__class__, self).__init__(parent)
        self.setupUi(self)
        self.setFixedSize(self.width(), self.height())

        self.action.connect(QtGui.QApplication.instance().errorHandling)

        self.icon.setPixmap(QtGui.QPixmap(":/crying_mantid.png"))

        self.requestTextBrowser.anchorClicked.connect(MantidQt.API.MantidDesktopServices.openUrl)

        self.input_name_line_edit.textChanged.connect(self.set_button_status)
        self.input_email_line_edit.textChanged.connect(self.set_button_status)

#  The options on what to do after closing the window (exit/continue)
        self.radioButtonContinue.setChecked(True)     # Set continue to be checked by default

#  These are the options along the bottom
        self.fullShareButton.clicked.connect(self.fullShare)
        self.nonIDShareButton.clicked.connect(self.nonIDShare)
        self.noShareButton.clicked.connect(self.noShare)

        self.setWindowFlags(QtCore.Qt.CustomizeWindowHint | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowStaysOnTopHint)
        self.setWindowModality(QtCore.Qt.ApplicationModal)

    def fullShare(self):
        self.action.emit(self.continue_working, 0, self.input_name, self.input_email)
        self.close()

    def nonIDShare(self):
        self.action.emit(self.continue_working, 1, self.input_name, self.input_email)
        self.close()

    def noShare(self):
        self.action.emit(self.continue_working, 2, self.input_name, self.input_email)
        self.close()

    def get_simple_line_edit_field(self, expected_type, line_edit):
        gui_element = getattr(self, line_edit)
        value_as_string = gui_element.text()
        return expected_type(value_as_string) if value_as_string else None

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
    def continue_working(self):
        return self.radioButtonContinue.isChecked()


def main():
    app = QtGui.QApplication(sys.argv)  # A new instance of QApplication
    form = CrashReportPage()            # We set the form to be our ExampleApp (design)
    form.show()                         # Show the form
    app.exec_()                         # and execute the app

if __name__ == '__main__':              # if we're running file directly and not importing it
    main()                              # run the main function
