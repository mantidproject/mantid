# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing, GenericObservable

from Muon.GUI.Common.ADSHandler.workspace_naming import get_run_numbers_as_string_from_workspace_name
from Muon.GUI.Common.fitting_tab_widget.basic_fitting_presenter import BasicFittingPresenter


from mantid import logger


class GeneralFittingPresenter(BasicFittingPresenter):

    def __init__(self, view, model, context):
        super(GeneralFittingPresenter, self).__init__(view, model, context)

        self.update_selected_workspace_list_for_fit()

        self.fit_parameter_changed_notifier = GenericObservable()
        self.fitting_mode_changed_notifier = GenericObservable()
        self.selected_single_fit_notifier = GenericObservable()

        self.input_workspace_observer = GenericObserver(self.handle_new_data_loaded)
        self.selected_group_pair_observer = GenericObserver(self.handle_selected_group_pair_changed)
        self.selected_plot_type_observer = GenericObserverWithArgPassing(self.handle_selected_plot_type_changed)

        self.view.set_slot_for_display_workspace_changed(self.handle_display_workspace_changed)
        self.view.set_slot_for_display_workspace_changed(self.handle_plot_guess_changed)
        self.view.set_slot_for_fitting_mode_changed(self.handle_fitting_mode_changed)
        self.view.set_slot_for_simultaneous_fit_by_changed(self.handle_simultaneous_fit_by_changed)
        self.view.set_slot_for_simultaneous_fit_by_specifier_changed(self.handle_simultaneous_fit_by_specifier_changed)

    def initialize_model_options(self):
        """Returns the fitting options to be used when initializing the model."""
        super().initialize_model_options()
        self.model.simultaneous_fit_by = self.view.simultaneous_fit_by  # TEMPORARY POSSIBLY
        self.model.simultaneous_fit_by_specifier = self.view.simultaneous_fit_by_specifier  # TEMPORARY POSSIBLY
        self.model.global_parameters = self.view.get_global_parameters()
        self.model.tf_asymmetry_mode = False  # TEMPORARY

    def get_loaded_workspaces(self):
        """Retrieve the names of the workspaces successfully loaded into the fitting interface."""
        return self.view.loaded_workspaces

    def _is_simultaneous_fitting(self):
        """Returns true if the fitting mode is simultaneous."""
        return self.view.is_simultaneous_fit_ticked

    def handle_selected_group_pair_changed(self):
        """Update the displayed workspaces when the selected group/pairs change in grouping tab."""
        self.update_selected_workspace_list_for_fit()
        self.update_fit_functions_in_model()
        self.model.simultaneous_fit_by_specifier = self.view.simultaneous_fit_by_specifier

    def handle_selected_plot_type_changed(self, _):
        """Update the selected workspace list when the selected plot has changed."""
        self.update_selected_workspace_list_for_fit()
        self.model.simultaneous_fit_by_specifier = self.view.simultaneous_fit_by_specifier

    def handle_new_data_loaded(self):
        """Handle when new data has been loaded into the interface."""
        self.update_selected_workspace_list_for_fit()
        self.model.simultaneous_fit_by_specifier = self.view.simultaneous_fit_by_specifier
        super().handle_new_data_loaded()

    def handle_fitting_finished(self, fit_function, fit_status, fit_chi_squared):
        """Handle when fitting is finished."""
        if self.view.is_simultaneous_fit_ticked:
            self.model.simultaneous_fit_function = fit_function
            self.model.fit_statuses = [fit_status] * len(self.model.start_xs)
            self.model.fit_chi_squares = [fit_chi_squared] * len(self.model.start_xs)
        else:
            self.model.current_single_fit_function = fit_function
            self.model.current_fit_status = fit_status
            self.model.current_fit_chi_squared = fit_chi_squared

        self.update_fit_status_in_the_view()

        # Send the workspaces to be plotted
        self.selected_single_fit_notifier.notify_subscribers(self.model.get_active_fit_results())

        # Update parameter values in sequential tab.
        parameter_values = self.model.get_fit_function_parameter_values(fit_function)
        self.model.update_ws_fit_function_parameters(self.model.get_active_workspace_names(), parameter_values)
        self.fit_parameter_changed_notifier.notify_subscribers()

    def handle_plot_guess_changed(self):
        """Handle when plot guess is ticked or unticked."""
        index = self.view.get_index_for_start_end_times()
        workspaces = self.model.get_active_workspace_names()
        self.model.change_plot_guess(self.view.plot_guess, workspaces, index)

    def handle_undo_fit_clicked(self):
        """Handle when undo fit is clicked."""
        super().handle_undo_fit_clicked()
        self.selected_single_fit_notifier.notify_subscribers(self.model.get_active_fit_results())

    def handle_fitting_mode_changed(self):
        """Handle when the fitting mode is changed to or from simultaneous fitting."""
        self.view.enable_undo_fit(False)
        self.model.simultaneous_fitting_mode = self.view.is_simultaneous_fit_ticked

        self.model.dataset_names = self.model.get_workspace_names_to_display_from_context()
        logger.warning(f"{str(self.model.dataset_names)}")

        if self.view.is_simultaneous_fit_ticked:
            self.view.set_workspace_combo_box_label("Display parameters for")
            self.view.enable_simultaneous_fit_options()
            self.view.switch_to_simultaneous()
        else:
            self.clear_and_reset_gui_state()
            self.view.set_workspace_combo_box_label("Select Workspace")
            self.view.disable_simultaneous_fit_options()
            self.view.switch_to_single()

        self.update_fit_functions_in_model()
        self.model.simultaneous_fit_by = self.view.simultaneous_fit_by

        self.fitting_mode_changed_notifier.notify_subscribers()
        self.fit_function_changed_notifier.notify_subscribers()
        # Send the workspaces to be plotted
        self.selected_single_fit_notifier.notify_subscribers(self.model.get_active_fit_results())
        self.handle_plot_guess_changed()

    def handle_simultaneous_fit_by_changed(self):
        """Handle when the simultaneous fit by combo box is changed."""
        self.model.simultaneous_fit_by = self.view.simultaneous_fit_by
        self.update_fit_by_specifier_list()

        self.view.update_displayed_data_combo_box(self.model.dataset_names)
        self.view.set_datasets_in_function_browser(self.model.dataset_names)

        self._reset_fit_status_information()
        self._reset_start_time_to_first_good_data_value()

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

        self.update_fit_functions_in_model()
        self.set_current_dataset_index(current_index)

        self.view.start_time = self.model.current_start_x
        self.view.end_time = self.model.current_end_x

        self.update_fit_status_in_the_view()
        self.handle_plot_guess_changed()  # update the guess (use the selected workspace as data for the guess)
        self.selected_single_fit_notifier.notify_subscribers(self.model.get_active_fit_results())

    def handle_function_structure_changed(self):
        """Handle when the function structure is changed."""
        #if not self._fit_function[0]:
        #    self.handle_display_workspace_changed()
        self.view.plot_guess = False
        self.update_fit_functions_in_model()
        if not self.view.fit_object:
            self.model.clear_fit_information()
            self.selected_single_fit_notifier.notify_subscribers(self.model.get_active_fit_results())

        self._reset_fit_status_information()

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

    def handle_start_x_updated(self):
        """Handle when the start X is changed."""
        value = self.view.start_time
        # Check start is greater than end, swap if need be
        if value > self.model.current_end_x:
            self.view.start_time, self.view.end_time = self.model.current_end_x, value
            self.model.current_end_x = value
            value = self.view.start_time

        self.model.current_start_x = value

    def handle_end_x_updated(self):
        """Handle when the end X is changed."""
        value = self.view.end_time
        # Check end is less than start, swap if need be
        if value < self.model.current_start_x:
            self.view.start_time, self.view.end_time = value, self.model.current_start_x
            self.model.current_start_x = value
            value = self.view.end_time

        self.model.current_end_x = value

    def handle_use_rebin_changed(self):
        """Handle the Use Rebin state change."""
        if not self._validate_rebin_options():
            return

        self.update_selected_workspace_list_for_fit()

        self.context.fitting_context.fit_raw = self.view.fit_to_raw
        self.model.fit_to_raw = self.view.fit_to_raw

    def clear_and_reset_gui_state(self):
        """Clears all data in the view and updates the model."""
        super().clear_and_reset_gui_state()
        self.view.update_displayed_data_combo_box(self.model.dataset_names)

    def update_selected_workspace_list_for_fit(self):
        """Updates the simultaneous fit specifier and selected data."""
        self.update_fit_by_specifier_list()

        self.model.dataset_names = self.model.get_workspace_names_to_display_from_context()
        self.clear_and_reset_gui_state()

    def set_display_workspace(self, workspace_name):
        """Set the display workspace programmatically."""
        self.view.display_workspace = workspace_name
        self.handle_display_workspace_changed()

    def update_fit_by_specifier_list(self):
        """Updates the simultaneous fit by specifier combo box."""
        if self.view.simultaneous_fit_by == "Run":
            # extract runs from run list of lists, which is in the format [ [run,...,runs],[runs],...,[runs] ]
            current_runs = self.context.data_context.current_runs
            if len(current_runs) > 1:
                run_numbers = [str(item) for sub_list in current_runs for item in sub_list]
            else:
                # extract runs from workspace
                ws_list = self.context.data_context.current_data["OutputWorkspace"]
                run_numbers = [str(get_run_numbers_as_string_from_workspace_name(
                    ws.workspace_name, self.context.data_context.instrument)) for ws in ws_list]
            run_numbers.sort()
            simul_choices = run_numbers
        elif self.view.simultaneous_fit_by == "Group/Pair":
            simul_choices = self.model._get_selected_groups_and_pairs()
        else:
            simul_choices = []

        self.view.setup_fit_by_specifier(simul_choices)

    def update_fit_status_in_the_view(self):
        if self.view.is_simultaneous_fit_ticked:
            self.update_fit_status_and_function_in_the_view(self.model.simultaneous_fit_function)
        else:
            self.update_fit_status_and_function_in_the_view(self.model.current_single_fit_function)

    def update_fit_functions_in_model(self):
        """Updates the fit functions stored in the model using the view."""
        if self.view.is_simultaneous_fit_ticked:
            self.update_simultaneous_fit_function_in_model()
        else:
            self.update_single_fit_functions_in_model()

    def update_simultaneous_fit_function_in_model(self):
        """Updates the simultaneous fit function in the model using the view."""
        self.model.simultaneous_fit_function = self.view.fit_object
