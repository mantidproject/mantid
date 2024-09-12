# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view import EAGroupingTableView
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_presenter import EAGroupingTablePresenter

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model import EAGroupingTabModel

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_widget_presenter import EAGroupingTabPresenter
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_view import EAGroupingTabView


class EAGroupingTabWidget(object):
    def __init__(self, context):
        self.group_tab_model = EAGroupingTabModel(context)

        self.grouping_table_view = EAGroupingTableView()
        self.grouping_table_widget = EAGroupingTablePresenter(self.grouping_table_view, self.group_tab_model)

        self.group_tab_view = EAGroupingTabView(self.grouping_table_view)
        self.group_tab_presenter = EAGroupingTabPresenter(self.group_tab_view, self.group_tab_model, self.grouping_table_widget)
