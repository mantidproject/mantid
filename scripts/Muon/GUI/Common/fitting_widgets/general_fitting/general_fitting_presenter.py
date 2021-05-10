# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction
from mantidqt.utils.observer_pattern import GenericObserver, GenericObservable, GenericObserverWithArgPassing

from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_presenter import BasicFittingPresenter
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model import GeneralFittingModel
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_view import GeneralFittingView


class GeneralFittingPresenter(BasicFittingPresenter):
    """
    The GeneralFittingPresenter has a GeneralFittingView and GeneralFittingModel and derives from BasicFittingPresenter.
    """

    def __init__(self, view: GeneralFittingView, model: GeneralFittingModel):
        """Initialize the GeneralFittingPresenter. Sets up the slots and event observers."""
        super(GeneralFittingPresenter, self).__init__(view, model)

        # This prevents plotting the wrong data when selecting different group/pairs on the grouping tab
        self._update_plot = True

        self.fitting_mode_changed_notifier = GenericObservable()
        self.simultaneous_fit_by_specifier_changed = GenericObservable()

        self.selected_group_pair_observer = GenericObserver(self.handle_selected_group_pair_changed)
        self.instrument_changed_observer = GenericObserver(self.handle_instrument_changed)
        self.fit_parameter_updated_observer = GenericObserver(self.update_fit_function_in_view_from_model)

        self.double_pulse_observer = GenericObserverWithArgPassing(self.handle_pulse_type_changed)
        self.model.context.gui_context.add_non_calc_subscriber(self.double_pulse_observer)

        self.view.set_slot_for_dataset_changed(self.handle_dataset_name_changed)
        self.view.set_slot_for_fitting_mode_changed(self.handle_fitting_mode_changed)
        self.view.set_slot_for_simultaneous_fit_by_changed(self.handle_simultaneous_fit_by_changed)
        self.view.set_slot_for_simultaneous_fit_by_specifier_changed(self.handle_simultaneous_fit_by_specifier_changed)

        self.update_and_reset_all_data()

    def initialize_model_options(self) -> None:
        """Returns the fitting options to be used when initializing the model."""
        super().initialize_model_options()
        self.model.simultaneous_fit_by = self.view.simultaneous_fit_by
        self.model.simultaneous_fit_by_specifier = self.view.simultaneous_fit_by_specifier
        self.model.global_parameters = self.view.global_parameters

    def handle_instrument_changed(self) -> None:
        """Handles when an instrument is changed and switches to normal fitting mode. Overridden by child."""
        self._update_plot = False
        self.update_and_reset_all_data()
        self._update_plot = True

    def handle_pulse_type_changed(self, updated_variables: dict) -> None:
        """Handles when double pulse mode is switched on and switches to normal fitting mode."""
        if "DoublePulseEnabled" in updated_variables:
            self._update_plot = False
            self.update_and_reset_all_data()
            self._update_plot = True

    def handle_selected_group_pair_changed(self) -> None:
        """Update the displayed workspaces when the selected group/pairs change in grouping tab."""
        self._update_plot = False
        self.update_and_reset_all_data()
        self._update_plot = True

    def handle_fitting_finished(self, fit_function, fit_status, chi_squared) -> None:
        """Handle when fitting is finished."""
        self.update_fit_statuses_and_chi_squared_in_model(fit_status, chi_squared)
        self.update_fit_function_in_model(fit_function)

        self.update_fit_statuses_and_chi_squared_in_view_from_model()
        self.update_fit_function_in_view_from_model()

        self.selected_fit_results_changed.notify_subscribers(self.model.get_active_fit_results())
        self.fit_parameter_changed_notifier.notify_subscribers()

    def handle_fitting_mode_changed(self) -> None:
        """Handle when the fitting mode is changed to or from simultaneous fitting."""
        self.model.simultaneous_fitting_mode = self.view.simultaneous_fitting_mode
        self.switch_fitting_mode_in_view()

        self.update_fit_functions_in_model_from_view()

        # Triggers handle_dataset_name_changed
        self.update_dataset_names_in_view_and_model()

        self.automatically_update_function_name()

        self.reset_fit_status_and_chi_squared_information()
        self.clear_cached_fit_functions()

        self.fitting_mode_changed_notifier.notify_subscribers()
        self.fit_function_changed_notifier.notify_subscribers()

    def handle_simultaneous_fit_by_changed(self) -> None:
        """Handle when the simultaneous fit by combo box is changed."""
        self.model.simultaneous_fit_by = self.view.simultaneous_fit_by

        # Triggers handle_simultaneous_fit_by_specifier_changed
        self.update_simultaneous_fit_by_specifiers_in_view()

    def handle_simultaneous_fit_by_specifier_changed(self) -> None:
        """Handle when the simultaneous fit by specifier combo box is changed."""
        self.model.simultaneous_fit_by_specifier = self.view.simultaneous_fit_by_specifier

        # Triggers handle_dataset_name_changed
        self.update_dataset_names_in_view_and_model()

        self.reset_start_xs_and_end_xs()
        self.reset_fit_status_and_chi_squared_information()
        self.clear_cached_fit_functions()

        self.simultaneous_fit_by_specifier_changed.notify_subscribers()

    def handle_dataset_name_changed(self) -> None:
        """Handle when the display workspace combo box is changed."""
        self.model.current_dataset_index = self.view.current_dataset_index

        self.view.start_x = self.model.current_start_x
        self.view.end_x = self.model.current_end_x

        self.update_fit_statuses_and_chi_squared_in_view_from_model()
        self.update_fit_function_in_view_from_model()

        if self._update_plot:
            self.selected_fit_results_changed.notify_subscribers(self.model.get_active_fit_results())
            self.model.update_plot_guess(self.view.plot_guess)

    def set_selected_dataset(self, dataset_name: str) -> None:
        """Sets the workspace to be displayed in the view programmatically."""
        # Triggers handle_dataset_name_changed which updates the model
        self.view.current_dataset_name = dataset_name

    def switch_fitting_mode_in_view(self) -> None:
        """Switches the fitting mode by updating the relevant labels and checkboxes in the view."""
        if self.model.simultaneous_fitting_mode:
            self.view.switch_to_simultaneous()
        else:
            self.view.switch_to_single()

    def update_and_reset_all_data(self) -> None:
        """Updates the various data displayed in the fitting widget. Resets and clears previous fit information."""
        # Triggers handle_simultaneous_fit_by_specifier_changed
        self.update_simultaneous_fit_by_specifiers_in_view()

    def update_fit_statuses_and_chi_squared_in_model(self, fit_status: str, chi_squared: float) -> None:
        """Updates the fit status and chi squared stored in the model. This is used after a fit."""
        if self.model.simultaneous_fitting_mode:
            self.model.fit_statuses = [fit_status] * self.model.number_of_datasets
            self.model.chi_squared = [chi_squared] * self.model.number_of_datasets
        else:
            self.model.current_fit_status = fit_status
            self.model.current_chi_squared = chi_squared

    def update_fit_function_in_model(self, fit_function: IFunction) -> None:
        """Updates the fit function stored in the model. This is used after a fit."""
        if self.model.simultaneous_fitting_mode:
            self.model.simultaneous_fit_function = fit_function
        else:
            self.model.current_single_fit_function = fit_function

    def update_simultaneous_fit_by_specifiers_in_view(self) -> None:
        """Updates the entries in the simultaneous fit by specifier combo box."""
        self.view.setup_fit_by_specifier(self.model.get_simultaneous_fit_by_specifiers_to_display_from_context())

    def update_dataset_names_in_view_and_model(self) -> None:
        """Updates the datasets currently displayed. The simultaneous fit by specifier must be updated before this."""
        super().update_dataset_names_in_view_and_model()
        self.view.update_dataset_name_combo_box(self.model.dataset_names)
        self.model.current_dataset_index = self.view.current_dataset_index

    def update_fit_functions_in_model_from_view(self) -> None:
        """Updates the fit functions stored in the model using the view."""
        self.model.global_parameters = self.view.global_parameters

        if self.model.simultaneous_fitting_mode:
            self.model.clear_single_fit_functions()
            self.update_simultaneous_fit_function_in_model()
        else:
            self.model.clear_simultaneous_fit_function()
            self.update_single_fit_functions_in_model()

    def update_simultaneous_fit_function_in_model(self) -> None:
        """Updates the simultaneous fit function in the model using the view."""
        self.model.simultaneous_fit_function = self.view.fit_object
