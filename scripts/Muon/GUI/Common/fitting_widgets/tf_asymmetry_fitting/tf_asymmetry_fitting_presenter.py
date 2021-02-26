# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserverWithArgPassing

from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_presenter import GeneralFittingPresenter
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_model import TFAsymmetryFittingModel
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_view import TFAsymmetryFittingView


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

        self.view.set_slot_for_normalisation_changed(self.handle_normalisation_changed)

    def initialize_model_options(self) -> None:
        """Returns the fitting options to be used when initializing the model."""
        super().initialize_model_options()
        self.model.tf_asymmetry_mode = self.view.tf_asymmetry_mode

    def handle_selected_group_pair_changed(self) -> None:
        """Disable TF Asymmetry mode when the selected group/pairs change in the grouping tab."""
        self.model.tf_asymmetry_mode = False
        self.view.tf_asymmetry_mode = self.model.tf_asymmetry_mode
        self.tf_asymmetry_mode_changed_notifier.notify_subscribers(False)

        super().handle_selected_group_pair_changed()

    def handle_function_structure_changed(self) -> None:
        """Handles when the function structure has been changed."""
        super().handle_function_structure_changed()
        self.update_tf_asymmetry_functions_in_model_and_view()

    def handle_dataset_name_changed(self) -> None:
        """Handles when the selected dataset has been changed."""
        super().handle_dataset_name_changed()
        self.view.normalisation = self.model.current_normalisation()

    def handle_tf_asymmetry_mode_changed(self, tf_asymmetry_on: bool) -> None:
        """Handles when TF Asymmetry fitting mode has been turned off or on."""
        if not self._check_tf_asymmetry_compliance(tf_asymmetry_on):
            return

        self.model.tf_asymmetry_mode = tf_asymmetry_on
        self.view.tf_asymmetry_mode = self.model.tf_asymmetry_mode

        self.update_tf_asymmetry_functions_in_model_and_view()

        self.reset_start_xs_and_end_xs()
        self.reset_fit_status_and_chi_squared_information()
        self.clear_cached_fit_functions()

        self.automatically_update_function_name()

        if self._update_plot:
            self.selected_fit_results_changed.notify_subscribers(self.model.get_active_fit_results())

        self.model.update_plot_guess(self.view.plot_guess)

    def handle_normalisation_changed(self) -> None:
        """Handles when the normalisation line edit has been changed by the user."""
        self.model.set_current_normalisation(self.view.normalisation)
        self.model.update_plot_guess(self.view.plot_guess)

    def update_and_reset_all_data(self) -> None:
        """Updates the various data displayed in the fitting widget. Resets and clears previous fit information."""
        super().update_and_reset_all_data()
        self.update_tf_asymmetry_functions_in_model_and_view()

    def update_tf_asymmetry_functions_in_model_and_view(self) -> None:
        """Recalculates the current TF Asymmetry functions based on the ordinary fit functions."""
        if not self.model.recalculate_tf_asymmetry_functions():
            self.view.warning_popup("Failed to convert fit function to a TF Asymmetry function.")
        self.view.normalisation = self.model.current_normalisation()

    def update_fit_function_in_model(self, fit_function: IFunction) -> None:
        """Updates the fit function stored in the model. This is used after a fit."""
        if self.model.tf_asymmetry_mode:
            if self.model.simultaneous_fitting_mode:
                self.model.update_simultaneous_fit_functions(fit_function)
            else:
                self.model.update_current_single_fit_functions(fit_function)
        else:
            super().update_fit_function_in_model(fit_function)

    def _check_tf_asymmetry_compliance(self, tf_asymmetry_on: bool) -> None:
        """Check that the current datasets are compatible with TF Asymmetry fitting mode."""
        if tf_asymmetry_on and not self.model.check_datasets_are_tf_asymmetry_compliant():
            self.view.warning_popup("Only Groups can be fitted in TF Asymmetry mode.")
            self.tf_asymmetry_mode_changed_notifier.notify_subscribers(False)
            return False
        return True
