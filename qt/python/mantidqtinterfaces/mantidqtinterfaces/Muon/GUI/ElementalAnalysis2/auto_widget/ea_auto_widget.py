# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_view import EAAutoTabView
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_presenter import EAAutoTabPresenter
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model import EAAutoTabModel
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.auto_widget.ea_match_table_view import EAMatchTableView
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.auto_widget.ea_match_table_presenter import EAMatchTablePresenter


class EAAutoTabWidget(object):

    def __init__(self,context):
        self.match_table_view = EAMatchTableView()
        self.match_table_presenter = EAMatchTablePresenter(self.match_table_view)
        self.auto_tab_view = EAAutoTabView(self.match_table_view)
        self.auto_tab_model = EAAutoTabModel(context)
        self.auto_tab_presenter = EAAutoTabPresenter(context, self.auto_tab_view , self.auto_tab_model,
                                                     self.match_table_presenter)
