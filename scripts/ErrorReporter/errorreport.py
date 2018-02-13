import sys
from PyQt4 import QtGui
import ui_errorreport

class CrashReportPage(QtGui.QWidget, ui_errorreport.Ui_Errorreport):

    def __init__(self, parent=None):
        super(self.__class__, self).__init__(parent)
        self.setupUi(self)

#  The options on what to do after closing the window (exit/continue)
        self.radioButtonContinue.setChecked(True)     # Set continue to be checked by default

#  These are the options along the bottom
        self.fullShareButton.clicked.connect(self.fullShare)
        self.nonIDShareButton.clicked.connect(self.nonIDShare)
        self.noShareButton.clicked.connect(self.noShare)

    def fullShare(self):
        print('Thank you so much!')
        if self.radioButtonContinue.isChecked() == True:
            print('Continuing Mantid')
        else:
            print('Exiting Mantid')
        sys.exit()

    def nonIDShare(self):
        print('No worries, it all helps.')
        if self.radioButtonContinue.isChecked() == True:
            print('Continuing Mantid')
        else:
            print('Exiting Mantid')
        sys.exit()

    def noShare(self):
        print('O.K.')
        if self.radioButtonContinue.isChecked() == True:
            print('Continuing Mantid')
        else:
            print('Exiting Mantid')
        sys.exit()




def main():
    app = QtGui.QApplication(sys.argv)  # A new instance of QApplication
    form = CrashReportPage()            # We set the form to be our ExampleApp (design)
    form.show()                         # Show the form
    app.exec_()                         # and execute the app

if __name__ == '__main__':              # if we're running file directly and not importing it
    main()                              # run the main function
