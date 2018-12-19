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
from Muon.GUI.Common.dummy_label.dummy_label_widget import DummyLabelWidget
from Muon.GUI.MuonAnalysis.dock.dock_widget import DockWidget
from Muon.GUI.Common.muon_context.muon_context import *  # MuonContext
from save_python import getWidgetIfOpen
from Muon.GUI.MuonAnalysis.load_widget.load_widget import LoadWidget
import Muon.GUI.Common.message_box as message_box
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget import GroupingTabWidget


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


class MuonAnalysis4Gui(QtGui.QMainWindow):
    """
    The Muon Analaysis 2.0 interface.
    """

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super(MuonAnalysis4Gui, self).__init__(parent)
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
        self.grouping_tab_widget = GroupingTabWidget(self.context)

        splitter = QtGui.QSplitter(QtCore.Qt.Vertical)
        splitter.addWidget(self.load_widget.load_widget_view)
        splitter.addWidget(self.grouping_tab_widget.group_tab_view)

        self.setCentralWidget(splitter)
        self.setWindowTitle("Muon Analysis version 2")

        self.load_widget.load_widget.loadNotifier.add_subscriber(self.grouping_tab_widget.group_tab_presenter.loadObserver)

    def closeEvent(self, event):
        print("Muon Analysis Close Event")
        self.load_widget.load_widget_view = None
        self.load_widget.load_run_view = None
        self.load_widget.load_file_view = None


class MuonAnalysis2Gui(QtGui.QMainWindow):

    def __init__(self, parent=None):
        super(MuonAnalysis2Gui, self).__init__(parent)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

        self.add_table_workspace()

        self._context = MuonContext(Name)

        self.loadWidget = DummyLabelWidget(self._context, LoadText, self)
        self.dockWidget = DockWidget(self._context, self)

        self.helpWidget = DummyLabelWidget(self._context, HelpText, self)

        splitter = QtGui.QSplitter(QtCore.Qt.Vertical)
        splitter.addWidget(self.loadWidget.widget)
        splitter.addWidget(self.dockWidget.widget)
        splitter.addWidget(self.helpWidget.widget)

        self.setCentralWidget(splitter)
        self.setWindowTitle(Name)

        self.dockWidget.setUpdateContext(self.update)

    def saveToProject(self):
        return self._context.save()

    def update(self):
        # update load
        self.loadWidget.updateContext()
        self.dockWidget.updateContext()
        self.helpWidget.updateContext()

        self.dockWidget.loadFromContext()

    def loadFromContext(self, project):
        self._context.loadFromProject(project)
        self.loadWidget.loadFromContext()
        self.dockWidget.loadFromContext()
        self.helpWidget.loadFromContext()

    # cancel algs if window is closed
    def closeEvent(self, event):
        self.dockWidget.closeEvent(event)
        global muonGUI
        muonGUI.deleteLater()
        muonGUI = None


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
        muon = MuonAnalysis4Gui()
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
    # muonGUI.loadFromContext(project)
    return muonGUI


if __name__ == '__main__':
    muon = main()
    # cannot assign straight to muonGUI
    # prevents reopening to the same GUI
    muonGUI = muon
