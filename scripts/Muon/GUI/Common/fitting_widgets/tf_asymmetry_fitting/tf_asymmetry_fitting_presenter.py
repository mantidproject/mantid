# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserver, GenericObserverWithArgPassing

from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_presenter import GeneralFittingPresenter
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_model import TFAsymmetryFittingModel
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_view import TFAsymmetryFittingView

from mantid import logger


class TFAsymmetryFittingPresenter(GeneralFittingPresenter):
    """
    The TFAsymmetryFittingPresenter has a TFAsymmetryFittingView and TFAsymmetryFittingModel and derives from
    GeneralFittingPresenter.
    """

    def __init__(self, view: TFAsymmetryFittingView, model: TFAsymmetryFittingModel):
        """Initialize the TFAsymmetryFittingPresenter. Sets up the slots and event observers."""
        super(TFAsymmetryFittingPresenter, self).__init__(view, model)

        self.tf_asymmetry_mode_changed_notifier = GenericObservable()
        self.tf_asymmetry_mode_changed_observer = GenericObserverWithArgPassing(self.handle_tf_asymmetry_mode_changed)

    def initialize_model_options(self) -> None:
        """Returns the fitting options to be used when initializing the model."""
        super().initialize_model_options()
        self.model.tf_asymmetry_mode = self.view.tf_asymmetry_mode

    def handle_function_structure_changed(self) -> None:
        super().handle_function_structure_changed()
        self.update_normalisations_in_model_and_view()

    def handle_dataset_name_changed(self) -> None:
        super().handle_dataset_name_changed()
        self.view.normalisation = self.model.current_normalisation

    def handle_tf_asymmetry_mode_changed(self, tf_asymmetry_on):
        if not self._check_tf_asymmetry_compliance(tf_asymmetry_on):
            return

        self.model.tf_asymmetry_mode = tf_asymmetry_on
        self.view.tf_asymmetry_mode = self.model.tf_asymmetry_mode

        self.update_normalisations_in_model_and_view()

        self.reset_start_xs_and_end_xs()
        self.reset_fit_status_and_chi_squared_information()
        self.clear_cached_fit_functions()

        self.automatically_update_function_name()

        self.model.update_plot_guess(self.view.plot_guess)

        if self._update_plot:
            self.selected_fit_results_changed.notify_subscribers(self.model.get_active_fit_results())

    def update_and_reset_all_data(self):
        super().update_and_reset_all_data()
        self.update_normalisations_in_model_and_view()

    def update_normalisations_in_model_and_view(self):
        if not self.model.recalculate_normalisations():
            self.view.warning_popup("Failed to calculate the normalisation: the fit function may be invalid.")
        self.view.normalisation = self.model.current_normalisation

    def _check_tf_asymmetry_compliance(self, tf_asymmetry_on):
        if tf_asymmetry_on and not self.model.check_datasets_are_tf_asymmetry_compliant():
            self.view.warning_popup("Only Groups can be fitted in TF Asymmetry mode.")
            self.tf_asymmetry_mode_changed_notifier.notify_subscribers(False)
            return False
        return True
