# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division)

from Muon.GUI.Common.results_tab_widget.results_tab_presenter import ResultsTabPresenter
from Muon.GUI.Common.results_tab_widget.results_tab_view import ResultsTabView
from Muon.GUI.Common.results_tab_widget.results_tab_model import ResultsTabModel


class ResultsTabWidget(object):
    """Factory object to wire together components of the results tab"""

    def __init__(self, fit_context, parent):
        """
        Initialize the widget.
        :param fit_context: A reference to the a FitContext object used to store fit results
        :param parent: A parent widget for the view
        """
        self.results_tab_view = ResultsTabView(parent=parent)
        self.results_tab_presenter = ResultsTabPresenter(
            self.results_tab_view, ResultsTabModel(fit_context))
