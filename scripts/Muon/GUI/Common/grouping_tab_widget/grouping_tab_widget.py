from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_view import GroupingTableView
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_presenter import GroupingTablePresenter

from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_view import PairingTableView
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_presenter import PairingTablePresenter

from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_presenter import GroupingTabPresenter
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_view import GroupingTabView


class GroupingTabWidget(object):
    def __init__(self, context):

        self.group_tab_model = GroupingTabModel(context)

        self.grouping_table_view = GroupingTableView()
        self.grouping_table_widget = GroupingTablePresenter(self.grouping_table_view, self.group_tab_model)

        self.pairing_table_view = PairingTableView()
        self.pairing_table_widget = PairingTablePresenter(self.pairing_table_view, self.group_tab_model)

        self.group_tab_view = GroupingTabView(self.grouping_table_view, self.pairing_table_view)
        self.group_tab_presenter = GroupingTabPresenter(self.group_tab_view,
                                                        self.group_tab_model,
                                                        self.grouping_table_widget,
                                                        self.pairing_table_widget)
