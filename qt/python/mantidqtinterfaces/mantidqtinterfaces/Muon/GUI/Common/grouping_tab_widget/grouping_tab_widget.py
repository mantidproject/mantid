# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.grouping_table_widget.grouping_table_widget_view import GroupingTableView
from mantidqtinterfaces.Muon.GUI.Common.grouping_table_widget.grouping_table_widget_presenter import GroupingTablePresenter

from mantidqtinterfaces.Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from mantidqtinterfaces.Muon.GUI.Common.pairing_table_widget.pairing_table_widget_view import PairingTableView
from mantidqtinterfaces.Muon.GUI.Common.pairing_table_widget.pairing_table_widget_presenter import PairingTablePresenter

from mantidqtinterfaces.Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_presenter import GroupingTabPresenter
from mantidqtinterfaces.Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_view import GroupingTabView

from mantidqtinterfaces.Muon.GUI.Common.difference_table_widget.difference_widget_presenter import DifferencePresenter


class GroupingTabWidget(object):
    def __init__(self, context, parent=None):
        self.group_tab_model = GroupingTabModel(context)

        self.grouping_table_view = GroupingTableView()
        self.grouping_table_widget = GroupingTablePresenter(self.grouping_table_view, self.group_tab_model)

        self.diff_table = DifferencePresenter(self.group_tab_model)

        self.pairing_table_view = PairingTableView()
        self.pairing_table_widget = PairingTablePresenter(self.pairing_table_view, self.group_tab_model)

        self.group_tab_view = GroupingTabView(self.grouping_table_view, self.pairing_table_view, self.diff_table.view)
        self.group_tab_presenter = GroupingTabPresenter(
            self.group_tab_view, self.group_tab_model, self.grouping_table_widget, self.pairing_table_widget, self.diff_table, parent
        )

        context.update_view_from_model_notifier.add_subscriber(self.group_tab_presenter.update_view_from_model_observer)
