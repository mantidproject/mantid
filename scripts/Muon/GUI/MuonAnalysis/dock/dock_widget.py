from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui

from Muon.GUI.Common.dock.dock_view import DockView

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


class DockWidget(QtGui.QWidget):
    """
    This is a special case of the widget class structure.
    Normally we would only store the presenter and would
    get the view via the presenter. However, the docks
    have no logic and therefore do not have a presenter.
    So this class simply wraps the dock (view) and
    populates it
    """

    def __init__(self, parent=None, context=None):
        super(DockWidget, self).__init__(parent)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

        # declare sub-widgets for home tab
        self.instrument_widget = None
        self.group_widget = None
        self.plot_widget = None
        self.run_info_widget = None
        # home tab MVP
        self.home_tab_model = None
        self.home_tab_widget = None
        self.home_tab_view = None
        # Grouping table
        self.grouping_table_view = None
        self.grouping_table_widget = None
        # Pairing table
        self.pairing_table_view = None
        self.pairing_table_widget = None
        # Grouping tab
        self.group_tab_model = None
        self.group_tab_presenter = None
        self.group_tab_view = None

        # The context is passed in as a dependency
        self.context = context

        self.dockWidget = QtGui.QWidget()


        self.setup_home_tab()
        self.setup_grouping_tab()

        self.group_widget.pairAlphaNotifier.add_subscriber(self.group_tab_presenter.loadObserver)
        self.group_tab_presenter.groupingNotifier.add_subscriber(self.home_tab_widget.groupingObserver)

        self.dock_view = DockView(self)
        self.dock_view.addDock(self.home_tab_view, "Home")
        self.dock_view.addDock(self.group_tab_view, "Grouping")

        self.dock_view.makeTabs()
        self.dock_view.keepDocksOpen()

        layout = QtGui.QHBoxLayout()
        layout.addWidget(self.dock_view)
        self.dockWidget.setLayout(layout)

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



    @property
    def widget(self):
        return self.dockWidget

    def loadFromProject(self, project):
        self.label.updateLabel(project)

    def handleButton(self, message):
        self.label.updateLabel(message)

    def closeEvent(self, event):
        self.dock_view.closeEvent(event)



    def focusInEvent(self, event):
        print("DockWidget Has focus")
        self.raise_()

    def focusOutEvent(self, event):
        print("DockWidget Loses focus")
        self.lower()

from Muon.GUI.Common.dock.dockable_tabs import DetachableTabWidget

class DetachableDockWidget(DetachableTabWidget):

    def __init__(self, parent=None):
        super(DetachableDockWidget, self).__init__(parent)

