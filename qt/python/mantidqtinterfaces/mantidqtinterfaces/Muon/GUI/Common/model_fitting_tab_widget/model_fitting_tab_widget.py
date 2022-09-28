# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_model import ModelFittingModel
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_presenter import ModelFittingPresenter
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_view import ModelFittingView


class ModelFittingTabWidget(object):
    """
    The ModelFittingTabWidget creates the tab used for model fitting in the Muon Analysis interface.
    """

    def __init__(self, context, parent):
        self.model_fitting_tab_view = ModelFittingView(parent)
        self.model_fitting_tab_view.hide_exclude_range_checkbox()
        self.model_fitting_tab_view.hide_fit_raw_checkbox()
        self.model_fitting_tab_view.hide_evaluate_function_as_checkbox()
        self.model_fitting_tab_model = ModelFittingModel(context, context.model_fitting_context)
        self.model_fitting_tab_presenter = ModelFittingPresenter(self.model_fitting_tab_view,
                                                                 self.model_fitting_tab_model)

        context.update_view_from_model_notifier.add_subscriber(
            self.model_fitting_tab_presenter.update_view_from_model_observer)

        context.results_context.clear_observable.add_subscriber(self.model_fitting_tab_presenter.clear_observer)
        context.results_context.remove_observable.add_subscriber(self.model_fitting_tab_presenter.remove_observer)
        context.results_context.replace_observable.add_subscriber(self.model_fitting_tab_presenter.replace_observer)
