from __future__ import (absolute_import, division, print_function)
import sys
from PyQt4 import QtGui

from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_model import GroupingTableModel
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_view import GroupingTableView
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_presenter import GroupingTablePresenter, MuonGroup

from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_view import PairingTableView
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_presenter import PairingTablePresenter, MuonPair

from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_presenter import GroupingTabPresenter
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_view import GroupingTabView

from Muon.GUI.Common.muon_context import MuonContext

if __name__ == "__main__":
    app = QtGui.QApplication(sys.argv)

    context = MuonContext()

    model = GroupingTabModel(data=context)
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

    tabView = GroupingTabView(grouping_table_view, pairing_table_view)
    tabPresenter = GroupingTabPresenter(tabView, model, ui, ui2)


    # ## Grouping table only
    # context = MuonContext()
    # model = GroupingTabModel(data=context)
    # grouping_table_view = GroupingTableView()
    # ui = GroupingTablePresenter(grouping_table_view, model)
    # testgroup1 = MuonGroup(group_name="fwd", detector_IDs=[1, 2, 3, 4, 5])
    # testgroup2 = MuonGroup(group_name="bwd", detector_IDs=[6, 7, 8, 9, 10])
    # testgroup3 = MuonGroup(group_name="top", detector_IDs=[11, 12, 13, 14, 15])
    # ui.add_group(testgroup1)
    # ui.add_group(testgroup2)
    # ui.add_group(testgroup3)
    #
    ## Pairing table only
    # model = GroupingTabModel(data=context)
    # pairing_table_view = PairingTableView()
    # ui2 = PairingTablePresenter(pairing_table_view, model)
    # testpair1 = MuonPair(pair_name="long1", group1_name="fwd", group2_name="bwd")
    # testpair2 = MuonPair(pair_name="long2", group1_name="fwd", group2_name="top")
    # ui2.add_pair(testpair1)
    # ui2.add_pair(testpair2)

    # ui2.show()
    # ui2.show()
    tabPresenter.show()
    sys.exit(app.exec_())
