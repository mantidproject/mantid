# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction

from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_presenter import GeneralFittingPresenter
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_model import TFAsymmetryFittingModel
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_view import TFAsymmetryFittingView


class TFAsymmetryFittingPresenter(GeneralFittingPresenter):
    """
    The TFAsymmetryFittingPresenter has a TFAsymmetryFittingView and TFAsymmetryFittingModel and derives from
    GeneralFittingPresenter.
    """

    def __init__(self, view: TFAsymmetryFittingView, model: TFAsymmetryFittingModel):
        """Initialize the TFAsymmetryFittingPresenter. Sets up the slots and event observers."""
        super(TFAsymmetryFittingPresenter, self).__init__(view, model)

        self.view.set_slot_for_fitting_type_changed(self.handle_tf_asymmetry_mode_changed)
        self.view.set_slot_for_normalisation_changed(self.handle_normalisation_changed)
        self.view.set_slot_for_fix_normalisation_changed(lambda fix: self.handle_fix_normalisation_changed(fix))

    def initialize_model_options(self) -> None:
        """Returns the fitting options to be used when initializing the model."""
        super().initialize_model_options()
        self.model.tf_asymmetry_mode = self.view.tf_asymmetry_mode

    def handle_instrument_changed(self) -> None:
        """Handles when an instrument is changed and switches to normal fitting mode."""
        self._switch_to_normal_fitting()
        super().handle_instrument_changed()

    def handle_ads_clear_or_remove_workspace_event(self, name: str) -> None:
        """Handle when there is a clear or remove workspace event in the ADS."""
        super().handle_ads_clear_or_remove_workspace_event(name)

        if self.model.number_of_datasets == 0:
            self._switch_to_normal_fitting()

    def handle_new_data_loaded(self) -> None:
        """Handle when new data has been loaded into the interface."""
        super().handle_new_data_loaded()

        if self.model.number_of_datasets == 0:
            self._switch_to_normal_fitting()

    def handle_function_structure_changed(self) -> None:
        """Handles when the function structure has been changed."""
        super().handle_function_structure_changed()
        self.update_tf_asymmetry_functions_in_model_and_view()

    def handle_dataset_name_changed(self) -> None:
        """Handles when the selected dataset has been changed."""
        super().handle_dataset_name_changed()
        self.view.set_normalisation(self.model.current_normalisation(), self.model.current_normalisation_error())
        self.view.is_normalisation_fixed = self.model.is_current_normalisation_fixed()

    def handle_tf_asymmetry_mode_changed(self, tf_asymmetry_on: bool) -> None:
        """Handles when TF Asymmetry fitting mode has been turned off or on."""
        if not self._check_tf_asymmetry_compliance(tf_asymmetry_on):
            return

        self.view.tf_asymmetry_mode, self.model.tf_asymmetry_mode = tf_asymmetry_on, tf_asymmetry_on

        self.update_tf_asymmetry_functions_in_model_and_view()

        self.automatically_update_function_name()

        self.reset_fit_status_and_chi_squared_information()
        self.clear_undo_data()

        self.update_plot_fit()
        self.update_plot_guess()

        self.fitting_mode_changed_notifier.notify_subscribers()

    def handle_normalisation_changed(self) -> None:
        """Handles when the normalisation line edit has been changed by the user."""
        self.model.set_current_normalisation(self.view.normalisation)
        self.update_plot_guess()

        self.fit_parameter_changed_notifier.notify_subscribers()

    def handle_fix_normalisation_changed(self, is_fixed: bool) -> None:
        """Handles when the value of the current normalisation has been fixed or unfixed."""
        self.model.toggle_fix_current_normalisation(is_fixed)

    def update_and_reset_all_data(self) -> None:
        """Updates the various data displayed in the fitting widget. Resets and clears previous fit information."""
        super().update_and_reset_all_data()
        self.update_tf_asymmetry_functions_in_model_and_view()

    def update_tf_asymmetry_functions_in_model_and_view(self) -> None:
        """Recalculates the current TF Asymmetry functions based on the ordinary fit functions."""
        if not self.model.recalculate_tf_asymmetry_functions():
            self.view.warning_popup("Failed to convert fit function to a TF Asymmetry function.")
        self.view.set_normalisation(self.model.current_normalisation(), self.model.current_normalisation_error())
        self.view.is_normalisation_fixed = self.model.is_current_normalisation_fixed()

    def update_fit_function_in_model(self, fit_function: IFunction) -> None:
        """Updates the fit function stored in the model. This is used after a fit."""
        if self.model.tf_asymmetry_mode:
            if self.model.simultaneous_fitting_mode:
                self.model.update_tf_asymmetry_simultaneous_fit_function(fit_function)
            else:
                self.model.update_tf_asymmetry_single_fit_function(self.model.current_dataset_index, fit_function)
        else:
            super().update_fit_function_in_model(fit_function)

    def update_dataset_names_in_view_and_model(self):
        """Updates the datasets currently displayed. TF Asymmetry mode is switched off if the datasets don't comply."""
        super().update_dataset_names_in_view_and_model()
        self._check_tf_asymmetry_compliance(self.model.tf_asymmetry_mode)

    def update_fit_function_in_view_from_model(self) -> None:
        """Updates the parameters of a fit function shown in the view."""
        super().update_fit_function_in_view_from_model()
        self.view.set_normalisation(self.model.current_normalisation(), self.model.current_normalisation_error())
        self.view.is_normalisation_fixed = self.model.is_current_normalisation_fixed()

    def _switch_to_normal_fitting(self):
        """Activates normal fitting by turning off TF Asymmetry fitting."""
        self.view.tf_asymmetry_mode, self.model.tf_asymmetry_mode = False, False
        self.automatically_update_function_name()

    def _check_tf_asymmetry_compliance(self, tf_asymmetry_on: bool) -> None:
        """Check that the current datasets are compatible with TF Asymmetry fitting mode."""
        tf_compliant, non_compliant_names = self.model.check_datasets_are_tf_asymmetry_compliant()
        if tf_asymmetry_on and not tf_compliant:
            self.view.warning_popup(
                f"Switching to Normal Fitting mode because only Groups can be fitted in TF "
                f"Asymmetry mode. Please unselect the following Pairs/Diffs in the grouping tab: "
                f"{non_compliant_names}"
            )
            self._switch_to_normal_fitting()
            return False
        return True
