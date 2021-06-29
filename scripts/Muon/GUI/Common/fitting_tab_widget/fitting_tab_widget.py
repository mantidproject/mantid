# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.frequency_domain_analysis_context import FrequencyDomainAnalysisContext
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model import GeneralFittingModel
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_presenter import GeneralFittingPresenter
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_view import GeneralFittingView
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_model import TFAsymmetryFittingModel
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_presenter \
    import TFAsymmetryFittingPresenter
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_view import TFAsymmetryFittingView


class FittingTabWidget(object):
    """
    The FittingTabWidget creates the tab used for fitting. Muon Analysis uses the TF Asymmetry fitting widget, and
    Frequency Domain Analysis uses the General fitting widget.
    """

    def __init__(self, context, parent):
        is_frequency_domain = isinstance(context, FrequencyDomainAnalysisContext)

        if is_frequency_domain:
            self.fitting_tab_view = GeneralFittingView(parent, is_frequency_domain)
            self.fitting_tab_model = GeneralFittingModel(context, is_frequency_domain)
            self.fitting_tab_presenter = GeneralFittingPresenter(self.fitting_tab_view, self.fitting_tab_model)
        else:
            self.fitting_tab_view = TFAsymmetryFittingView(parent, is_frequency_domain)
            self.fitting_tab_model = TFAsymmetryFittingModel(context, is_frequency_domain)
            self.fitting_tab_presenter = TFAsymmetryFittingPresenter(self.fitting_tab_view, self.fitting_tab_model)

        self.fitting_tab_presenter.disable_fitting_notifier.add_subscriber(self.fitting_tab_view.disable_tab_observer)

        context.update_view_from_model_notifier.add_subscriber(
            self.fitting_tab_presenter.update_view_from_model_observer)
