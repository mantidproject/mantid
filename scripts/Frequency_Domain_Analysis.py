#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
import sys

import PyQt4.QtGui as QtGui

from Muon import FFT_presenter
from Muon import FFT_view


class FrequencyDomainAnalysisGui(QtGui.QMainWindow):
    def __init__(self,parent=None):
        super(FrequencyDomainAnalysisGui,self).__init__(parent)
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
ex= FrequencyDomainAnalysisGui()
ex.resize(700,700)
ex.show()
app.exec_()
