# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

import sys

import PyQt4.QtGui as QtGui
import PyQt4.QtCore as QtCore

from Muon.GUI.Common.dummy_label.dummy_label_widget import DummyLabelWidget
from Muon.GUI.MuonAnalysis.dock.dock_widget import DockWidget


class MuonAnalysis2Gui(QtGui.QMainWindow):

    def __init__(self, parent=None):
        super(MuonAnalysis2Gui, self).__init__(parent)

        loadWidget = DummyLabelWidget("Load dummy", self)
        self.dockWidget = DockWidget(self)

        helpWidget = DummyLabelWidget("Help dummy", self)

        splitter = QtGui.QSplitter(QtCore.Qt.Vertical)
        splitter.addWidget(loadWidget.widget)
        splitter.addWidget(self.dockWidget.widget)
        splitter.addWidget(helpWidget.widget)

        self.setCentralWidget(splitter)
        self.setWindowTitle("Muon Analysis version 2")

    # cancel algs if window is closed
    def closeEvent(self, event):
        self.dockWidget.closeEvent(event)


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app


app = qapp()
try:
    ex = MuonAnalysis2Gui()
    ex.resize(700, 700)
    ex.show()
    app.exec_()
except RuntimeError as error:
    ex = QtGui.QWidget()
    QtGui.QMessageBox.warning(ex, "Muon Analysis version 2", str(error))
