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
from mantid.kernel import ConfigServiceImpl
from Muon.GUI.Common.muon_data_context import MuonDataContext
from save_python import getWidgetIfOpen
from Muon.GUI.MuonAnalysis.load_widget.load_widget import LoadWidget
import Muon.GUI.Common.message_box as message_box
from Muon.GUI.Common.muon_load_data import MuonLoadData


Name = "Muon_Analysis_2"


muonGUI = None
SUPPORTED_FACILITIES = ["ISIS", "SmuS"]


def check_facility():
    """
    Get the currently set facility and check if it is in the list
    of supported facilities, raising an AttributeError if not.
    """
    current_facility = ConfigServiceImpl.Instance().getFacility().name()
    if current_facility not in SUPPORTED_FACILITIES:
        raise AttributeError("Your facility {} is not supported by MuonAnalysis 2.0, so you"
                             "will not be able to load any files. \n \n"
                             "Supported facilities are :"
                             + "\n - ".join(SUPPORTED_FACILITIES))


class MuonAnalysisGui(QtGui.QMainWindow):
    """
    The Muon Analaysis 2.0 interface.
    """

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super(MuonAnalysisGui, self).__init__(parent)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

        try:
            check_facility()
        except AttributeError as error:
            self.warning_popup(error.args[0])

        # initialise the data storing classes of the interface
        self.loaded_data = MuonLoadData()
        self.context = MuonDataContext(load_data=self.loaded_data)

        # construct all the widgets.
        self.load_widget = LoadWidget(self.loaded_data, self.context.instrument, self)

        splitter = QtGui.QSplitter(QtCore.Qt.Vertical)
        splitter.addWidget(self.load_widget.load_widget_view)

        self.setCentralWidget(splitter)
        self.setWindowTitle("Muon Analysis version 2")

    def closeEvent(self, event):
        self.load_widget = None


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app


def main():
    widget = getWidgetIfOpen(Name)
    if widget is not None:
        # if GUI is open bring to front
        widget.raise_()
        return widget
    app = qapp()
    try:
        global muon
        muon = MuonAnalysisGui()
        muon.resize(700, 700)
        muon.show()
        app.exec_()
        return muon
    except RuntimeError as error:
        muon = QtGui.QWidget()
        QtGui.QMessageBox.warning(muon, Name, str(error))
        return muon


def saveToProject():
    widget = getWidgetIfOpen(Name)
    if widget is None:
        return ""
    widget.update()
    project = widget.saveToProject()
    return project


def loadFromProject(project):
    global muonGUI
    muonGUI = main()
    muonGUI.dock_widget.loadFromProject(project)
    muonGUI.loadFromContext(project)
    return muonGUI


if __name__ == '__main__':
    muon = main()
    # cannot assign straight to muonGUI
    # prevents reopening to the same GUI
    muonGUI = muon
