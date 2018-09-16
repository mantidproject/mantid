from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui

from Muon.GUI.Common.dummy.dummy_widget import DummyWidget
from Muon.GUI.Common.dummy_label.dummy_label_widget import DummyLabelWidget
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

from Muon.GUI.Common.muon_context import MuonContext

from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_model import GroupingTableModel
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_view import GroupingTableView
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_presenter import GroupingTablePresenter, MuonGroup

from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_view import PairingTableView
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_presenter import PairingTablePresenter, MuonPair

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
        self.context = context

        self.dockWidget = QtGui.QWidget()

        self.setup_home_tab()
        self.setup_grouping_tab()

        self.dock_view = DockView(self)

        self.dock_view.addDock(self.home_tab_view, "Home")

        self.dock_view.addDock(self.group_tab_view, "Grouping")

        self.dock_view.makeTabs()
        self.dock_view.keepDocksOpen()

        QHbox = QtGui.QHBoxLayout()
        QHbox.addWidget(self.dock_view)

        self.dockWidget.setLayout(QHbox)

    def setup_home_tab(self):
        # self.home_tab_view =  HomePlotWidgetView(self) #DummyWidget("test", self).widget#

        #self.context = MuonContext()

        inst_view = InstrumentWidgetView(self)
        grp_view = HomeGroupingWidgetView(self)
        plot_view = HomePlotWidgetView(self)
        run_info_view = HomeRunInfoWidgetView(self)

        self.instrument_widget = InstrumentWidgetPresenter(inst_view, InstrumentWidgetModel(muon_data=self.context))
        ui2 = HomeGroupingWidgetPresenter(grp_view, HomeGroupingWidgetModel(muon_data=self.context))
        ui3 = HomePlotWidgetPresenter(plot_view, HomePlotWidgetModel())
        ui4 = HomeRunInfoWidgetPresenter(run_info_view, HomeRunInfoWidgetModel(muon_data=self.context))

        self.home_tab_view = HomeTabView(parent=None,
                                         instrument_widget=inst_view,
                                         grouping_widget=grp_view,
                                         plot_widget=plot_view,
                                         run_info_widget=run_info_view)
        tab_model = HomeTabModel(muon_data=self.context)

        self.home_tab_widget = HomeTabPresenter(self.home_tab_view, tab_model,
                                                subwidgets=[self.instrument_widget, ui2, ui3, ui4])

        self.instrument_widget.instrumentNotifier.add_subscriber(self.home_tab_widget.instrumentObserver)

    def setup_grouping_tab(self):
        model = GroupingTabModel()
        grouping_table_view = GroupingTableView()
        ui = GroupingTablePresenter(grouping_table_view, model)
        testgroup1 = MuonGroup(group_name="fwd", detector_IDs=[1, 2, 3, 4, 5])
        testgroup2 = MuonGroup(group_name="bwd", detector_IDs=[6, 7, 8, 9, 10])
        testgroup3 = MuonGroup(group_name="top", detector_IDs=[11, 12, 13, 14, 15])
        ui.add_group(testgroup1)
        ui.add_group(testgroup2)
        ui.add_group(testgroup3)

        pairing_table_view = PairingTableView()
        ui2 = PairingTablePresenter(pairing_table_view, model)
        testpair1 = MuonPair(pair_name="long1", group1_name="fwd", group2_name="bwd")
        testpair2 = MuonPair(pair_name="long2", group1_name="fwd", group2_name="top")
        ui2.add_pair(testpair1)
        ui2.add_pair(testpair2)

        self.group_tab_view = GroupingTabView(grouping_table_view, pairing_table_view)
        self.group_tab_presenter = GroupingTabPresenter(self.group_tab_view, model, ui, ui2)

    def loadFromProject(self, project):
        self.label.updateLabel(project)

    def handleButton(self, message):
        self.label.updateLabel(message)

    @property
    def widget(self):
        return self.dockWidget

    def closeEvent(self, event):
        self.dock_view.closeEvent(event)
