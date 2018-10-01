# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

import sys

import PyQt4.QtGui as QtGui
import PyQt4.QtCore as QtCore

from mantid.kernel import ConfigServiceImpl
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

from Muon.GUI.Common.help_widget.help_widget_model import HelpWidgetModel
from Muon.GUI.Common.help_widget.help_widget_view import HelpWidgetView
from Muon.GUI.Common.help_widget.help_widget_presenter import HelpWidgetPresenter

import Muon.GUI.Common.message_box as message_box
from Muon.GUI.Common.muon_load_data import MuonLoadData

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
        self.context = MuonContext(load_data=self.loaded_data)

        # construct all the widgets.
        self.setup_load_widget()
        self.setup_help_widget()
        self.setup_home_tab()
        self.setup_grouping_tab()
        # set up the tabbing structure
        self.setup_tabs()

        splitter = QtGui.QSplitter(QtCore.Qt.Vertical)
        splitter.addWidget(self.load_widget_view)
        splitter.addWidget(self.tabs)
        splitter.addWidget(self.help_widget)
        self.setCentralWidget(splitter)
        self.setWindowTitle("Muon Analysis version 2")

        # Set up observer/observables
        #   - Home tab notifies if instrument changes
        #   - Home tab notifies if user changes alpha for a pair
        self.group_widget.pairAlphaNotifier.add_subscriber(self.group_tab_presenter.loadObserver)
        self.group_tab_presenter.groupingNotifier.add_subscriber(self.home_tab_widget.groupingObserver)
        self.instrument_widget.instrumentNotifier.add_subscriber(self.home_tab_widget.instrumentObserver)
        self.instrument_widget.instrumentNotifier.add_subscriber(self.load_widget.instrumentObserver)
        self.instrument_widget.instrumentNotifier.add_subscriber(self.group_tab_presenter.instrumentObserver)
        self.load_widget.loadNotifier.add_subscriber(self.home_tab_widget.loadObserver)
        self.load_widget.loadNotifier.add_subscriber(self.group_tab_presenter.loadObserver)

    def setup_load_widget(self):
        # set up the views
        self.load_file_view = BrowseFileWidgetView(self)
        self.load_run_view = LoadRunWidgetView(self)
        self.load_widget_view = LoadWidgetView(parent=self,
                                               load_file_view=self.load_file_view,
                                               load_run_view=self.load_run_view)
        self.load_widget = LoadWidgetPresenter(self.load_widget_view,
                                               LoadWidgetModel(self.loaded_data))

        self.file_widget = BrowseFileWidgetPresenter(self.load_file_view, BrowseFileWidgetModel(self.loaded_data))
        self.run_widget = LoadRunWidgetPresenter(self.load_run_view, LoadRunWidgetModel(self.loaded_data))

        self.load_widget.set_load_file_widget(self.file_widget)
        self.load_widget.set_load_run_widget(self.run_widget)

        self.load_widget.set_current_instrument(self.context.instrument)

    def setup_help_widget(self):
        self.help_widget_model = HelpWidgetModel()
        self.help_widget = HelpWidgetView()
        self.help_widget_presenter = HelpWidgetPresenter(self.help_widget, self.help_widget_model)

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

    def setup_home_tab(self):
        self.inst_view = InstrumentWidgetView()
        self.grp_view = HomeGroupingWidgetView()
        self.plot_view = HomePlotWidgetView()
        self.run_info_view = HomeRunInfoWidgetView()

        # keep a handle to the presenters of sub-widgets
        self.instrument_widget = InstrumentWidgetPresenter(self.inst_view,
                                                           InstrumentWidgetModel(muon_data=self.context))
        self.group_widget = HomeGroupingWidgetPresenter(self.grp_view, HomeGroupingWidgetModel(muon_data=self.context))
        self.plot_widget = HomePlotWidgetPresenter(self.plot_view, HomePlotWidgetModel())
        self.run_info_widget = HomeRunInfoWidgetPresenter(self.run_info_view,
                                                          HomeRunInfoWidgetModel(muon_data=self.context))

        self.home_tab_view = HomeTabView(parent=None,
                                         widget_list=[self.inst_view,
                                                      self.grp_view,
                                                      self.plot_view,
                                                      self.run_info_view])
        self.home_tab_model = HomeTabModel(muon_data=self.context)
        self.home_tab_widget = HomeTabPresenter(self.home_tab_view, self.home_tab_model,
                                                subwidgets=[self.instrument_widget,
                                                            self.group_widget,
                                                            self.plot_widget,
                                                            self.run_info_widget])

    def setup_tabs(self):
        """
        Set up the tabbing structure; the tabs work similarly to conventional
        web browsers.
        """
        self.tabs = DetachableTabWidget(self)
        self.tabs.addTab(self.home_tab_view, 'Home')
        self.tabs.addTab(self.group_tab_view, 'Grouping')

    def closeEvent(self, event):
        print("Muon Analysis Close Event")
        self.tabs.closeEvent(event)
        self.load_widget_view = None
        self.load_run_view = None
        self.load_file_view = None

        self.inst_view = None
        self.grp_view = None
        self.plot_view = None
        self.run_info_view = None
        self.home_tab_view = None


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
        muonGUI = MuonAnalysis4Gui()
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
