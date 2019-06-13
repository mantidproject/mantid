# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from Muon.GUI.Common.fitting_tab_widget.workspace_selector_view import WorkspaceSelectorView
from Muon.GUI.Common.observer_pattern import GenericObserver, GenericObserverWithArgPassing
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput
from Muon.GUI.Common import thread_model
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
        self.manual_selection_made = False
        self.automatically_update_fit_name = True
        self.thread_success = True
        self.update_selected_workspace_guess()
        self.gui_context_observer = GenericObserverWithArgPassing(
            self.handle_gui_changes_made)
        self.selected_group_pair_observer = GenericObserver(
            self.handle_selected_group_pair_changed)
        self.input_workspace_observer = GenericObserver(
            self.handle_new_data_loaded)
        self.disable_tab_observer = GenericObserver(lambda: self.view.
                                                    setEnabled(False))
        self.enable_tab_observer = GenericObserver(lambda: self.view.
                                                   setEnabled(True))

        self.update_view_from_model_observer = GenericObserver(self.update_view_from_model)

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

    def update_selected_workspace_guess(self):
        if not self.manual_selection_made:
            guess_selection = self.context.get_names_of_workspaces_to_fit(
                runs='All',
                group_and_pair=self.context.group_pair_context.selected,
                phasequad=True,
                rebin=not self.view.fit_to_raw)
        else:
            guess_selection = self.selected_data

        self.selected_data = guess_selection

    def handle_display_workspace_changed(self):
        fit_type = self.view.fit_type
        current_index = self.view.get_index_for_start_end_times()
        self.view.start_time = self.start_x[current_index]
        self.view.end_time = self.end_x[current_index]

        if fit_type != self.view.simultaneous_fit:
            self.view.set_datasets_in_function_browser(
                [self.view.display_workspace])
        else:
            self.view.function_browser.setCurrentDataset(current_index)

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
        fit_type = self.view.fit_type
        self.clear_fit_information()

        if fit_type == self.view.single_fit:
            self.view.workspace_combo_box_label.setText('Select Workspace')
        else:
            self.view.workspace_combo_box_label.setText(
                'Display parameters for')

        self.update_selected_workspace_guess()

        if self.view.fit_type == self.view.simultaneous_fit:
            self.view.set_datasets_in_function_browser(self.selected_data)
        else:
            self.view.set_datasets_in_function_browser(
                [self.selected_data[0]] if self.selected_data else [])

        self.update_fit_status_information_in_view()

    def handle_fit_clicked(self):
        fit_type = self.view.fit_type

        try:
            if fit_type == self.view.single_fit:
                single_fit_parameters = self.get_parameters_for_single_fit()
                calculation_function = functools.partial(
                    self.model.do_single_fit, single_fit_parameters)
                self.calculation_thread = self.create_thread(
                    calculation_function)
            elif fit_type == self.view.simultaneous_fit:
                simultaneous_fit_parameters = self.get_multi_domain_fit_parameters(
                )
                global_parameters = self.view.get_global_parameters()
                calculation_function = functools.partial(
                    self.model.do_simultaneous_fit,
                    simultaneous_fit_parameters, global_parameters)
                self.calculation_thread = self.create_thread(
                    calculation_function)
            elif fit_type == self.view.sequential_fit:
                sequential_fit_parameters = self.get_multi_domain_fit_parameters(
                )
                calculation_function = functools.partial(
                    self.model.do_sequential_fit, sequential_fit_parameters)
                self.calculation_thread = self.create_thread(
                    calculation_function)

            self.calculation_thread.threadWrapperSetUp(self.handle_started,
                                                       self.handle_finished,
                                                       self.handle_error)
            self.calculation_thread.start()
        except ValueError as error:
            self.view.warning_popup(error)

    def handle_started(self):
        self.view.setEnabled(False)
        self.thread_success = True

    def handle_finished(self):
        if not self.thread_success:
            return

        self.view.setEnabled(True)
        fit_function, fit_status, fit_chi_squared = self.fitting_calculation_model.result
        index = self.view.get_index_for_start_end_times()

        if self.view.fit_type == self.view.sequential_fit:
            self._fit_function = fit_function
            self._fit_status = fit_status
            self._fit_chi_squared = fit_chi_squared
        elif self.view.fit_type == self.view.single_fit:
            self._fit_function[index] = fit_function
            self._fit_status[index] = fit_status
            self._fit_chi_squared[index] = fit_chi_squared
        elif self.view.fit_type == self.view.simultaneous_fit:
            self._fit_function = [fit_function] * len(self.start_x)
            self._fit_status = [fit_status] * len(self.start_x)
            self._fit_chi_squared = [fit_chi_squared] * len(self.start_x)

        self.update_fit_status_information_in_view()

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
        self.clear_fit_information()
        if self.automatically_update_fit_name:
            self.view.function_name = self.model.get_function_name(
                self.view.fit_object)
            self.model.function_name = self.view.function_name

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

    def _get_shared_parameters(self):
        """
        :return: The set of attributes common to all fit types
        """
        return {
            'Function': self.view.fit_object,
            'Minimizer': self.view.minimizer,
            'EvaluationType': self.view.evaluation_type,
            'FitGroupName': self.view.group_name
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
        self._fit_status = [None] * len(
            self.selected_data) if self.selected_data else [None]
        self._fit_chi_squared = [0.0] * len(
            self.selected_data) if self.selected_data else [0.0]
        self._fit_function = [None] * len(
            self.selected_data) if self.selected_data else [None]

        if self.view.fit_type == self.view.simultaneous_fit:
            self.view.set_datasets_in_function_browser(self.selected_data)
        else:
            self.view.set_datasets_in_function_browser(
                [self.selected_data[0]] if self.selected_data else [])

        self.reset_start_time_to_first_good_data_value()
        self.view.update_displayed_data_combo_box(self.selected_data)
        self.update_fit_status_information_in_view()

    def clear_fit_information(self):
        self._fit_status = [None] * len(
            self.selected_data) if self.selected_data else [None]
        self._fit_chi_squared = [0.0] * len(
            self.selected_data) if self.selected_data else [0.0]
        self._fit_function = [None] * len(
            self.selected_data) if self.selected_data else [None]
        self.update_fit_status_information_in_view()

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
        self._start_x = [self.retrieve_first_good_data_from_run_name(run_name) for run_name in self.selected_data] if\
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

    def update_view_from_model(self):
        pass
