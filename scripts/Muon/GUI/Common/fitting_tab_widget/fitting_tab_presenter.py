# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from Muon.GUI.Common.fitting_tab_widget.workspace_selector_view import WorkspaceSelectorView
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing, GenericObservable
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput
from Muon.GUI.Common import thread_model
from Muon.GUI.Common.ADSHandler.workspace_naming import get_run_number_from_workspace_name, get_group_or_pair_from_name
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
        self.gui_context_observer = GenericObserverWithArgPassing(
            self.handle_gui_changes_made)
        self.selected_group_pair_observer = GenericObserver(
            self.handle_selected_group_pair_changed)
        self.selected_plot_type_observer = GenericObserverWithArgPassing(
            self.handle_selected_plot_type_changed)
        self.input_workspace_observer = GenericObserver(
            self.handle_new_data_loaded)
        self.disable_tab_observer = GenericObserver(lambda: self.view.
                                                    setEnabled(False))
        self.enable_tab_observer = GenericObserver(lambda: self.view.
                                                   setEnabled(True))

        self.update_view_from_model_observer = GenericObserverWithArgPassing(
            self.update_view_from_model)

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

    def handle_select_fit_data_clicked(self):
        selected_data, dialog_return = WorkspaceSelectorView.get_selected_data(
            self.context.data_context.current_runs,
            self.context.data_context.instrument, self.selected_data,
            self.view.fit_to_raw, self._plot_type, self.context, self.view)

        if dialog_return:
            self.selected_data = selected_data
            self.manual_selection_made = True

    def handle_new_data_loaded(self):
        self.manual_selection_made = False
        self.update_selected_workspace_list_for_fit()

    def handle_gui_changes_made(self, changed_values):
        for key in changed_values.keys():
            if key in ['FirstGoodDataFromFile', 'FirstGoodData']:
                self.reset_start_time_to_first_good_data_value()

    def handle_selected_group_pair_changed(self):
        self.update_selected_workspace_list_for_fit()

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

    def handle_fit_type_changed(self):
        self.view.undo_fit_button.setEnabled(False)

        if self.view.is_simul_fit():
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

        self.model.update_stored_fit_function(self._fit_function[0])
        self.fit_function_changed_notifier.notify_subscribers()

    def handle_plot_guess_changed(self):
        if self.view.is_simul_fit():
            parameters = self.get_multi_domain_fit_parameters()
            current_idx = self.view.get_index_for_start_end_times()
            if len(parameters['InputWorkspace']) > current_idx:
                parameters['InputWorkspace'] = parameters['InputWorkspace'][current_idx]
            if len(parameters['StartX']) > current_idx:
                parameters['StartX'] = parameters['StartX'][current_idx]
            if len(parameters['EndX']) > current_idx:
                parameters['EndX'] = parameters['EndX'][current_idx]
        else:
            parameters = self.get_parameters_for_single_fit()

        self.model.change_plot_guess(self.view.plot_guess, parameters)

    def handle_fit_clicked(self):
        self.context.fitting_context.number_of_fits = 0
        if self._tf_asymmetry_mode:
            self.perform_tf_asymmetry_fit()
        else:
            self.perform_standard_fit()

    def handle_started(self):
        self.view.setEnabled(False)
        self.thread_success = True

    def handle_finished(self):
        if not self.thread_success:
            return

        self.view.setEnabled(True)
        fit_function, fit_status, fit_chi_squared = self.fitting_calculation_model.result
        if any([not fit_function, not fit_status, not fit_chi_squared]):
            return
        if self.view.is_simul_fit():
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

    def handle_error(self, error):
        self.thread_success = False
        self.view.warning_popup(error)
        self.view.setEnabled(True)

    def handle_start_x_updated(self):
        value = self.view.start_time
        index = self.view.get_index_for_start_end_times()
        self.update_start_x(index, value)

    def handle_end_x_updated(self):
        value = self.view.end_time
        index = self.view.get_index_for_start_end_times()
        self.update_end_x(index, value)

    def handle_fit_name_changed_by_user(self):
        self.automatically_update_fit_name = False
        self.model.function_name = self.view.function_name

    def handle_function_structure_changed(self):
        if self._tf_asymmetry_mode:
            self.view.warning_popup('Cannot change function structure during tf asymmetry mode')
            self.view.function_browser.blockSignals(True)
            self.view.function_browser.setFunction(str(self._fit_function[self.view.get_index_for_start_end_times()]))
            self.view.function_browser.blockSignals(False)
            return
        if not self.view.fit_object:
            if self.view.is_simul_fit():
                self._fit_function = [None]
            else:
                self._fit_function = [None] * len(self.selected_data)
        else:
            self._fit_function = [func.clone() for func in self._get_fit_function()]

        self.clear_fit_information()
        if self.automatically_update_fit_name:
            name = self._get_fit_function()[0]
            self.view.function_name = self.model.get_function_name(name)
            self.model.function_name = self.view.function_name

        self.model.update_stored_fit_function(self._fit_function[0])

        self.fit_function_changed_notifier.notify_subscribers()

    def handle_tf_asymmetry_mode_changed(self):
        def calculate_tf_fit_function(original_fit_function):
            tf_asymmetry_parameters = self.get_parameters_for_tf_function_calculation(original_fit_function)

            try:
                tf_function = self.model.calculate_tf_function(tf_asymmetry_parameters)
            except RuntimeError:
                self.view.warning_popup('The input function was not of the form N*(1+f)+A*exp(-lambda*t)')
                return tf_asymmetry_parameters['InputFunction']
            return tf_function

        self.view.undo_fit_button.setEnabled(False)

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
            self.view.select_workspaces_to_fit_button.setEnabled(False)
            new_global_parameters = [str('f0.f1.f1.' + item) for item in global_parameters]
            if self.automatically_update_fit_name:
                self.view.function_name += ',TFAsymmetry'
                self.model.function_name = self.view.function_name
        else:
            self.view.select_workspaces_to_fit_button.setEnabled(True)
            new_global_parameters = [item[9:] for item in global_parameters]
            if self.automatically_update_fit_name:
                self.view.function_name = self.view.function_name.replace(',TFAsymmetry', '')
                self.model.function_name = self.view.function_name

        if not self.view.is_simul_fit():
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

    def handle_function_parameter_changed(self):
        if not self.view.is_simul_fit():
            index = self.view.get_index_for_start_end_times()
            self._fit_function[index] = self._get_fit_function()[index]
        else:
            self._fit_function = self._get_fit_function()

    def handle_undo_fit_clicked(self):
        self._fit_function = self._fit_function_cache
        self.clear_fit_information()
        self.update_fit_status_information_in_view()
        self.view.undo_fit_button.setEnabled(False)
        self.context.fitting_context.remove_latest_fit(self._number_of_fits_cached)
        self._number_of_fits_cached = 0

    def handle_fit_by_changed(self):
        self.manual_selection_made = False  # reset manual selection flag
        if self.view.is_simul_fit():
            self.update_selected_workspace_list_for_fit()
            if self.view.simultaneous_fit_by == "Custom":
                self.view.simul_fit_by_specifier.setEnabled(False)
                self.view.select_workspaces_to_fit_button.setEnabled(True)
            else:
                self.view.simul_fit_by_specifier.setEnabled(True)
                self.view.select_workspaces_to_fit_button.setEnabled(False)
        else:
            return

    def handle_fit_specifier_changed(self):
        self.selected_data = self.get_workspace_selected_list()

    def perform_standard_fit(self):
        if not self.view.fit_object:
            return

        self._fit_function_cache = [func.clone() for func in self._fit_function]

        try:
            if self.view.is_simul_fit():
                self._number_of_fits_cached += 1
                simultaneous_fit_parameters = self.get_multi_domain_fit_parameters()
                global_parameters = self.view.get_global_parameters()
                calculation_function = functools.partial(
                    self.model.do_simultaneous_fit,
                    simultaneous_fit_parameters, global_parameters)

                self.calculation_thread = self.create_thread(
                    calculation_function)
            else:
                self._number_of_fits_cached += 1
                single_fit_parameters = self.get_parameters_for_single_fit()
                calculation_function = functools.partial(
                    self.model.do_single_fit, single_fit_parameters)
                self.calculation_thread = self.create_thread(
                    calculation_function)

            self.calculation_thread.threadWrapperSetUp(self.handle_started,
                                                       self.handle_finished,
                                                       self.handle_error)
            self.calculation_thread.start()

        except ValueError as error:
            self.view.warning_popup(error)

    def perform_tf_asymmetry_fit(self):
        self._fit_function_cache = [item.clone() for item in self._fit_function if item]

        try:
            if self.view.is_simul_fit():
                simultaneous_fit_parameters = self.get_multi_domain_tf_fit_parameters()
                global_parameters = self.view.get_global_parameters()
                calculation_function = functools.partial(
                    self.model.do_simultaneous_tf_fit,
                    simultaneous_fit_parameters, global_parameters)
                self.calculation_thread = self.create_thread(calculation_function)
            else:
                single_fit_parameters = self.get_parameters_for_tf_single_fit_calculation()
                calculation_function = functools.partial(
                    self.model.do_single_tf_fit, single_fit_parameters)
                self.calculation_thread = self.create_thread(calculation_function)

            self.calculation_thread.threadWrapperSetUp(self.handle_started,
                                                       self.handle_finished,
                                                       self.handle_error)
            self.calculation_thread.start()
        except ValueError as error:
            self.view.warning_popup(error)

    def get_parameters_for_tf_single_fit_calculation(self):
        workspace, workspace_directory = self.model.create_fitted_workspace_name(self.view.display_workspace,
                                                                                 self.view.fit_object)

        return {
            'InputFunction': self._current_fit_function(),
            'ReNormalizedWorkspaceList': self.view.display_workspace,
            'UnNormalizedWorkspaceList': self.context.group_pair_context.get_unormalisised_workspace_list(
                [self.view.display_workspace])[0],
            'OutputFitWorkspace': workspace,
            'StartX': self.start_x[0],
            'EndX': self.end_x[0],
            'Minimizer': self.view.minimizer
        }

    def get_multi_domain_tf_fit_parameters(self):
        workspace, workspace_directory = self.model.create_multi_domain_fitted_workspace_name(
            self.view.display_workspace, self.view.fit_object)
        return {
            'InputFunction': self.view.fit_object,
            'ReNormalizedWorkspaceList': self.selected_data,
            'UnNormalizedWorkspaceList': self.context.group_pair_context.get_unormalisised_workspace_list(
                self.selected_data),
            'OutputFitWorkspace': workspace,
            'StartX': self.start_x[self.view.get_index_for_start_end_times()],
            'EndX': self.end_x[self.view.get_index_for_start_end_times()],
            'Minimizer': self.view.minimizer
        }

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
        self.update_fit_status_information_in_view()

    def clear_fit_information(self):
        self._fit_status = [None] * len(
            self.selected_data) if self.selected_data else [None]
        self._fit_chi_squared = [0.0] * len(
            self.selected_data) if self.selected_data else [0.0]
        self.update_fit_status_information_in_view()
        self.view.undo_fit_button.setEnabled(False)

    def update_selected_workspace_list_for_fit(self):
        if self.view.is_simul_fit():
            if self.manual_selection_made:
                return  # if it is a manual selection then the data should not change
            self.update_fit_specifier_list()
        else:
            self.selected_data = self.get_workspace_selected_list()

    def get_workspace_selected_list(self):
        if self.context._frequency_context is not None:
            freq = self.context._frequency_context.plot_type
        else:
            freq = 'None'

        selected_runs, selected_groups_and_pairs = self._get_selected_runs_and_groups_for_fitting()
        selected_workspaces = []
        for grp_and_pair in selected_groups_and_pairs:
            selected_workspaces += self.context.get_names_of_workspaces_to_fit(
                runs=selected_runs,
                group_and_pair=grp_and_pair,
                phasequad=False,
                rebin=not self.view.fit_to_raw, freq=freq)

        selected_workspaces = list(set(self._check_data_exists(selected_workspaces)))
        if len(selected_workspaces) > 1:  # sort the list to preserve order
            selected_workspaces.sort(key=self._workspace_list_sorter)

        return selected_workspaces

    def update_fit_specifier_list(self):
        if self.view.simultaneous_fit_by == "Run":
            # extract runs from run list of lists, which is in the format [ [run,...,runs],[runs],...,[runs] ]
            flattened_run_list = [str(item) for sublist in self.context.data_context.current_runs for item in sublist]
            flattened_run_list.sort()
            simul_choices = flattened_run_list
        elif self.view.simultaneous_fit_by == "Group/Pair":
            simul_choices = self._get_selected_groups_and_pairs()
        else:
            simul_choices = self.selected_data

        self.view.setup_fit_by_specifier(simul_choices)

    def get_parameters_for_single_fit(self):
        params = self._get_shared_parameters()
        params['InputWorkspace'] = self.view.display_workspace
        params['StartX'] = self.start_x[self._fit_function_index()]
        params['EndX'] = self.end_x[self._fit_function_index()]

        return params

    def get_multi_domain_fit_parameters(self):
        params = self._get_shared_parameters()
        params['InputWorkspace'] = self.selected_data
        params['StartX'] = self.start_x
        params['EndX'] = self.end_x

        return params

    def get_simul_fit_workspaces(self):
        selected_data = self.selected_data
        fit_workspaces = []
        if self.view.simultaneous_fit_by == "Run":
            selected_run = self.view.simultaneous_fit_by_specifier
            for workspace in selected_data:
                if selected_run == get_run_number_from_workspace_name(workspace, self.context.data_context.instrument):
                    fit_workspaces.append(workspace)
        else:
            pass
        return fit_workspaces

    def _get_shared_parameters(self):
        """
        :return: The set of attributes common to all fit types
        """
        return {
            'Function': self._current_fit_function(),
            'Minimizer': self.view.minimizer,
            'EvaluationType': self.view.evaluation_type
        }

    def _update_stored_fit_functions(self):
        if self.view.is_simul_fit():
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
        if self.view.is_simul_fit():
            return [self.view.fit_object]  # return the fit function stored in the browser
        else:  # we need to convert stored function into equiv
            if self.view.fit_object:  # make sure thers a fit function in the browser
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
        if self.view.is_simul_fit():
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
        self.view.update_global_fit_state(self._fit_status)

    def update_view_from_model(self, workspace_removed=None):
        if workspace_removed:
            self.selected_data = [
                item for item in self.selected_data if item != workspace_removed]
        else:
            self.selected_data = []

    def get_parameters_for_tf_function_calculation(self, fit_function):
        mode = 'Construct' if self.view.tf_asymmetry_mode else 'Extract'
        workspace_list = self.selected_data if self.view.is_simul_fit() else [
            self.view.display_workspace]
        return {'InputFunction': fit_function,
                'WorkspaceList': workspace_list,
                'Mode': mode}

    def _get_selected_groups_and_pairs(self):
        return self.context.group_pair_context.selected_groups + self.context.group_pair_context.selected_pairs

    def _get_selected_runs_and_groups_for_fitting(self):
        runs = 'All'
        groups_and_pairs = self._get_selected_groups_and_pairs()
        if self.view.is_simul_fit():
            if self.view.simultaneous_fit_by == "Run":
                runs = self.view.simultaneous_fit_by_specifier
            elif self.view.simultaneous_fit_by == "Group/Pair":
                groups_and_pairs = [self.view.simultaneous_fit_by_specifier]

        return runs, groups_and_pairs

    def _transform_grp_or_pair_to_float(self, workspace_name):
        grp_or_pair_name = get_group_or_pair_from_name(workspace_name)
        if grp_or_pair_name not in self._grppair_index:
            self._grppair_index[grp_or_pair_name] = len(self._grppair_index)

        grp_pair_values = list(self._grppair_index.values())
        if len(self._grppair_index) > 1:
            return ((self._grppair_index[grp_or_pair_name] - grp_pair_values[0]) /
                    (grp_pair_values[-1] - grp_pair_values[0])) * 0.99
        else:

            return 0.99

    def _workspace_list_sorter(self, workspace_name):
        run_number = get_run_number_from_workspace_name(workspace_name, self.context.data_context.instrument)
        grp_pair_number = self._transform_grp_or_pair_to_float(workspace_name)
        return int(run_number) + grp_pair_number

    @staticmethod
    def check_workspaces_are_tf_asymmetry_compliant(workspace_list):
        non_compliant_workspaces = [item for item in workspace_list if 'Group' not in item]
        return False if non_compliant_workspaces else True

    @staticmethod
    def _check_data_exists(guess_selection):
        return [item for item in guess_selection if AnalysisDataService.doesExist(item)]
