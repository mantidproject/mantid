#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from Muon import FFT_presenter
from Muon import FFT_view
from PyQt4.QtGui import *
import sys


class FDAG(QMainWindow):
    def __init__(self,parent=None):
        super(FDAG,self).__init__(parent)
        view =FFT_view.FFTView(self)
        self.presenter =FFT_presenter.FFTPresenter(view) #the main ui class in this file is called MainWindow
        self.setCentralWidget(view)
        self.setWindowTitle("Frequency Domain Analysis")


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app

app = qapp()
ex= FDAG()
ex.resize(700,700)
ex.show()
app.exec_()
