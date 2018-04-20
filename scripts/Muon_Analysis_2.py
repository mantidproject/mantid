#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

import sys

import PyQt4.QtGui as QtGui

from Muon import dummy_label_widget 
from Muon import dock_widget


class MuonAnalysis2Gui(QtGui.QMainWindow):
    def __init__(self,parent=None):
        super(MuonAnalysis2Gui,self).__init__(parent)
        
        loadWidget = dummy_label_widget.DummyLabelWidget("Load dummy",self)
        self.dockWidget = dock_widget.DockWidget(self)

        helpWidget = dummy_label_widget.DummyLabelWidget("Help dummy",self)

        splitter = QtGui.QSplitter(QtCore.Qt.Vertical)
        splitter.addWidget(loadWidget.getWidget())
        splitter.addWidget(self.dockWidget.getWidget())
        splitter.addWidget(helpWidget.getWidget())

        self.setCentralWidget(splitter)
        self.setWindowTitle("Muon Analysis version 2")

    # cancel algs if window is closed
    def closeEvent(self,event):
        self.dockWidget.closeEvent(event)


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app


app = qapp()
try:
    ex= MuonAnalysis2Gui()
    ex.resize(700,700)
    ex.show()
    app.exec_()
except RuntimeError as error:
    ex = QtGui.QWidget()
    QtGui.QMessageBox.warning(ex,"Muon Analysis version 2",str(error))
