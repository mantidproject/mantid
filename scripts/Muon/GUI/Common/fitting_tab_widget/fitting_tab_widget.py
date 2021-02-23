# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.frequency_domain_analysis_context import FrequencyDomainAnalysisContext
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_model import TFAsymmetryFittingModel
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_presenter \
    import TFAsymmetryFittingPresenter
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_view import TFAsymmetryFittingView
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_view import FittingTabView


class FittingTabWidget(object):
    def __init__(self, context, parent):
        is_frequency_domain = isinstance(context, FrequencyDomainAnalysisContext)

        self.tf_asymmetry_fitting_view = TFAsymmetryFittingView(parent, is_frequency_domain)
        self.tf_asymmetry_fitting_model = TFAsymmetryFittingModel(context, is_frequency_domain)
        self.tf_asymmetry_fitting_presenter = TFAsymmetryFittingPresenter(self.tf_asymmetry_fitting_view,
                                                                          self.tf_asymmetry_fitting_model)

        self.fitting_tab_view = FittingTabView(parent, context, self.tf_asymmetry_fitting_view)

        self.tf_asymmetry_fitting_presenter.reset_tab_notifier.add_subscriber(
            self.fitting_tab_view.reset_tab_observer)
        self.tf_asymmetry_fitting_presenter.enable_editing_notifier.add_subscriber(
            self.fitting_tab_view.enable_tab_observer)
        self.tf_asymmetry_fitting_presenter.tf_asymmetry_mode_changed_notifier.add_subscriber(
            self.fitting_tab_view.tf_asymmetry_mode_changed_observer)

        self.fitting_tab_view.tf_asymmetry_mode_changed_notifier.add_subscriber(
            self.tf_asymmetry_fitting_presenter.tf_asymmetry_mode_changed_observer)

        context.update_view_from_model_notifier.add_subscriber(
            self.tf_asymmetry_fitting_presenter.update_view_from_model_observer)
