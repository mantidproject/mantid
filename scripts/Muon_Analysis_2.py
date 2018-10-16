# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

import sys

import PyQt4.QtGui as QtGui
import PyQt4.QtCore as QtCore

from Muon.GUI.Common.dummy_label.dummy_label_widget import DummyLabelWidget
from Muon.GUI.MuonAnalysis.dock.dock_widget import DockWidget
from Muon.GUI.Common.muon_context.muon_context import *#MuonContext


muonGUI = None

Name = "Muon Analysis 2"
class MuonAnalysis2Gui(QtGui.QMainWindow):

    def __init__(self, parent=None):
        super(MuonAnalysis2Gui, self).__init__(parent)

        self._context = MuonContext()

        self.loadWidget = DummyLabelWidget(self._context ,LoadText, self)
        self.dockWidget = DockWidget(self._context,self)

        self.helpWidget = DummyLabelWidget(self._context,HelpText, self)

        splitter = QtGui.QSplitter(QtCore.Qt.Vertical)
        splitter.addWidget(self.loadWidget.widget)
        splitter.addWidget(self.dockWidget.widget)
        splitter.addWidget(self.helpWidget.widget)

        self.setCentralWidget(splitter)
        self.setWindowTitle("Muon Analysis version 2")

        self.dockWidget.setUpdateContext(self.update)

    def saveToProject(self):
        return self._context.save()

    def update(self):
        # update load
        self.loadWidget.updateContext()
        self.dockWidget.updateContext()
        self.helpWidget.updateContext()

        self._context.printContext()
        self.dockWidget.loadFromContext(self._context)

    # cancel algs if window is closed
    def closeEvent(self, event):
        self.dockWidget.closeEvent(event)
        global muonGUI
        muonGUI = None


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app


def main():
    app = qapp()
    try:
        muon = MuonAnalysis2Gui()
        muon.resize(700, 700)
        muon.setProperty("launcher", "Muon_Analysis_2")
        muon.show()
        muon.setAccessibleName(Name)
        app.exec_()
        return muon
    except RuntimeError as error:
        muon = QtGui.QWidget()
        QtGui.QMessageBox.warning(muon, "Muon Analysis version 2", str(error))
        return muon


def saveToProject():
    allWidgets = QtGui.QApplication.allWidgets()
    for widget in allWidgets:
       if widget.accessibleName() == Name:
            widget.update()
            project = widget.saveToProject()#_context.save()
            return project
    return ""


def loadFromProject(project):
    global muonGUI
    muonGUI = main()
    muonGUI._context.loadFromProject(project)
    muonGUI.update()
    return muonGUI

if __name__ == '__main__':
    global muonGUI
    muonGUI = main()
