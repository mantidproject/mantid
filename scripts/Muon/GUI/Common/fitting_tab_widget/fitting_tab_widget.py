# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.frequency_domain_analysis_context import FrequencyDomainAnalysisContext
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_view import FittingTabView
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_model import FittingTabModel
from Muon.GUI.Common.fitting_tab_widget.general_fitting_presenter import GeneralFittingPresenter
from Muon.GUI.Common.fitting_tab_widget.general_fitting_view import GeneralFittingView


class FittingTabWidget(object):
    def __init__(self, context, parent):
        is_frequency_domain = isinstance(context, FrequencyDomainAnalysisContext)

        self.general_fitting_view = GeneralFittingView(parent, is_frequency_domain)
        self.general_fitting_model = FittingTabModel(context)
        self.general_fitting_presenter = GeneralFittingPresenter(self.general_fitting_view, self.general_fitting_model,
                                                                 context)

        self.fitting_tab_view = FittingTabView(parent, context, self.general_fitting_view)
