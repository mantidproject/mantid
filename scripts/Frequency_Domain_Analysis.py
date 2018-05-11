#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

import sys

import PyQt4.QtGui as QtGui

from Muon.transform_widget import TransformWidget
from Muon import load_utils


class FrequencyDomainAnalysisGui(QtGui.QMainWindow):
    def __init__(self,parent=None):
        super(FrequencyDomainAnalysisGui,self).__init__(parent)

        load = load_utils.LoadUtils()
        if not load.MuonAnalysisExists:
            return
        self.transform = TransformWidget(load = load, parent = self)

        self.setCentralWidget(self.transform.widget)
        self.setWindowTitle("Frequency Domain Analysis")

    # cancel algs if window is closed
    def closeEvent(self,event):
        self.presenter.close()


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app


app = qapp()
try:
    ex= FrequencyDomainAnalysisGui()
    ex.resize(700,700)
    ex.show()
    app.exec_()
except RuntimeError as error:
    ex = QtGui.QWidget()
    QtGui.QMessageBox.warning(ex,"Frequency Domain Analysis",str(error))
