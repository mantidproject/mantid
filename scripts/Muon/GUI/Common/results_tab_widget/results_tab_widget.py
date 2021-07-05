# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.results_tab_widget.results_tab_presenter import ResultsTabPresenter
from Muon.GUI.Common.results_tab_widget.results_tab_view import ResultsTabView
from Muon.GUI.Common.results_tab_widget.results_tab_model import ResultsTabModel


class ResultsTabWidget(object):
    """Factory object to wire together components of the results tab"""

    def __init__(self, fit_context, context, parent):
        """
        Initialize the widget.
        :param fit_context: A reference to the a FitContext object used to store fit results
        :param parent: A parent widget for the view
        """
        self.results_tab_view = ResultsTabView(parent=parent)
        self.results_tab_presenter = ResultsTabPresenter(
            self.results_tab_view, ResultsTabModel(fit_context, context.results_context))

        context.update_view_from_model_notifier.add_subscriber(self.results_tab_presenter.update_view_from_model_observer)
        fit_context.fit_removed_notifier.add_subscriber(self.results_tab_presenter.new_fit_performed_observer)
