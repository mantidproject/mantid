# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import MultiDomainFunction
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing, GenericObservable

from Muon.GUI.Common.ADSHandler.workspace_naming import get_run_numbers_as_string_from_workspace_name
from Muon.GUI.Common.fitting_tab_widget.basic_fitting_presenter import BasicFittingPresenter
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_model import FitPlotInformation


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

    def get_model_fitting_options(self):
        """Returns the fitting options to be used when initializing the model."""
        fitting_options = super().get_model_fitting_options()
        fitting_options["fit_type"] = self._get_fit_type()
        fitting_options["fit_by"] = self.view.simultaneous_fit_by
        fitting_options["global_parameters"] = self.view.get_global_parameters()
        fitting_options["tf_asymmetry_mode"] = False  # TEMPORARY
        return fitting_options

    def get_loaded_workspaces(self):
        """Retrieve the names of the workspaces successfully loaded into the fitting interface."""
        return self.view.loaded_workspaces

    def _is_simultaneous_fitting(self):
        """Returns true if the fitting mode is simultaneous."""
        return self.view.is_simultaneous_fit_ticked

    def handle_selected_group_pair_changed(self):
        """Update the displayed workspaces when the selected group/pairs change in grouping tab."""
        self.update_selected_workspace_list_for_fit()
        self._update_stored_fit_functions()

    def handle_selected_plot_type_changed(self, _):
        """Update the selected workspace list when the selected plot has changed."""
        self.update_selected_workspace_list_for_fit()

    def handle_new_data_loaded(self):
        """Handle when new data has been loaded into the interface."""
        self.update_selected_workspace_list_for_fit()
        super().handle_new_data_loaded()

    def handle_fitting_finished(self, fit_function, fit_status, fit_chi_squared):
        """Handle when fitting is finished."""
        if self.view.is_simultaneous_fit_ticked:
            self.model.simultaneous_fit_function = fit_function
            #self._fit_function[0] = fit_function
            self._fit_status = [fit_status] * len(self.model.start_xs)
            self._fit_chi_squared = [fit_chi_squared] * len(self.model.start_xs)
        else:
            current_index = self.view.get_index_for_start_end_times()
            self.model.update_single_fit_function(current_index, fit_function)
            logger.warning(f"{fit_function}")
            logger.warning(f"{self.model.single_fit_functions[current_index]}")
            #self._fit_function[current_index] = fit_function
            self._fit_status[current_index] = fit_status
            self._fit_chi_squared[current_index] = fit_chi_squared

        self.update_fit_status_in_the_view()

        # Send the workspaces to be plotted
        self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())

        # Update parameter values in sequential tab.
        parameter_values = self.model.get_fit_function_parameter_values(fit_function)
        self.model.update_ws_fit_function_parameters(self.get_fit_input_workspaces(), parameter_values)
        self.fit_parameter_changed_notifier.notify_subscribers()

    def handle_plot_guess_changed(self):
        """Handle when plot guess is ticked or unticked."""
        index = self.view.get_index_for_start_end_times()
        workspaces = self.get_fit_input_workspaces()
        self.model.change_plot_guess(self.view.plot_guess, workspaces, index)

    def handle_undo_fit_clicked(self):
        """Handle when undo fit is clicked."""
        super().handle_undo_fit_clicked()
        self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())

    def handle_fitting_mode_changed(self):
        """Handle when the fitting mode is changed to or from simultaneous fitting."""
        self.view.enable_undo_fit(False)
        self.model.simultaneous_fitting_mode = self.view.is_simultaneous_fit_ticked

        if self.view.is_simultaneous_fit_ticked:
            self.view.set_workspace_combo_box_label("Display parameters for")
            self.view.enable_simultaneous_fit_options()
            self.view.switch_to_simultaneous()
            self._update_stored_fit_functions()
            self.update_fit_specifier_list()
        else:
            self.selected_data = self.get_workspace_selected_list()
            self.view.set_workspace_combo_box_label("Select Workspace")
            self.view.switch_to_single()
            self._update_stored_fit_functions()
            self.view.disable_simultaneous_fit_options()

        #self.model.current_domain_index = 0
        self.update_model_from_view(fit_type=self._get_fit_type(), fit_by=self.view.simultaneous_fit_by)
        #self.update_model_from_view(fit_function=self._fit_function[0], fit_type=self._get_fit_type(),
        #                            fit_by=self.view.simultaneous_fit_by)
        self.fitting_mode_changed_notifier.notify_subscribers()
        self.fit_function_changed_notifier.notify_subscribers()
        # Send the workspaces to be plotted
        self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())
        self.handle_plot_guess_changed()

    def handle_simultaneous_fit_by_changed(self):
        """Handle when the simultaneous fit by combo box is changed."""
        self.update_selected_workspace_list_for_fit()
        self.view.enable_simultaneous_fit_by_specifier(True)
        #self.model.current_domain_index = 0
        self.update_model_from_view(fit_by=self.view.simultaneous_fit_by)
        #self.update_model_from_view(fit_function=self._fit_function[0], fit_by=self.view.simultaneous_fit_by)

        self.fitting_mode_changed_notifier.notify_subscribers()
        self.fit_function_changed_notifier.notify_subscribers()
        self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())

    def handle_simultaneous_fit_by_specifier_changed(self):
        """Handle when the simultaneous fit by specifier combo box is changed."""
        self.selected_data = self.get_workspace_selected_list()
        self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())

    def handle_display_workspace_changed(self):
        """Handle when the display workspace combo box is changed."""
        current_index = self.view.get_index_for_start_end_times()

        self._update_stored_fit_functions()
        self.set_current_domain_index(current_index)

        self.view.start_time = self.model.current_start_x
        self.view.end_time = self.model.current_end_x

        self.update_fit_status_in_the_view()
        self.handle_plot_guess_changed()  # update the guess (use the selected workspace as data for the guess)
        self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())

    def handle_function_structure_changed(self):
        """Handle when the function structure is changed."""
        #if not self._fit_function[0]:
        #    self.handle_display_workspace_changed()
        self.view.plot_guess = False
        self._update_stored_fit_functions()
        if not self.view.fit_object:
            self.model.clear_fit_information()
            self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())

        self._reset_fit_status_information()

        self.automatically_update_function_name()

        self.update_model_from_view(global_parameters=self.view.get_global_parameters())

        self.fit_function_changed_notifier.notify_subscribers()

    def handle_function_parameter_changed(self):
        """Handle when the value of a parameter in a function is changed."""
        if not self.view.is_simultaneous_fit_ticked:
            fit_function = self._get_fit_functions_for_each_domain()[self.model.current_domain_index]
            self.model.current_single_fit_function = fit_function
        else:
            fit_function = self.view.fit_object
            self.model.simultaneous_fit_function = fit_function

        parameter_values = self.model.get_fit_function_parameter_values(fit_function)
        self.model.update_ws_fit_function_parameters(self.get_fit_input_workspaces(), parameter_values)
        self.fit_parameter_changed_notifier.notify_subscribers()
        self.model.update_plot_guess(self.get_fit_input_workspaces(), self.view.get_index_for_start_end_times())

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
        self.update_model_from_view(fit_to_raw=self.view.fit_to_raw)

    def clear_and_reset_gui_state(self):
        """Clears all data in the view and updates the model."""
        super().clear_and_reset_gui_state()
        self.view.update_displayed_data_combo_box(self.selected_data)

    def update_selected_workspace_list_for_fit(self):
        """Updates the simultaneous fit specifier and selected data."""
        if self.view.is_simultaneous_fit_ticked:
            self.update_fit_specifier_list()

        self.selected_data = self.get_workspace_selected_list()

    def set_display_workspace(self, workspace_name):
        """Set the display workspace programmatically."""
        self.view.display_workspace = workspace_name
        self.handle_display_workspace_changed()

    def get_workspace_selected_list(self):
        """Get the names of workspaces stored in the context for fitting."""
        freq = self.get_x_data_type()

        selected_runs, selected_groups_and_pairs = self._get_selected_runs_and_groups_for_fitting()
        selected_workspaces = []
        for group_and_pair in selected_groups_and_pairs:
            selected_workspaces += self.context.get_names_of_workspaces_to_fit(
                runs=selected_runs,
                group_and_pair=group_and_pair,
                rebin=not self.view.fit_to_raw, freq=freq)

        selected_workspaces = list(set(self._check_data_exists(selected_workspaces)))
        if len(selected_workspaces) > 1:  # sort the list to preserve order
            selected_workspaces.sort(key=self.model.workspace_list_sorter)

        return selected_workspaces

    def update_fit_specifier_list(self):
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
            simul_choices = self._get_selected_groups_and_pairs()
        else:
            simul_choices = []

        self.view.setup_fit_by_specifier(simul_choices)

    def update_fit_status_in_the_view(self):
        """Updates the fit status and chi squared information in the view."""
        current_index = self.view.get_index_for_start_end_times()
        if self.view.is_simultaneous_fit_ticked:
            self.update_fit_status_and_function_in_the_view(self.model.simultaneous_fit_function,
                                                            current_index)
        else:
            self.update_fit_status_and_function_in_the_view(self.model.single_fit_functions[current_index],
                                                            current_index)

    def _update_stored_fit_functions(self):
        """Updates the stored fit functions."""
        if self.view.is_simultaneous_fit_ticked:
            self.model.simultaneous_fit_function = self.view.fit_object
        else:
            self.model.single_fit_functions = self._get_fit_functions_for_each_domain()

    def _get_fit_function(self):
        """Get all of the fit functions stored within the fitting interface."""
        if self.view.is_simultaneous_fit_ticked:
            return self.view.fit_object
        else:
            return self._get_fit_functions_for_each_domain()

    def _get_fit_functions_for_each_domain(self):
        if self.view.fit_object:  # make sure there is a fit function in the browser
            if isinstance(self.view.fit_object, MultiDomainFunction):
                return [function.clone() for function in self.view.fit_object.createEquivalentFunctions()]
            return [self.view.fit_object]
        number_of_domains = self.view.number_of_domains()
        return [None] * number_of_domains if number_of_domains > 0 else [None]

    def _fit_function_index(self):
        """Returns the index of the currently displayed fit function."""
        if not self.view.is_simultaneous_fit_ticked:
            return self.view.get_index_for_start_end_times()
        return 0

    def _get_fit_type(self):
        """Returns the currently selected fitting mode."""
        if self.view.is_simultaneous_fit_ticked:
            fit_type = "Simul"
        else:
            fit_type = "Single"
        return fit_type

    def get_fit_input_workspaces(self):
        """Returns the workspaces that are to be fitted."""
        if self.view.is_simultaneous_fit_ticked:
            return self.selected_data
        return [self.view.display_workspace]

    def get_selected_fit_workspaces(self):
        """Returns the fitted workspaces."""
        if self.selected_data:
            if self._get_fit_type() == "Single":
                fit = self.context.fitting_context.find_fit_for_input_workspace_list_and_function(
                    [self.view.display_workspace], self.model.function_name)
                return [FitPlotInformation(input_workspaces=[self.view.display_workspace], fit=fit)]
            else:
                fit = self.context.fitting_context.find_fit_for_input_workspace_list_and_function(
                    self.selected_data, self.model.function_name)
                return [FitPlotInformation(input_workspaces=self.selected_data, fit=fit)]
        else:
            return []

    def _get_selected_groups_and_pairs(self):
        """Returns the selected groups and pairs."""
        return self.context.group_pair_context.selected_groups + self.context.group_pair_context.selected_pairs

    def _get_selected_runs_and_groups_for_fitting(self):
        """Returns the selected runs and groups to be used for fitting."""
        runs = "All"
        groups_and_pairs = self._get_selected_groups_and_pairs()
        if self.view.is_simultaneous_fit_ticked:
            if self.view.simultaneous_fit_by == "Run":
                runs = self.view.simultaneous_fit_by_specifier
            elif self.view.simultaneous_fit_by == "Group/Pair":
                groups_and_pairs = [self.view.simultaneous_fit_by_specifier]

        return runs, groups_and_pairs
