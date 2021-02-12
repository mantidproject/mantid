# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing, GenericObservable

from Muon.GUI.Common.fitting_tab_widget.basic_fitting_presenter import BasicFittingPresenter

from mantid import logger


class GeneralFittingPresenter(BasicFittingPresenter):

    def __init__(self, view, model):
        super(GeneralFittingPresenter, self).__init__(view, model)

        self.fit_parameter_changed_notifier = GenericObservable()
        self.fitting_mode_changed_notifier = GenericObservable()
        self.selected_single_fit_notifier = GenericObservable()

        self.input_workspace_observer = GenericObserver(self.handle_new_data_loaded)
        self.selected_group_pair_observer = GenericObserver(self.handle_selected_group_pair_changed)

        self.view.set_slot_for_display_workspace_changed(self.handle_display_workspace_changed)
        self.view.set_slot_for_display_workspace_changed(self.handle_plot_guess_changed)
        self.view.set_slot_for_fitting_mode_changed(self.handle_fitting_mode_changed)
        self.view.set_slot_for_simultaneous_fit_by_changed(self.handle_simultaneous_fit_by_changed)
        self.view.set_slot_for_simultaneous_fit_by_specifier_changed(self.handle_simultaneous_fit_by_specifier_changed)

        self.update_and_reset_all_data()

    def initialize_model_options(self):
        """Returns the fitting options to be used when initializing the model."""
        super().initialize_model_options()
        self.model.simultaneous_fit_by = self.view.simultaneous_fit_by
        self.model.simultaneous_fit_by_specifier = self.view.simultaneous_fit_by_specifier
        self.model.global_parameters = self.view.get_global_parameters()
        self.model.tf_asymmetry_mode = False  # TEMPORARY

    def simultaneous_fitting_mode(self):
        """Returns true if the fitting mode is simultaneous."""
        return self.model.simultaneous_fitting_mode

    def handle_new_data_loaded(self):
        """Handle when new data has been loaded into the interface."""
        self.update_and_reset_all_data()
        super().handle_new_data_loaded()

    def handle_selected_group_pair_changed(self):
        """Update the displayed workspaces when the selected group/pairs change in grouping tab."""
        self.update_and_reset_all_data()

    def handle_fitting_finished(self, fit_function, fit_status, fit_chi_squared):
        """Handle when fitting is finished."""
        self.update_fit_statuses_and_chi_squared_in_model(fit_status, fit_chi_squared)
        self.update_fit_function_in_model(fit_function)

        self.update_fit_statuses_and_chi_squared_in_view_from_model()
        self.update_fit_function_in_view_from_model()

        # Send the workspaces to be plotted
        self.selected_single_fit_notifier.notify_subscribers(self.model.get_active_fit_results())

        # Update parameter values in sequential tab.
        parameter_values = self.model.get_fit_function_parameter_values(fit_function)
        self.model.update_ws_fit_function_parameters(self.model.get_active_workspace_names(), parameter_values)
        self.fit_parameter_changed_notifier.notify_subscribers()

    def handle_plot_guess_changed(self):
        """Handle when plot guess is ticked or unticked."""
        active_workspaces = self.model.get_active_workspace_names()
        self.model.change_plot_guess(self.view.plot_guess, active_workspaces, self.model.current_dataset_index)

    def handle_undo_fit_clicked(self):
        """Handle when undo fit is clicked."""
        super().handle_undo_fit_clicked()
        self.model.simultaneous_fit_function = self.model.simultaneous_fit_function_cache
        self.selected_single_fit_notifier.notify_subscribers(self.model.get_active_fit_results())

    def handle_fitting_mode_changed(self):
        """Handle when the fitting mode is changed to or from simultaneous fitting."""
        self.model.simultaneous_fitting_mode = self.view.is_simultaneous_fit_ticked

        if self.view.is_simultaneous_fit_ticked:
            self.view.switch_to_simultaneous()
        else:
            self.view.switch_to_single()

        self.update_dataset_names_in_view_and_model()
        self.update_fit_functions_in_model_from_view()

        self.fitting_mode_changed_notifier.notify_subscribers()
        self.fit_function_changed_notifier.notify_subscribers()

        # Send the workspaces to be plotted
        self.selected_single_fit_notifier.notify_subscribers(self.model.get_active_fit_results())

        self.view.enable_undo_fit(False)
        self.handle_plot_guess_changed()

    def handle_simultaneous_fit_by_changed(self):
        """Handle when the simultaneous fit by combo box is changed."""
        self.model.simultaneous_fit_by = self.view.simultaneous_fit_by
        self.update_simultaneous_fit_by_specifiers_in_view()

        self.view.update_displayed_data_combo_box(self.model.dataset_names)
        self.view.set_datasets_in_function_browser(self.model.dataset_names)

        self.clear_fit_status_and_chi_squared_information()
        self.reset_start_xs_and_end_xs()

        self.fitting_mode_changed_notifier.notify_subscribers()
        self.fit_function_changed_notifier.notify_subscribers()
        self.selected_single_fit_notifier.notify_subscribers(self.model.get_active_fit_results())

    def handle_simultaneous_fit_by_specifier_changed(self):
        """Handle when the simultaneous fit by specifier combo box is changed."""
        self.model.simultaneous_fit_by_specifier = self.view.simultaneous_fit_by_specifier

        self.model.dataset_names = self.model.get_workspace_names_to_display_from_context()
        self.clear_and_reset_gui_state()
        self.selected_single_fit_notifier.notify_subscribers(self.model.get_active_fit_results())

    def handle_display_workspace_changed(self):
        """Handle when the display workspace combo box is changed."""
        current_index = self.view.get_index_for_start_end_times()

        self.update_fit_functions_in_model_from_view()
        self.set_current_dataset_index(current_index)

        self.view.start_time = self.model.current_start_x
        self.view.end_time = self.model.current_end_x

        self.update_fit_statuses_and_chi_squared_in_view_from_model()
        self.update_fit_function_in_view_from_model()
        self.handle_plot_guess_changed()  # update the guess (use the selected workspace as data for the guess)
        self.selected_single_fit_notifier.notify_subscribers(self.model.get_active_fit_results())

    def handle_function_structure_changed(self):
        """Handle when the function structure is changed."""
        #if not self._fit_function[0]:
        #    self.handle_display_workspace_changed()
        self.view.plot_guess = False
        self.update_fit_functions_in_model_from_view()
        if not self.view.fit_object:
            self.model.clear_fit_information()
            self.selected_single_fit_notifier.notify_subscribers(self.model.get_active_fit_results())

        self.clear_fit_status_and_chi_squared_information()

        self.automatically_update_function_name()

        self.model.global_parameters = self.view.get_global_parameters()

        self.fit_function_changed_notifier.notify_subscribers()

    def handle_function_parameter_changed(self):
        """Handle when the value of a parameter in a function is changed."""
        if not self.view.is_simultaneous_fit_ticked:
            fit_function = self.current_single_fit_function_in_view()
            self.model.current_single_fit_function = fit_function
        else:
            fit_function = self.view.fit_object
            self.model.simultaneous_fit_function = fit_function

        parameter_values = self.model.get_fit_function_parameter_values(fit_function)
        self.model.update_ws_fit_function_parameters(self.model.get_active_workspace_names(), parameter_values)
        self.fit_parameter_changed_notifier.notify_subscribers()
        self.model.update_plot_guess(self.model.get_active_workspace_names(), self.view.get_index_for_start_end_times())

    def clear_and_reset_gui_state(self):
        #"""Clears all data in the view and updates the model."""
        super().clear_and_reset_gui_state()
        self.view.update_displayed_data_combo_box(self.model.dataset_names)

    def update_and_reset_all_data(self):
        """Updates the various data displayed in the fitting widget. Resets and clears previous fit information."""
        self.update_simultaneous_fit_by_specifiers_in_view()
        self.update_dataset_names_in_view_and_model()

        self.clear_fit_status_and_chi_squared_information()
        self.reset_start_xs_and_end_xs()
        self.view.enable_undo_fit(False)
        #self.reset_fit_function()

    def set_display_workspace(self, workspace_name):
        """Sets the workspace to be displayed in the view programmatically."""
        self.view.display_workspace = workspace_name
        self.handle_display_workspace_changed()

    def update_fit_statuses_and_chi_squared_in_model(self, fit_status, chi_squared):
        """Updates the fit status and chi squared stored in the model. This is used after a fit."""
        if self.view.is_simultaneous_fit_ticked:
            self.model.fit_statuses = [fit_status] * self.model.number_of_datasets
            self.model.fit_chi_squares = [chi_squared] * self.model.number_of_datasets
        else:
            self.model.current_fit_status = fit_status
            self.model.current_fit_chi_squared = chi_squared

    def update_fit_function_in_model(self, fit_function):
        """Updates the fit function stored in the model. This is used after a fit."""
        if self.view.is_simultaneous_fit_ticked:
            self.model.simultaneous_fit_function = fit_function
        else:
            self.model.current_single_fit_function = fit_function

    def update_fit_function_in_view_from_model(self):
        """Updates the parameters of a fit function shown in the view."""
        if self.view.is_simultaneous_fit_ticked:
            self.view.update_fit_function(self.model.simultaneous_fit_function)
        else:
            self.view.update_fit_function(self.model.current_single_fit_function)

    def update_simultaneous_fit_by_specifiers_in_view(self):
        """Updates the entries in the simultaneous fit by specifier combo box."""
        self.view.setup_fit_by_specifier(self.model.get_simultaneous_fit_by_specifiers_to_display_from_context())

    def update_dataset_names_in_view_and_model(self):
        """Updates the datasets currently displayed. The simultaneous fit by specifier must be updated before this."""
        self.model.dataset_names = self.model.get_workspace_names_to_display_from_context()
        self.view.update_displayed_data_combo_box(self.model.dataset_names)
        self.view.set_datasets_in_function_browser(self.model.dataset_names)

    def update_fit_functions_in_model_from_view(self):
        """Updates the fit functions stored in the model using the view."""
        if self.view.is_simultaneous_fit_ticked:
            self.model.clear_single_fit_functions()
            self.update_simultaneous_fit_function_in_model()
        else:
            self.model.clear_simultaneous_fit_function()
            self.update_single_fit_functions_in_model()

    def update_simultaneous_fit_function_in_model(self):
        """Updates the simultaneous fit function in the model using the view."""
        self.model.simultaneous_fit_function = self.view.fit_object
