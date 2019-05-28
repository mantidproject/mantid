# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.results_tab_widget.results_tab_presenter import ResultsTabPresenter
from Muon.GUI.Common.results_tab_widget.results_tab_view import ResultsTabView
from Muon.GUI.Common.results_tab_widget.results_tab_model import ResultsTabModel


class ResultsTabWidget(object):
    def __init__(self, context, parent):
        self.results_tab_view = ResultsTabView(parent=parent)
        self.results_tab_presenter = ResultsTabPresenter(
            self.results_tab_view, ResultsTabModel(context))
