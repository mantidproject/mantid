# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.fitting_tab_widget.workspace_selector_view import WorkspaceSelectorView
from Muon.GUI.Common.observer_pattern import GenericObserver
from mantid.api import FunctionFactory
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput
from Muon.GUI.Common import thread_model
import functools


class FittingTabPresenter(object):
    def __init__(self, view, model, context):
        self.view = view
        self.model = model
        self.context = context
        self._selected_data = []
        self._start_x = [self.view.start_time]
        self._end_x = [self.view.end_time]
        self.manual_selection_made = False
        self.update_selected_workspace_guess()
        self.gui_context_observer = GenericObserver(self.update_selected_workspace_guess)
        self.run_changed_observer = GenericObserver(self.handle_new_data_loaded)

    def handle_select_fit_data_clicked(self):
        selected_data, dialog_return = WorkspaceSelectorView.get_selected_data(self.context.data_context.current_runs,
                                                                               self.context.data_context.instrument,
                                                                               self.selected_data,
                                                                               self.view.fit_to_raw,
                                                                               self.context,
                                                                               self.view)

        if dialog_return:
            self.selected_data = selected_data
            self.manual_selection_made = True

    def handle_new_data_loaded(self):
        self.manual_selection_made = False
        self.update_selected_workspace_guess()

    def update_selected_workspace_guess(self):
        fit_type = self.view.fit_type

        if fit_type == self.view.single_fit:
            if not self.manual_selection_made:
                guess_selection = self.context.get_names_of_workspaces_to_fit(runs='All',
                                                                              group_and_pair='All', phasequad=True,
                                                                              rebin=not self.view.fit_to_raw)
            else:
                guess_selection = self.selected_data
            self.selected_data = guess_selection
        else:
            if not self.manual_selection_made:
                guess_selection = self.context.get_names_of_workspaces_to_fit(runs='All',
                                                                              group_and_pair=self.context.gui_context[
                                                                                  'selected_group_pair'],
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
            self.view.set_datasets_in_function_browser([self.view.display_workspace])
        else:
            self.view.function_browser.setCurrentDataset(current_index)

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

        if fit_type == self.view.single_fit:
            self.view.workspace_combo_box_label.setText('Select Workspace')
        else:
            self.view.workspace_combo_box_label.setText('Display parameters for')

        self.update_selected_workspace_guess()

    def handle_fit_clicked(self):
        fit_type = self.view.fit_type

        try:
            if fit_type == self.view.single_fit:
                single_fit_parameters = self.get_parameters_for_single_fit()
                calculation_function = functools.partial(self.model.do_single_fit, single_fit_parameters)
                self.calculation_thread = self.create_thread(calculation_function)
            elif fit_type == self.view.simultaneous_fit:
                simultaneous_fit_parameters = self.get_multi_domain_fit_parameters()
                calculation_function = functools.partial(self.model.do_simultaneous_fit, simultaneous_fit_parameters)
                self.calculation_thread = self.create_thread(calculation_function)
            elif fit_type == self.view.sequential_fit:
                sequential_fit_parameters = self.get_multi_domain_fit_parameters()
                calculation_function = functools.partial(self.model.do_sequential_fit, sequential_fit_parameters)
                self.calculation_thread = self.create_thread(calculation_function)

            self.calculation_thread.threadWrapperSetUp(self.handle_started, self.handle_finished, self.handle_error)
            self.calculation_thread.start()
        except ValueError as error:
            self.view.warning_popup(error)

    def handle_started(self):
        self.view.setEnabled(False)

    def handle_finished(self):
        self.view.setEnabled(True)

    def handle_error(self, error):
        self.view.warning_popup(error)

    def handle_start_x_updated(self):
        value = self.view.start_time
        index = self.view.get_index_for_start_end_times()
        self.update_start_x(index, value)

    def handle_end_x_updated(self):
        value = self.view.end_time
        index = self.view.get_index_for_start_end_times()
        self.update_end_x(index, value)

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
        params = {}
        params['Function'] = FunctionFactory.createInitialized(self.view.fit_string)
        params['Minimizer'] = self.view.minimizer
        params['EvaluationType'] = self.view.evaluation_type
        return params

    @property
    def selected_data(self):
        return self._selected_data

    @selected_data.setter
    def selected_data(self, selected_data):
        self._selected_data = selected_data
        if self.view.fit_type != self.view.single_fit:
            self._start_x = [self.view.start_time] * len(selected_data)
            self._end_x = [self.view.end_time] * len(selected_data)

        if self.view.fit_type == self.view.simultaneous_fit:
            self.view.set_datasets_in_function_browser(self._selected_data)
        else:
            self.view.set_datasets_in_function_browser([self._selected_data[0]] if self._selected_data else [])

        self.view.update_displayed_data_combo_box(selected_data)

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
