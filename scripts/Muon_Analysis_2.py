# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

import sys

import PyQt4.QtGui as QtGui
import PyQt4.QtCore as QtCore

import mantid.simpleapi as simpleapi

from Muon.GUI.Common.muon_context import MuonContext

from Muon.GUI.Common.dummy_label.dummy_label_widget import DummyLabelWidget
from Muon.GUI.MuonAnalysis.dock.dock_widget import DockWidget

from Muon.GUI.Common.dock.dockable_tabs import DetachableTabWidget

from Muon.GUI.MuonAnalysis.loadfile.load_file_model import BrowseFileWidgetModel
from Muon.GUI.MuonAnalysis.loadfile.load_file_view import BrowseFileWidgetView
from Muon.GUI.MuonAnalysis.loadfile.load_file_presenter import BrowseFileWidgetPresenter

from Muon.GUI.MuonAnalysis.loadrun.load_run_model import LoadRunWidgetModel
from Muon.GUI.MuonAnalysis.loadrun.load_run_view import LoadRunWidgetView
from Muon.GUI.MuonAnalysis.loadrun.load_run_presenter import LoadRunWidgetPresenter

from Muon.GUI.MuonAnalysis.loadwidget.load_widget_model import LoadWidgetModel
from Muon.GUI.MuonAnalysis.loadwidget.load_widget_view import LoadWidgetView
from Muon.GUI.MuonAnalysis.loadwidget.load_widget_presenter import LoadWidgetPresenter

from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_model import InstrumentWidgetModel
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_view import InstrumentWidgetView
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_presenter import InstrumentWidgetPresenter

from Muon.GUI.Common.home_grouping_widget.home_grouping_widget_model import HomeGroupingWidgetModel
from Muon.GUI.Common.home_grouping_widget.home_grouping_widget_view import HomeGroupingWidgetView
from Muon.GUI.Common.home_grouping_widget.home_grouping_widget_presenter import HomeGroupingWidgetPresenter

from Muon.GUI.Common.home_plot_widget.home_plot_widget_model import HomePlotWidgetModel
from Muon.GUI.Common.home_plot_widget.home_plot_widget_view import HomePlotWidgetView
from Muon.GUI.Common.home_plot_widget.home_plot_widget_presenter import HomePlotWidgetPresenter

from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_model import HomeRunInfoWidgetModel
from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_view import HomeRunInfoWidgetView
from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_presenter import HomeRunInfoWidgetPresenter

from Muon.GUI.Common.home_tab.home_tab_model import HomeTabModel
from Muon.GUI.Common.home_tab.home_tab_view import HomeTabView
from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabPresenter

from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_view import GroupingTableView
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_presenter import GroupingTablePresenter

from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_view import PairingTableView
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_presenter import PairingTablePresenter

from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_presenter import GroupingTabPresenter
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_view import GroupingTabView

from Muon.GUI.Common.muon_load_data import MuonLoadData

muonGUI = None


class MuonAnalysis3Gui(QtGui.QMainWindow):

    def __init__(self, parent=None):
        super(MuonAnalysis3Gui, self).__init__(parent)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

        self.add_table_workspace()

        self.context = MuonContext()
        self.data = self.context._loaded_data

        self.setup_load_widget()
        self.tabs = self.setup_tabs()
        self.help_widget = DummyLabelWidget("Help dummy", self)

        splitter = QtGui.QSplitter(QtCore.Qt.Vertical)
        splitter.addWidget(self.load_widget_view)
        splitter.addWidget(self.tabs)
        splitter.addWidget(self.help_widget.widget)
        splitter.setCollapsible(0, False)
        splitter.setCollapsible(1, False)
        splitter.setCollapsible(2, False)

        self.setCentralWidget(splitter)
        self.setWindowTitle("Muon Analysis version 2")

        self.instrument_widget.instrumentNotifier.add_subscriber(self.load_widget.instrumentObserver)
        self.instrument_widget.instrumentNotifier.add_subscriber(self.group_tab_presenter.instrumentObserver)
        self.load_widget.loadNotifier.add_subscriber(self.home_tab_widget.loadObserver)
        self.load_widget.loadNotifier.add_subscriber(self.group_tab_presenter.loadObserver)
        self.group_tab_presenter.groupingNotifier.add_subscriber(self.home_tab_widget.groupingObserver)

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

    def focusInEvent(self, event):
        self.setFocus()
        self.raise_()
        self.isActiveWindow()

    def setup_home_tab(self):
        inst_view = InstrumentWidgetView(self)
        grp_view = HomeGroupingWidgetView(self)
        plot_view = HomePlotWidgetView(self)
        run_info_view = HomeRunInfoWidgetView(self)

        # keep a handle to the presenters of sub-widgets
        self.instrument_widget = InstrumentWidgetPresenter(inst_view, InstrumentWidgetModel(muon_data=self.context))
        self.group_widget = HomeGroupingWidgetPresenter(grp_view, HomeGroupingWidgetModel(muon_data=self.context))
        self.plot_widget = HomePlotWidgetPresenter(plot_view, HomePlotWidgetModel())
        self.run_info_widget = HomeRunInfoWidgetPresenter(run_info_view, HomeRunInfoWidgetModel(muon_data=self.context))

        self.home_tab_view = HomeTabView(parent=None,
                                         instrument_widget=inst_view,
                                         grouping_widget=grp_view,
                                         plot_widget=plot_view,
                                         run_info_widget=run_info_view)
        self.home_tab_model = HomeTabModel(muon_data=self.context)
        self.home_tab_widget = HomeTabPresenter(self.home_tab_view, self.home_tab_model,
                                                subwidgets=[self.instrument_widget,
                                                            self.group_widget,
                                                            self.plot_widget,
                                                            self.run_info_widget])

        # Set up observer/observables
        #   - Home tab notifies if instrument changes
        #   - Home tab notifies if user changes alpha for a pair
        self.instrument_widget.instrumentNotifier.add_subscriber(self.home_tab_widget.instrumentObserver)

    def setup_grouping_tab(self):
        # Share a single model between the sub-widgets
        self.group_tab_model = GroupingTabModel(self.context)

        self.grouping_table_view = GroupingTableView()
        self.grouping_table_widget = GroupingTablePresenter(self.grouping_table_view, self.group_tab_model)

        self.pairing_table_view = PairingTableView()
        self.pairing_table_widget = PairingTablePresenter(self.pairing_table_view, self.group_tab_model)

        self.group_tab_view = GroupingTabView(self.grouping_table_view, self.pairing_table_view)
        self.group_tab_presenter = GroupingTabPresenter(self.group_tab_view,
                                                        self.group_tab_model,
                                                        self.grouping_table_widget,
                                                        self.pairing_table_widget)

    def setup_tabs(self):
        self.setup_home_tab()
        self.setup_grouping_tab()
        self.group_widget.pairAlphaNotifier.add_subscriber(self.group_tab_presenter.loadObserver)
        self.group_tab_presenter.groupingNotifier.add_subscriber(self.home_tab_widget.groupingObserver)

        tabWidget = DetachableTabWidget(self)

        tab1 = QtGui.QLabel('Test Widget 1')
        tabWidget.addTab(self.home_tab_view, 'Home')

        tab2 = QtGui.QLabel('Test Widget 2')
        tabWidget.addTab(self.group_tab_view, 'Grouping')

        self.setCentralWidget(tabWidget)
        return tabWidget

    def closeEvent(self, event):
        self.tabs.closeEvent(event)
        self.load_widget_view.close()
        self.load_run_view.close()
        self.load_file_view.close()


class MuonAnalysis2Gui(QtGui.QMainWindow):

    def __init__(self, parent=None):
        super(MuonAnalysis2Gui, self).__init__(parent)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

        self.add_table_workspace()

        self.context = MuonContext()
        self.data = self.context._loaded_data

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

    # ------------------------------------------------------------------------------------------------------------------
    # EXTRAS
    # ------------------------------------------------------------------------------------------------------------------

    def focusInEvent(self, event):
        print("Muon analysis has focus")
        self.setFocus()
        self.raise_()
        self.isActiveWindow()


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
        muonGUI = MuonAnalysis3Gui()
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
