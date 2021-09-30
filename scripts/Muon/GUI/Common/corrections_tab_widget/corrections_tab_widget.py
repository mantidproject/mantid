# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel
from Muon.GUI.Common.corrections_tab_widget.corrections_presenter import CorrectionsPresenter
from Muon.GUI.Common.corrections_tab_widget.corrections_view import CorrectionsView


class CorrectionsTabWidget(object):
    """
    The CorrectionsTabWidget creates the tab used for corrections in the Muon Analysis and Frequency Domain Analysis
    interface.
    """

    def __init__(self, context, parent):
        self.corrections_tab_view = CorrectionsView(parent)
        self.corrections_tab_model = CorrectionsModel(context)
        self.corrections_tab_presenter = CorrectionsPresenter(self.corrections_tab_view, self.corrections_tab_model,
                                                              context)

        context.update_view_from_model_notifier.add_subscriber(
            self.corrections_tab_presenter.update_view_from_model_observer)
