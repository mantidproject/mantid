# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.ADSHandler.workspace_naming import get_run_numbers_as_string_from_workspace_name
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_model import FitPlotInformation
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing, GenericObservable
from mantidqt.widgets.fitscriptgenerator import (FittingMode, FitScriptGeneratorModel, FitScriptGeneratorPresenter,
                                                 FitScriptGeneratorView)
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput
from Muon.GUI.Common.contexts.frequency_domain_analysis_context import FrequencyDomainAnalysisContext
from Muon.GUI.Common import thread_model
from mantid.api import MultiDomainFunction, AnalysisDataService
import functools
import re


class FittingTabPresenter(object):

    def __init__(self, view, model, context):
        self.view = view
        self.model = model
        self.context = context
        self._selected_data = []
        self._start_x = [self.view.start_time]
        self._end_x = [self.view.end_time]
        self._grppair_index = {}
        self._fit_status = [None]
        self._fit_chi_squared = [0.0]
        self._fit_function = [None]
        self._tf_asymmetry_mode = False
        self._fit_function_cache = [None]
        self._plot_type = None
        self._number_of_fits_cached = 0
        self._multi_domain_function = None
        self.manual_selection_made = False
        self.automatically_update_fit_name = True
        self.thread_success = True
        self.fitting_calculation_model = None
        self.update_selected_workspace_list_for_fit()
        self.fit_function_changed_notifier = GenericObservable()
        self.fit_parameter_changed_notifier = GenericObservable()
        self.fit_type_changed_notifier = GenericObservable()
        self.selected_single_fit_notifier = GenericObservable()

        self.gui_context_observer = GenericObserverWithArgPassing(
            self.handle_gui_changes_made)
        self.selected_group_pair_observer = GenericObserver(
            self.handle_selected_group_pair_changed)
        self.selected_plot_type_observer = GenericObserverWithArgPassing(
            self.handle_selected_plot_type_changed)
        self.input_workspace_observer = GenericObserver(
            self.handle_new_data_loaded)

        self.disable_tab_observer = GenericObserver(self.disable_view)
        self.enable_tab_observer = GenericObserver(self.enable_view)
        self.instrument_changed_observer = GenericObserver(self.instrument_changed)

        self.update_view_from_model_observer = GenericObserverWithArgPassing(
            self.update_view_from_model)

        self.initialise_model_options()
        self.double_pulse_observer = GenericObserverWithArgPassing(
            self.handle_double_pulse_set)
        self.model.context.gui_context.add_non_calc_subscriber(self.double_pulse_observer)

        self.view.setEnabled(False)

        self.enable_editing_notifier = GenericObservable()
        self.disable_editing_notifier = GenericObservable()

        self.fsg_model = None
        self.fsg_view = None
        self.fsg_presenter = None

    def disable_view(self):
        self.update_selected_workspace_list_for_fit()
        if not self.selected_data:
            self.view.setEnabled(False)

    def enable_view(self):
        if self.selected_data:
            self.view.setEnabled(True)

    @property
    def selected_data(self):
        return self._selected_data

    @selected_data.setter
    def selected_data(self, selected_data):
        if self._selected_data == selected_data:
            return

        self._selected_data = selected_data
        self.clear_and_reset_gui_state()

    @property
    def start_x(self):
        return self._start_x

    @property
    def end_x(self):
        return self._end_x

    def handle_fit_generator_clicked(self):
        fitting_mode = FittingMode.SIMULTANEOUS if self.view.is_simul_fit else FittingMode.SEQUENTIAL
        fit_options = {"Minimizer": self.view.minimizer, "EvaluationType": self.view.evaluation_type}

        self.fsg_model = FitScriptGeneratorModel()
        self.fsg_view = FitScriptGeneratorView(None, fitting_mode, fit_options)
        self.fsg_presenter = FitScriptGeneratorPresenter(self.fsg_view, self.fsg_model, self.view.loaded_workspaces,
                                                         self.view.start_time, self.view.end_time)

        self.fsg_presenter.openFitScriptGenerator()

    def handle_new_data_loaded(self):
        self.manual_selection_made = False
        self.view.plot_guess_checkbox.setChecked(False)
        self.update_selected_workspace_list_for_fit()
        self.model.create_ws_fit_function_map()
        if self.selected_data:
            self.view.setEnabled(True)

    def handle_gui_changes_made(self, changed_values):
        for key in changed_values.keys():
            if key in ['FirstGoodDataFromFile', 'FirstGoodData']:
                self.reset_start_time_to_first_good_data_value()

    def handle_selected_group_pair_changed(self):
        self.update_selected_workspace_list_for_fit()
        self._update_stored_fit_functions()

    def handle_selected_plot_type_changed(self, plot_type):
        self._plot_type = plot_type
        self.update_selected_workspace_list_for_fit()

    def handle_display_workspace_changed(self):
        current_index = self.view.get_index_for_start_end_times()
        self.view.start_time = self.start_x[current_index]
        self.view.end_time = self.end_x[current_index]
        self.view.function_browser.setCurrentDataset(current_index)
        self._update_stored_fit_functions()
        self.update_fit_status_information_in_view()
        self.handle_plot_guess_changed()  # update the guess (use the selected workspace as data for the guess)
        self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())

    def handle_use_rebin_changed(self):
        if not self.view.fit_to_raw and not self.context._do_rebin():
            self.view.fit_to_raw = True
            self.view.warning_popup('No rebin options specified')
            return

        if not self.manual_selection_made:
            self.update_selected_workspace_list_for_fit()
        else:
            self.selected_data = self.context.get_list_of_binned_or_unbinned_workspaces_from_equivalents(
                self.selected_data)
        self.context.fitting_context.fit_raw = self.view.fit_to_raw
        self.update_model_from_view(fit_to_raw=self.view.fit_to_raw)

    def handle_fit_type_changed(self):
        if self.view.tf_asymmetry_mode:
            self.view.warning_popup("Cannot change Fitting Mode while TF Asymmetry Mode is checked.")
            self.view.is_simul_fit = not self.view.is_simul_fit
            return

        self.view.undo_fit_button.setEnabled(False)
        if self.view.is_simul_fit:
            self.view.workspace_combo_box_label.setText(
                'Display parameters for')
            self.view.enable_simul_fit_options()
            self.view.switch_to_simultaneous()
            self._update_stored_fit_functions()
            self.update_fit_specifier_list()
        else:
            self.selected_data = self.get_workspace_selected_list()
            self.view.workspace_combo_box_label.setText('Select Workspace')
            self.view.switch_to_single()
            self._update_stored_fit_functions()
            self.view.disable_simul_fit_options()

        self.update_model_from_view(fit_function=self._fit_function[0], fit_type=self._get_fit_type(),
                                    fit_by=self.view.simultaneous_fit_by)
        self.fit_type_changed_notifier.notify_subscribers()
        self.fit_function_changed_notifier.notify_subscribers()
        # Send the workspaces to be plotted
        self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())
        self.handle_plot_guess_changed()

    def handle_plot_guess_changed(self):
        index = self.view.get_index_for_start_end_times()
        workspaces = self.get_fit_input_workspaces()
        self.model.change_plot_guess(self.view.plot_guess, workspaces, index)

    def handle_fit_clicked(self):
        if len(self.selected_data) < 1:
            self.view.warning_popup('No data selected to fit')
            return
        # Some attributes of fit functions do not cause an update to the model
        # Do one final update to ensure using most recent values
        if self._fit_function != self._get_fit_function():
            self._fit_function = self._get_fit_function()
            self.update_model_from_view(fit_function=self._fit_function[0])
        self.perform_fit()

    def handle_started(self):
        self.disable_editing_notifier.notify_subscribers()
        self.thread_success = True

    def handle_finished(self):
        self.enable_editing_notifier.notify_subscribers()
        if not self.thread_success:
            return

        fit_function, fit_status, fit_chi_squared = self.fitting_calculation_model.result
        if any([not fit_function, not fit_status, not fit_chi_squared]):
            return
        if self.view.is_simul_fit:
            self._fit_function[0] = fit_function
            self._fit_status = [fit_status] * len(self.start_x)
            self._fit_chi_squared = [fit_chi_squared] * len(self.start_x)
        else:
            current_index = self.view.get_index_for_start_end_times()
            self._fit_function[current_index] = fit_function
            self._fit_status[current_index] = fit_status
            self._fit_chi_squared[current_index] = fit_chi_squared

        self.update_fit_status_information_in_view()
        self.view.undo_fit_button.setEnabled(True)
        self.view.plot_guess_checkbox.setChecked(False)
        # Send the workspaces to be plotted
        self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())
        # Update parameter values in sequential tab.
        parameter_values = self.model.get_fit_function_parameter_values(fit_function)
        self.model.update_ws_fit_function_parameters(self.get_fit_input_workspaces(), parameter_values)
        self.fit_parameter_changed_notifier.notify_subscribers()

    def handle_error(self, error):
        self.enable_editing_notifier.notify_subscribers()
        self.thread_success = False
        self.view.warning_popup(error)

    def handle_start_x_updated(self):
        value = self.view.start_time
        index = self.view.get_index_for_start_end_times()
        # Check start is greater than end, swap if need be
        if value > self.end_x[index]:
            self.view.start_time, self.view.end_time = self.end_x[index], value
            self.update_end_x(index, value)
            self.update_model_from_view(endX=value)
            value = self.view.start_time

        self.update_start_x(index, value)
        self.update_model_from_view(startX=value)

    def handle_end_x_updated(self):
        value = self.view.end_time
        index = self.view.get_index_for_start_end_times()
        # Check end is less than start, swap if need be
        if value < self.start_x[index]:
            self.view.start_time, self.view.end_time = value, self.start_x[index]
            self.update_start_x(index, value)
            self.update_model_from_view(startX=value)
            value = self.view.end_time

        self.update_end_x(index, value)
        self.update_model_from_view(endX=value)

    def handle_minimiser_changed(self):
        self.update_model_from_view(minimiser=self.view.minimizer)

    def handle_evaluation_type_changed(self):
        self.update_model_from_view(evaluation_type=self.view.evaluation_type)

    def handle_fit_name_changed_by_user(self):
        self.automatically_update_fit_name = False
        self.model.function_name = self.view.function_name

    def handle_function_structure_changed(self):
        if not self._fit_function[0]:
            self.handle_display_workspace_changed()
        self.view.plot_guess_checkbox.setChecked(False)
        if self._tf_asymmetry_mode:
            self.view.warning_popup('Cannot change function structure during tf asymmetry mode')
            self.view.function_browser.blockSignals(True)
            self.view.function_browser.setFunction(str(self._fit_function[self.view.get_index_for_start_end_times()]))
            self.view.function_browser.blockSignals(False)
            return
        if not self.view.fit_object:
            if self.view.is_simul_fit:
                self._fit_function = [None]
            else:
                self._fit_function = [None] * len(self.selected_data)\
                    if self.selected_data else [None]
            self.model.clear_fit_information()
            self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())
        else:
            self._fit_function = [func.clone() for func in self._get_fit_function()]

        self.clear_fit_information()

        if self.automatically_update_fit_name:
            name = self._get_fit_function()[0]
            self.view.function_name = self.model.get_function_name(name)
            self.model.function_name = self.view.function_name

        self.update_model_from_view(fit_function=self._fit_function[0],
                                    global_parameters=self.view.get_global_parameters())

        self.fit_function_changed_notifier.notify_subscribers()

    def handle_double_pulse_set(self, updated_variables):
        if 'DoublePulseEnabled' in updated_variables:
            self.view.tf_asymmetry_mode = False

    def handle_tf_asymmetry_mode_changed(self):
        def calculate_tf_fit_function(original_fit_function):
            tf_asymmetry_parameters = self.get_parameters_for_tf_function_calculation(original_fit_function)
            try:
                tf_function = self.model.convert_to_tf_function(tf_asymmetry_parameters)
            except RuntimeError:
                self.view.warning_popup('The input function was not of the form N*(1+f)+A*exp(-lambda*t)')
                return tf_asymmetry_parameters['InputFunction']
            return tf_function

        self.view.undo_fit_button.setEnabled(False)
        self.view.plot_guess_checkbox.setChecked(False)

        groups_only = self.check_workspaces_are_tf_asymmetry_compliant(self.selected_data)
        if (
                not groups_only and self.view.tf_asymmetry_mode) or not self.view.fit_object and self.view.tf_asymmetry_mode:
            self.view.tf_asymmetry_mode = False

            self.view.warning_popup('Can only fit groups in tf asymmetry mode and need a function defined')
            return

        if self._tf_asymmetry_mode == self.view.tf_asymmetry_mode:
            return

        self._tf_asymmetry_mode = self.view.tf_asymmetry_mode
        global_parameters = self.view.get_global_parameters()
        if self._tf_asymmetry_mode:
            new_global_parameters = [str('f0.f1.f1.' + item) for item in global_parameters]
            if self.automatically_update_fit_name:
                self.view.function_name += ',TFAsymmetry'
                self.model.function_name = self.view.function_name
        else:
            new_global_parameters = [item[9:] for item in global_parameters]
            if self.automatically_update_fit_name:
                self.view.function_name = self.view.function_name.replace(',TFAsymmetry', '')
                self.model.function_name = self.view.function_name

        if not self.view.is_simul_fit:
            for index, fit_function in enumerate(self._fit_function):
                fit_function = fit_function if fit_function else self.view.fit_object.clone()
                new_function = calculate_tf_fit_function(fit_function)
                self._fit_function[index] = new_function.clone()

            self.view.function_browser.blockSignals(True)
            self.view.function_browser.clear()
            self.view.function_browser.setFunction(str(self._fit_function[self.view.get_index_for_start_end_times()]))
            self.view.function_browser.setGlobalParameters(new_global_parameters)
            self.view.function_browser.blockSignals(False)
        else:
            new_function = calculate_tf_fit_function(self.view.fit_object)
            self._fit_function = [new_function.clone()]
            self.view.function_browser.blockSignals(True)
            self.view.function_browser.clear()
            self.view.function_browser.setFunction(str(self._fit_function[0]))
            self.view.function_browser.setGlobalParameters(new_global_parameters)
            self.view.function_browser.blockSignals(False)

        self.update_fit_status_information_in_view()
        self.handle_display_workspace_changed()
        self.update_model_from_view(fit_function=self._fit_function[0], tf_asymmetry_mode=self.view.tf_asymmetry_mode)
        self.fit_function_changed_notifier.notify_subscribers()

    def get_parameters_for_tf_function_calculation(self, fit_function):
        mode = 'Construct' if self.view.tf_asymmetry_mode else 'Extract'
        workspace_list = self.selected_data if self.view.is_simul_fit else [self.view.display_workspace]
        return {'InputFunction': fit_function,
                'WorkspaceList': workspace_list,
                'Mode': mode,
                'CopyTies': False}

    def handle_function_parameter_changed(self):
        if not self.view.is_simul_fit:
            index = self.view.get_index_for_start_end_times()
            fit_function = self._get_fit_function()[index]
            self._fit_function[index] = self._get_fit_function()[index]
        else:
            fit_function = self._get_fit_function()[0]
            self._fit_function = self._get_fit_function()

        parameter_values = self.model.get_fit_function_parameter_values(fit_function)
        self.model.update_ws_fit_function_parameters(self.get_fit_input_workspaces(), parameter_values)
        self.fit_parameter_changed_notifier.notify_subscribers()
        self.model.update_plot_guess(self.get_fit_input_workspaces(), self.view.get_index_for_start_end_times())

    def handle_undo_fit_clicked(self):
        self._fit_function = self._fit_function_cache
        self._fit_function = self._fit_function_cache
        self.clear_fit_information()
        self.update_fit_status_information_in_view()
        self.view.undo_fit_button.setEnabled(False)
        self.context.fitting_context.remove_latest_fit()
        self._number_of_fits_cached = 0
        self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())

    def handle_fit_by_changed(self):
        if self.view.tf_asymmetry_mode:
            self.view.warning_popup("Cannot change Run - Group/Pair selection while TF Asymmetry Mode is checked.")
            self.view.simultaneous_fit_by = "Run" if self.view.simultaneous_fit_by == "Group/Pair" else "Group/Pair"
            return

        self.manual_selection_made = False  # reset manual selection flag
        self.update_selected_workspace_list_for_fit()
        self.view.simul_fit_by_specifier.setEnabled(True)
        self.update_model_from_view(fit_function=self._fit_function[0], fit_by=self.view.simultaneous_fit_by)
        self.fit_type_changed_notifier.notify_subscribers()
        self.fit_function_changed_notifier.notify_subscribers()
        # Send the workspaces to be plotted
        self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())

    def handle_fit_specifier_changed(self):
        self.selected_data = self.get_workspace_selected_list()
        # Send the workspaces to be plotted
        self.selected_single_fit_notifier.notify_subscribers(self.get_selected_fit_workspaces())

    # Perform fit
    def perform_fit(self):
        if not self.view.fit_object:
            return
        self._fit_function_cache = [func.clone() for func in self._fit_function]
        try:
            workspaces = self.get_fit_input_workspaces()
            self._number_of_fits_cached += 1
            calculation_function = functools.partial(
                self.model.evaluate_single_fit, workspaces)
            self.calculation_thread = self.create_thread(
                calculation_function)
            self.calculation_thread.threadWrapperSetUp(self.handle_started,
                                                       self.handle_finished,
                                                       self.handle_error)
            self.calculation_thread.start()

        except ValueError as error:
            self.view.warning_popup(error)

    # Query view and update model.
    def clear_and_reset_gui_state(self):
        self.view.set_datasets_in_function_browser(self.selected_data)

        self._fit_status = [None] * len(
            self.selected_data) if self.selected_data else [None]
        self._fit_chi_squared = [0.0] * len(
            self.selected_data) if self.selected_data else [0.0]
        if self.view.fit_object:
            self._fit_function = [func.clone() for func in self._get_fit_function()]
        else:
            self._fit_function = [None] * len(self.selected_data) if self.selected_data else [None]

        self.view.undo_fit_button.setEnabled(False)

        self.reset_start_time_to_first_good_data_value()
        self.view.update_displayed_data_combo_box(self.selected_data)
        fitting_options = {"fit_function": self._fit_function[0], "startX": self.start_x[0], "endX": self.end_x[0]}
        self.update_model_from_view(**fitting_options)
        self.update_fit_status_information_in_view()

    def clear_fit_information(self):
        self._fit_status = [None] * len(
            self.selected_data) if self.selected_data else [None]
        self._fit_chi_squared = [0.0] * len(
            self.selected_data) if self.selected_data else [0.0]
        self.update_fit_status_information_in_view()
        self.view.undo_fit_button.setEnabled(False)

    def update_selected_workspace_list_for_fit(self):
        if self.view.is_simul_fit:
            if self.manual_selection_made:
                return  # if it is a manual selection then the data should not change
            self.update_fit_specifier_list()

        self.selected_data = self.get_workspace_selected_list()

    def set_display_workspace(self, workspace_name):
        self.view.display_workspace = workspace_name
        self.handle_display_workspace_changed()

    def get_workspace_selected_list(self):
        if isinstance(self.context, FrequencyDomainAnalysisContext):
            freq = self.context._frequency_context.plot_type
        else:
            freq = 'None'

        selected_runs, selected_groups_and_pairs = self._get_selected_runs_and_groups_for_fitting()
        selected_workspaces = []
        for grp_and_pair in selected_groups_and_pairs:
            selected_workspaces += self.context.get_names_of_workspaces_to_fit(
                runs=selected_runs,
                group_and_pair=grp_and_pair,
                rebin=not self.view.fit_to_raw, freq=freq)

        selected_workspaces = list(set(self._check_data_exists(selected_workspaces)))
        if len(selected_workspaces) > 1:  # sort the list to preserve order
            selected_workspaces.sort(key=self.model.workspace_list_sorter)

        return selected_workspaces

    def update_fit_specifier_list(self):
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

    def _update_stored_fit_functions(self):
        if self.view.is_simul_fit:
            if self.view.fit_object:
                self._fit_function = [self.view.fit_object.clone()]
            else:
                self._fit_function = [None]
        else:  # we need to convert stored function into equivalent function
            if self.view.fit_object:  # make sure there is a fit function in the browser
                if isinstance(self.view.fit_object, MultiDomainFunction):
                    equiv_fit_function = self.view.fit_object.createEquivalentFunctions()
                    single_domain_fit_functions = [func.clone() for func in equiv_fit_function]
                else:
                    single_domain_fit_functions = [self.view.fit_object.clone()]
                self._fit_function = single_domain_fit_functions
            else:
                self._fit_function = [None] * len(self._start_x)

    def _get_fit_function(self):
        if self.view.is_simul_fit:
            return [self.view.fit_object]  # return the fit function stored in the browser
        else:  # we need to convert stored function into equiv
            if self.view.fit_object:  # make sure there's a fit function in the browser
                if isinstance(self.view.fit_object, MultiDomainFunction):
                    equiv_fit_funtion = self.view.fit_object.createEquivalentFunctions()
                    single_domain_fit_function = equiv_fit_funtion
                else:
                    single_domain_fit_function = [self.view.fit_object]
                return single_domain_fit_function
            else:
                return [None] * len(self._start_x)

    def _current_fit_function(self):
        return self._fit_function[self._fit_function_index()]

    def _fit_function_index(self):
        if self.view.is_simul_fit:
            return 0  # if we are doing a single simultaneous fit return 0
        else:  # else fitting on one of the display workspaces
            return self.view.get_index_for_start_end_times()

    def update_start_x(self, index, value):
        self._start_x[index] = value

    def update_end_x(self, index, value):
        self._end_x[index] = value

    def create_thread(self, callback):
        self.fitting_calculation_model = ThreadModelWrapperWithOutput(callback)
        return thread_model.ThreadModel(self.fitting_calculation_model)

    def retrieve_first_good_data_from_run_name(self, workspace_name):
        try:
            run = [float(re.search('[0-9]+', workspace_name).group())]
        except AttributeError:
            return 0.0

        return self.context.first_good_data(run)

    def reset_start_time_to_first_good_data_value(self):
        self._start_x = [self.retrieve_first_good_data_from_run_name(run_name) for run_name in self.selected_data] if \
            self.selected_data else [0.0]
        self._end_x = [self.view.end_time] * len(
            self.selected_data) if self.selected_data else [15.0]
        self.view.start_time = self.start_x[0] if 0 < len(
            self.start_x) else 0.0
        self.view.end_time = self.end_x[0] if 0 < len(self.end_x) else 15.0

    def update_fit_status_information_in_view(self):
        current_index = self._fit_function_index()
        self.view.update_with_fit_outputs(self._fit_function[current_index],
                                          self._fit_status[current_index],
                                          self._fit_chi_squared[current_index])
        self.view.update_global_fit_state(self._fit_status, current_index)

    def update_view_from_model(self, workspace_removed=None):
        if workspace_removed:
            self.selected_data = [
                item for item in self.selected_data if item != workspace_removed]
        else:
            self.selected_data = []

    def update_model_from_view(self, **kwargs):
        self.model.update_model_fit_options(**kwargs)

    def initialise_model_options(self):
        fitting_options = {"fit_function": self._fit_function[0], "startX": self.start_x[0], "endX": self.end_x[0],
                           "minimiser": self.view.minimizer, "evaluation_type": self.view.evaluation_type,
                           "fit_to_raw": self.view.fit_to_raw, "fit_type": self._get_fit_type(),
                           "fit_by": self.view.simultaneous_fit_by,
                           "global_parameters": self.view.get_global_parameters(),
                           "tf_asymmetry_mode": self.view.tf_asymmetry_mode}
        self.model.update_model_fit_options(**fitting_options)

    def _get_fit_type(self):
        if self.view.is_simul_fit:
            fit_type = "Simul"
        else:
            fit_type = "Single"
        return fit_type

    def get_fit_input_workspaces(self):
        if self.view.is_simul_fit:
            return self.selected_data
        else:
            return [self.view.display_workspace]

    def get_selected_fit_workspaces(self):
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

    def instrument_changed(self):
        self.view.tf_asymmetry_mode = False

    def _get_selected_groups_and_pairs(self):
        return self.context.group_pair_context.selected_groups_and_pairs

    def _get_selected_runs_and_groups_for_fitting(self):
        runs = 'All'
        groups_and_pairs = self._get_selected_groups_and_pairs()
        if self.view.is_simul_fit:
            if self.view.simultaneous_fit_by == "Run":
                runs = self.view.simultaneous_fit_by_specifier
            elif self.view.simultaneous_fit_by == "Group/Pair":
                groups_and_pairs = [self.view.simultaneous_fit_by_specifier]

        return runs, groups_and_pairs

    @staticmethod
    def check_workspaces_are_tf_asymmetry_compliant(workspace_list):
        non_compliant_workspaces = [item for item in workspace_list if 'Group' not in item]
        return False if non_compliant_workspaces else True

    @staticmethod
    def _check_data_exists(guess_selection):
        return [item for item in guess_selection if AnalysisDataService.doesExist(item)]
