# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.results_tab_widget.results_tab_presenter import ResultsTabPresenter
from Muon.GUI.Common.results_tab_widget.results_tab_view import ResultsTabView


class ResultsTabWidget(object):
    def __init__(self, context, parent):
        self.results_tab_view = ResultsTabView(parent=parent)

        self.results_tab_presenter = ResultsTabPresenter(context, self.results_tab_view)

        self.results_tab_view.fit_function_combo.currentIndexChanged.connect(self.results_tab_presenter.handle_fit_function_changed)
