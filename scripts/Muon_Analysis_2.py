# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

import sys

import PyQt4.QtGui as QtGui
import PyQt4.QtCore as QtCore

import mantid.simpleapi as simpleapi

from Muon.GUI.Common.muon_context import MuonContext

from Muon.GUI.Common.dummy_label.dummy_label_widget import DummyLabelWidget
from Muon.GUI.MuonAnalysis.dock.dock_widget import DockWidget

from Muon.GUI.MuonAnalysis.loadfile.load_file_model import BrowseFileWidgetModel
from Muon.GUI.MuonAnalysis.loadfile.load_file_view import BrowseFileWidgetView
from Muon.GUI.MuonAnalysis.loadfile.load_file_presenter import BrowseFileWidgetPresenter

from Muon.GUI.MuonAnalysis.loadrun.load_run_model import LoadRunWidgetModel
from Muon.GUI.MuonAnalysis.loadrun.load_run_view import LoadRunWidgetView
from Muon.GUI.MuonAnalysis.loadrun.load_run_presenter import LoadRunWidgetPresenter

from Muon.GUI.MuonAnalysis.loadwidget.load_widget_model import LoadWidgetModel
from Muon.GUI.MuonAnalysis.loadwidget.load_widget_view import LoadWidgetView
from Muon.GUI.MuonAnalysis.loadwidget.load_widget_presenter import LoadWidgetPresenter

from Muon.GUI.Common.muon_load_data import MuonLoadData

muonGUI = None


class MuonAnalysis2Gui(QtGui.QMainWindow):

    def __init__(self, parent=None):
        super(MuonAnalysis2Gui, self).__init__(parent)

        self.add_table_workspace()

        self.context = MuonContext()
        self.data = self.context._loaded_data  # MuonLoadData()

        self.setup_load_widget()
        self.dock_widget = DockWidget(self, self.context)
        self.help_widget = DummyLabelWidget("Help dummy", self)

        splitter = QtGui.QSplitter(QtCore.Qt.Vertical)
        splitter.addWidget(self.load_widget_view)
        splitter.addWidget(self.dock_widget.widget)
        splitter.addWidget(self.help_widget.widget)

        self.setCentralWidget(splitter)
        self.setWindowTitle("Muon Analysis version 2")

        # Set up the observer/observable pattern
        self.dock_widget.instrument_widget.instrumentNotifier.add_subscriber(
            self.load_widget.instrumentObserver)
        self.dock_widget.instrument_widget.instrumentNotifier.add_subscriber(
            self.dock_widget.group_tab_presenter.instrumentObserver)

        self.load_widget.loadNotifier.add_subscriber(
            self.dock_widget.home_tab_widget.loadObserver)
        self.load_widget.loadNotifier.add_subscriber(
            self.dock_widget.group_tab_presenter.loadObserver)

        self.dock_widget.group_tab_presenter.groupingNotifier.add_subscriber(
            self.dock_widget.home_tab_widget.groupingObserver)

    def add_table_workspace(self):
        # add dead time tables
        correctTable = simpleapi.CreateEmptyTableWorkspace()
        incorrectTable = simpleapi.CreateEmptyTableWorkspace()

        correctTable.addColumn("int", "spectrum", 0)
        correctTable.addColumn("float", "dead-time", 0)
        for i in range(96):
            correctTable.addRow([i + 1, 0.1])

    def setup_load_widget(self):
        # set up the views
        self.load_file_view = BrowseFileWidgetView(self)
        self.load_run_view = LoadRunWidgetView(self)
        self.load_widget_view = LoadWidgetView(parent=self,
                                               load_file_view=self.load_file_view,
                                               load_run_view=self.load_run_view)
        self.load_widget = LoadWidgetPresenter(self.load_widget_view,
                                               LoadWidgetModel(self.data))

        self.file_widget = BrowseFileWidgetPresenter(self.load_file_view, BrowseFileWidgetModel(self.data))
        self.run_widget = LoadRunWidgetPresenter(self.load_run_view, LoadRunWidgetModel(self.data))

        self.load_widget.set_load_file_widget(self.file_widget)
        self.load_widget.set_load_run_widget(self.run_widget)

    # # cancel algs if window is closed
    # def closeEvent(self, event):
    #     print("MuonAnalysis closeEvent")
    #     self.dock_widget.closeEvent(event)
    #     self.load_widget_view.close()
    #     self.load_run_view.close()
    #     self.load_file_view.close()
    #     global muonGUI
    #     muonGUI = None


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app


def main():
    app = qapp()
    try:
        global muonGUI
        muonGUI = MuonAnalysis2Gui()
        muonGUI.resize(700, 700)
        muonGUI.show()
        app.exec_()
        return muonGUI
    except RuntimeError as error:
        muonGUI = QtGui.QWidget()
        QtGui.QMessageBox.warning(muonGUI, "Muon Analysis version 2", str(error))
        return muonGUI


def saveToProject():
    if muonGUI is None:
        return ""
    project = "test"
    return project


def loadFromProject(project):
    muonGUI = main()
    muonGUI.dock_widget.loadFromProject(project)
    return muonGUI


if __name__ == '__main__':
    muonGUI = main()
