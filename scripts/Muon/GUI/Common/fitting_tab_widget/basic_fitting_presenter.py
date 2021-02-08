# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing, GenericObservable
from mantidqt.widgets.fitscriptgenerator import (FitScriptGeneratorModel, FitScriptGeneratorPresenter,
                                                 FitScriptGeneratorView)

import re


class BasicFittingPresenter:

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
        self._fit_function_cache = [None]
        self._multi_domain_function = None

        self._number_of_fits_cached = 0

        self.automatically_update_fit_name = True

        self.thread_success = True
        self.enable_editing_notifier = GenericObservable()
        self.disable_editing_notifier = GenericObservable()
        self.fitting_calculation_model = None

        self.fit_function_changed_notifier = GenericObservable()

        self.gui_context_observer = GenericObserverWithArgPassing(self.handle_gui_changes_made)

        self.view.setEnabled(False)

        self.fsg_model = None
        self.fsg_view = None
        self.fsg_presenter = None

    def disable_view(self):
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

    def clear_and_reset_gui_state(self):
        raise NotImplementedError("This method must be overridden by a child class.")

    @property
    def start_x(self):
        return self._start_x

    @property
    def end_x(self):
        return self._end_x

    def handle_fit_generator_clicked(self):
        self._open_fit_script_generator_interface(self.get_loaded_workspaces(), self.get_fit_browser_options())

    def handle_gui_changes_made(self, changed_values):
        for key in changed_values.keys():
            if key in ['FirstGoodDataFromFile', 'FirstGoodData']:
                self._reset_start_time_to_first_good_data_value()

    def handle_fit_clicked(self):
        if len(self.selected_data) < 1:
            self.view.warning_popup('No data selected to fit')
            return
        self.perform_fit()

    def handle_started(self):
        self.disable_editing_notifier.notify_subscribers()
        self.thread_success = True

    def perform_fit(self):
        raise NotImplementedError("This method must be overridden by a child class.")

    def get_loaded_workspaces(self):
        raise NotImplementedError("This method must be overridden by a child class.")

    def get_fit_browser_options(self):
        raise NotImplementedError("This method must be overridden by a child class.")

    def _open_fit_script_generator_interface(self, loaded_workspaces, fit_options):
        self.fsg_model = FitScriptGeneratorModel()
        self.fsg_view = FitScriptGeneratorView(None, fit_options)
        self.fsg_presenter = FitScriptGeneratorPresenter(self.fsg_view, self.fsg_model, loaded_workspaces,
                                                         self.view.start_time, self.view.end_time)

        self.fsg_presenter.openFitScriptGenerator()

    def _reset_start_time_to_first_good_data_value(self):
        self._start_x = [self._retrieve_first_good_data_from_run_name(run_name) for run_name in self.selected_data] if \
            self.selected_data else [0.0]
        self._end_x = [self.view.end_time] * len(self.selected_data) if self.selected_data else [15.0]

        self.view.start_time = self.start_x[0] if 0 < len(self.start_x) else 0.0
        self.view.end_time = self.end_x[0] if 0 < len(self.end_x) else 15.0

    def _retrieve_first_good_data_from_run_name(self, workspace_name):
        try:
            run = [float(re.search('[0-9]+', workspace_name).group())]
        except AttributeError:
            return 0.0

        return self.context.first_good_data(run)
