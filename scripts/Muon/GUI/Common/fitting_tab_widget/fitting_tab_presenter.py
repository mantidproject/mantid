# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from Muon.GUI.Common.fitting_tab_widget.workspace_selector_view import WorkspaceSelectorView
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput
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
        self._fit_status = [None]
        self._fit_chi_squared = [0.0]
        self._fit_function = [None]
        self._tf_asymmetry_mode = False
        self._fit_function_cache = [None]
        self._number_of_fits_cached = 0
        self._multi_domain_function = None
        self.manual_selection_made = False
        self.automatically_update_fit_name = True
        self.thread_success = True
        self.update_selected_workspace_guess()
        self.gui_context_observer = GenericObserverWithArgPassing(
            self.handle_gui_changes_made)
        self.selected_group_pair_observer = GenericObserver(
            self.handle_selected_group_pair_changed)
        self.selected_plot_type_observer = GenericObserver(
            self.handle_selected_plot_type_changed)
        self.input_workspace_observer = GenericObserver(
            self.handle_new_data_loaded)
        self.disable_tab_observer = GenericObserver(lambda: self.view.
                                                    setEnabled(False))
        self.enable_tab_observer = GenericObserver(lambda: self.view.
                                                   setEnabled(True))

        self.update_view_from_model_observer = GenericObserverWithArgPassing(
            self.update_view_from_model)

    def handle_select_fit_data_clicked(self):
        selected_data, dialog_return = WorkspaceSelectorView.get_selected_data(
            self.context.data_context.current_runs,
            self.context.data_context.instrument, self.selected_data,
            self.view.fit_to_raw, self.context, self.view)

        if dialog_return:
            self.selected_data = selected_data
            self.manual_selection_made = True

    def handle_new_data_loaded(self):
        self.manual_selection_made = False
        self.update_selected_workspace_guess()

    def handle_gui_changes_made(self, changed_values):
        for key in changed_values.keys():
            if key in ['FirstGoodDataFromFile', 'FirstGoodData']:
                self.reset_start_time_to_first_good_data_value()

    def handle_selected_group_pair_changed(self):
        self.update_selected_workspace_guess()

    def handle_selected_plot_type_changed(self):
        self.update_selected_workspace_guess()

    def update_selected_workspace_guess(self):
        if self.view.fit_type == self.view.simultaneous_fit:
            self.update_fit_selector_list()
        if self.manual_selection_made:
            guess_selection = self.selected_data
            self.selected_data = guess_selection
        elif self.context._frequency_context:
            self.update_selected_frequency_workspace_guess()
        else:
            self.update_selected_time_workspace_guess()

    def update_selected_frequency_workspace_guess(self):
        runs = 'All'
        groups_and_pairs = self.context.group_pair_context.selected
        if self.view.fit_type == self.view.simultaneous_fit:
            if self.view.simultaneous_fit_by == "Run":
                runs = self.view.simultaneous_fit_by_specifier
            else:
                groups_and_pairs = self.view.simultaneous_fit_by_specifier
        guess_selection = self.context.get_names_of_workspaces_to_fit(
            runs=runs,
            group_and_pair=groups_and_pairs,
            phasequad=True,
            rebin=not self.view.fit_to_raw, freq=self.context._frequency_context.plot_type)
        self.selected_data = guess_selection

    def update_selected_time_workspace_guess(self):
        runs = 'All'
        groups_and_pairs = self._get_selected_groups_and_pairs()
        if self.view.fit_type == self.view.simultaneous_fit:
            if self.view.simultaneous_fit_by == "Run":
                runs = self.view.simultaneous_fit_by_specifier
            elif self.view.simultaneous_fit_by == "Group/Pair":
                groups_and_pairs = [self.view.simultaneous_fit_by_specifier]

        guess_selection = []
        for grppair in groups_and_pairs:
            guess_selection += self.context.get_names_of_workspaces_to_fit(
                    runs=runs,
                    group_and_pair=grppair,
                    phasequad=False,
                    rebin=not self.view.fit_to_raw)

        guess_selection = list(set(self._check_data_exists(guess_selection)))
        self.selected_data = guess_selection

    def handle_display_workspace_changed(self):
        current_index = self.view.get_index_for_start_end_times()
        self.view.start_time = self.start_x[current_index]
        self.view.end_time = self.end_x[current_index]

        self.view.set_datasets_in_function_browser(
            [self.view.display_workspace])
        self.view.function_browser_multi.setCurrentDataset(current_index)

        self.update_fit_status_information_in_view()

    def handle_use_rebin_changed(self):
        if not self.view.fit_to_raw and not self.context._do_rebin():
            self.view.fit_to_raw = True
            self.view.warning_popup('No rebin options specified')
            return

        if not self.manual_selection_made:
            self.update_selected_workspace_guess()
        else:
            self.selected_data = self.context.get_list_of_binned_or_unbinned_workspaces_from_equivalents(
                self.selected_data)

    def handle_fit_type_changed(self):
        if self.view.fit_type == self.view.simultaneous_fit:
            self.view.workspace_combo_box_label.setText(
                'Display parameters for')
            self.view.simul_fit_by_specifier.setVisible(True)
            self.update_fit_selector_list()
        else:
            self.view.workspace_combo_box_label.setText('Select Workspace')
            self.view.simul_fit_by_specifier.setVisible(False)

    def handle_plot_guess_changed(self):
        if self.view.fit_type == self.view.simultaneous_fit:
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

    def fitting_domain_type_changed(self):
        if self.view.fit_type == self.view.simultaneous_fit:
            multi_domain_function = self.create_multi_domain_function(self._fit_function)
            self._multi_domain_function = multi_domain_function
            if multi_domain_function:
                self.view.function_browser_multi.blockSignals(True)
                self.view.function_browser_multi.setFunction(str(multi_domain_function))
                self.view.function_browser_multi.blockSignals(False)
                self._fit_function = [self.view.fit_object] * len(self.selected_data) if self.selected_data else [
                    self.view.fit_object]
            else:
                self._fit_function = [None] * len(self.selected_data) if self.selected_data else [None]
            self.view.switch_to_simultaneous()
            self.clear_fit_information()
        else:
            if self.view.fit_object:
                function_list = self.view.function_browser_multi.getGlobalFunction().createEquivalentFunctions()
                self._fit_function = function_list
                self.view.function_browser.blockSignals(True)
                self.view.function_browser.setFunction(str(self._fit_function[0]))
                self.view.function_browser.blockSignals(False)
            else:
                self._fit_function = [None] * len(self.selected_data) if self.selected_data else [None]
            self.view.switch_to_single()
            self.clear_fit_information()

    def handle_fit_clicked(self):
        self.context.fitting_context.number_of_fits = 0
        if self._tf_asymmetry_mode:
            self.perform_tf_asymmetry_fit()
        else:
            self.perform_standard_fit()

    def perform_standard_fit(self):
        self._fit_function_cache = [item.clone() for item in self._fit_function if item]
        fit_type = self.view.fit_type

        try:
            if fit_type == self.view.simultaneous_fit:
                self._number_of_fits_cached = 1
                simultaneous_fit_parameters = self.get_multi_domain_fit_parameters()
                global_parameters = self.view.get_global_parameters()
                calculation_function = functools.partial(
                    self.model.do_simultaneous_fit,
                    simultaneous_fit_parameters, global_parameters)
                self.calculation_thread = self.create_thread(
                    calculation_function)
            else:
                self._number_of_fits_cached = 1
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
        fit_type = self.view.fit_type

        try:
            if fit_type == self.view.simultaneous_fit:
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
            'InputFunction': self.view.fit_object,
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
        index = self.view.get_index_for_start_end_times()

        if self.view.fit_type == self.view.simultaneous_fit:
            self._fit_function = [fit_function] * len(self.start_x)
            self._fit_status = [fit_status] * len(self.start_x)
            self._fit_chi_squared = [fit_chi_squared] * len(self.start_x)

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
            self._fit_function = [None] * len(self.selected_data) if self.selected_data else [None]
        else:
            self._fit_function = [self.view.fit_object.clone() for _ in self.selected_data] \
                if self.selected_data else [self.view.fit_object.clone()]

        if self.automatically_update_fit_name:
            self.view.function_name = self.model.get_function_name(
                self.view.fit_object)
            self.model.function_name = self.view.function_name

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

        if self.view.fit_type != self.view.simultaneous_fit:
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
            self._fit_function = [new_function.clone()] * len(self.selected_data)
            self.view.function_browser_multi.blockSignals(True)
            self.view.function_browser_multi.clear()
            self.view.function_browser_multi.setFunction(
                str(self._fit_function[self.view.get_index_for_start_end_times()]))
            self.view.function_browser_multi.setGlobalParameters(new_global_parameters)
            self.view.function_browser_multi.blockSignals(False)

        self.update_fit_status_information_in_view()
        self.handle_display_workspace_changed()

    def handle_function_parameter_changed(self):
        if self.view.fit_type != self.view.simultaneous_fit:
            index = self.view.get_index_for_start_end_times()
            self._fit_function[index] = self.view.fit_object.clone()
        else:
            self._fit_function = [self.view.fit_object] * len(self.selected_data)

    def handle_undo_fit_clicked(self):
        self._fit_function = self._fit_function_cache
        self.clear_fit_information()
        self.update_fit_status_information_in_view()
        self.view.undo_fit_button.setEnabled(False)
        self.context.fitting_context.remove_latest_fit(self._number_of_fits_cached)

    def handle_fit_by_changed(self):
        if self.view.fit_type == self.view.simultaneous_fit:
            self.update_fit_selector_list()
            self.update_selected_workspace_guess()
            if self.view.simultaneous_fit_by == "Custom":
                self.view.simul_fit_by_specifier.setVisible(False)
                self.view.select_workspaces_to_fit_button.setVisible(True)
            else:
                self.view.simul_fit_by_specifier.setVisible(True)
                self.view.select_workspaces_to_fit_button.setVisible(False)
        else:
            return

    def handle_fit_specifier_changed(self):
        self.update_selected_workspace_guess()

    def update_fit_selector_list(self):
        if self.view.simultaneous_fit_by == "Run":
            flattened_run_list = [str(item) for sublist in self.context.data_context.current_runs for item in sublist]
            simul_choices = flattened_run_list
        else:
            simul_choices = self._get_selected_groups_and_pairs()

        self.view.setup_fit_by_specifier(simul_choices)

    def get_parameters_for_single_fit(self):
        params = self._get_shared_parameters()

        params['InputWorkspace'] = self.view.display_workspace
        params['StartX'] = self.start_x[0]
        params['EndX'] = self.end_x[0]

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
                if selected_run == self._get_run_number_from_workspace(workspace):
                    fit_workspaces.append(workspace)
        else:
            pass
        return fit_workspaces

    def _get_shared_parameters(self):
        """
        :return: The set of attributes common to all fit types
        """
        return {
            'Function': self.view.fit_object,
            'Minimizer': self.view.minimizer,
            'EvaluationType': self.view.evaluation_type
        }

    @property
    def selected_data(self):
        return self._selected_data

    @selected_data.setter
    def selected_data(self, selected_data):
        if self._selected_data == selected_data:
            return

        self._selected_data = selected_data
        self.clear_and_reset_gui_state()

    def clear_and_reset_gui_state(self):
        single_data = [self.selected_data[0]] if self.selected_data else []
        self.view.set_datasets_in_function_browser(single_data)
        self.view.set_datasets_in_function_browser_multi(self.selected_data)

        self._fit_status = [None] * len(
            self.selected_data) if self.selected_data else [None]
        self._fit_chi_squared = [0.0] * len(
            self.selected_data) if self.selected_data else [0.0]
        self._fit_function = [self.view.fit_object] * len(
            self.selected_data) if self.selected_data else [self.view.fit_object]
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

    @property
    def start_x(self):
        return self._start_x

    @property
    def end_x(self):
        return self._end_x

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
        current_index = self.view.get_index_for_start_end_times()
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

    def check_workspaces_are_tf_asymmetry_compliant(self, workspace_list):
        non_compliant_workspaces = [item for item in workspace_list if 'Group' not in item]
        return False if non_compliant_workspaces else True

    def get_parameters_for_tf_function_calculation(self, fit_function):
        mode = 'Construct' if self.view.tf_asymmetry_mode else 'Extract'
        workspace_list = self.selected_data if self.view.fit_type == self.view.simultaneous_fit else [
            self.view.display_workspace]
        return {'InputFunction': fit_function,
                'WorkspaceList': workspace_list,
                'Mode': mode}

    def create_multi_domain_function(self, function_list):
        if not any(function_list):
            return None
        multi_domain_function = MultiDomainFunction()
        for index, func in enumerate(function_list):
            multi_domain_function.add(func)
            multi_domain_function.setDomainIndex(index, index)

        return multi_domain_function

    def _get_selected_groups_and_pairs(self):
        return self.context.group_pair_context.selected_groups + self.context.group_pair_context.selected_pairs

    def _check_data_exists(self, guess_selection):
        return [item for item in guess_selection if AnalysisDataService.doesExist(item)]

    def _get_run_number_from_workspace(self, workspace_name):
        instrument = self.context.data_context.instrument
        run = re.findall(r'%s(\d+)' % instrument, workspace_name)
        return run[0]

    # the string following either 'Pair Asym; %s' or  Group; "
    def _get_group_or_pair_from_workspace_name(self, workspace_name):
        for grppair in self.context.group_pair_context.selected_groups + self.context.group_pair_context.selected_pairs:
            grp = re.findall(r'%s' % grppair, workspace_name)
            if len(grp) > 0:
                return grp[0]
