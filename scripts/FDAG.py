#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from Muon import FFTPresenter #test
from Muon import FFTView #test
from PyQt4.QtGui import *
import sys


class FDAG(QDialog):
    def __init__(self,parent=None):
        super(FDAG,self).__init__(parent)
        view =FFTView.FFTView() 
        self.presenter =FFTPresenter.FFTPresenter(view) #the main ui class in this file is called MainWindow
        self.setLayout(view.grid)
   


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app

app = qapp()
ex= FDAG()
#ex =test.Form() #the main ui class in this file is called MainWindow
ex.resize(700,700)
ex.show()
app.exec_()
