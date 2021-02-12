# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import MultiDomainFunction
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing, GenericObservable
from mantidqt.widgets.fitscriptgenerator import (FitScriptGeneratorModel, FitScriptGeneratorPresenter,
                                                 FitScriptGeneratorView)

from Muon.GUI.Common import thread_model
from Muon.GUI.Common.fitting_tab_widget.basic_fitting_model import DEFAULT_START_X, DEFAULT_END_X
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput

import functools

from mantid import logger


class BasicFittingPresenter:

    def __init__(self, view, model):
        self.view = view
        self.model = model

        self._number_of_fits_cached = 0

        self.thread_success = True
        self.enable_editing_notifier = GenericObservable()
        self.disable_editing_notifier = GenericObservable()
        self.fitting_calculation_model = None

        self.fit_function_changed_notifier = GenericObservable()

        self.gui_context_observer = GenericObserverWithArgPassing(self.handle_gui_changes_made)
        self.update_view_from_model_observer = GenericObserverWithArgPassing(self.update_view_from_model)

        self.fsg_model = None
        self.fsg_view = None
        self.fsg_presenter = None

        self.initialize_model_options()

        self.view.set_slot_for_fit_generator_clicked(self.handle_fit_generator_clicked)
        self.view.set_slot_for_fit_button_clicked(self.handle_fit_clicked)
        self.view.set_slot_for_undo_fit_clicked(self.handle_undo_fit_clicked)
        self.view.set_slot_for_plot_guess_changed(self.handle_plot_guess_changed)
        self.view.set_slot_for_fit_name_changed(self.handle_fit_name_changed_by_user)
        self.view.set_slot_for_function_structure_changed(self.handle_function_structure_changed)
        self.view.set_slot_for_function_parameter_changed(self.handle_function_parameter_changed)
        self.view.set_slot_for_start_x_updated(self.handle_start_x_updated)
        self.view.set_slot_for_end_x_updated(self.handle_end_x_updated)
        self.view.set_slot_for_minimizer_changed(self.handle_minimizer_changed)
        self.view.set_slot_for_evaluation_type_changed(self.handle_evaluation_type_changed)
        self.view.set_slot_for_use_raw_changed(self.handle_use_rebin_changed)

    def initialize_model_options(self):
        """Initialise the model with the default fitting options."""
        self.model.start_xs = [self.view.start_time]
        self.model.end_xs = [self.view.end_time]
        self.model.minimizer = self.view.minimizer
        self.model.evaluation_type = self.view.evaluation_type
        self.model.fit_to_raw = self.view.fit_to_raw

    def update_view_from_model(self, workspace_removed=None):
        """Update the view using the data stored in the model."""
        if workspace_removed:
            self.model.dataset_names = [name for name in self.model.dataset_names if name != workspace_removed]
        else:
            self.model.dataset_names = []
        self.clear_and_reset_gui_state()

    def handle_gui_changes_made(self, changed_values):
        """Handle when changes to the context have been made."""
        for key in changed_values.keys():
            if key in ['FirstGoodDataFromFile', 'FirstGoodData']:
                self._reset_start_time_to_first_good_data_value()

    def handle_fit_clicked(self):
        """Handle when the fit button is clicked."""
        if self.model.number_of_datasets < 1:
            self.view.warning_popup("No data selected for fitting.")
            return
        if not self.view.fit_object:
            return

        self.model.cache_the_current_fit_functions()
        self._perform_fit()

    def handle_started(self):
        """Handle when fitting has started."""
        self.disable_editing_notifier.notify_subscribers()
        self.thread_success = True

    def handle_finished(self):
        """Handle when fitting has finished."""
        self.enable_editing_notifier.notify_subscribers()
        if not self.thread_success:
            return

        fit_function, fit_status, fit_chi_squared = self.fitting_calculation_model.result
        if any([not fit_function, not fit_status, not fit_chi_squared]):
            return

        self.handle_fitting_finished(fit_function, fit_status, fit_chi_squared)
        self.view.enable_undo_fit(True)
        self.view.plot_guess = False

    def handle_error(self, error):
        """Handle when an error occurs while fitting."""
        self.enable_editing_notifier.notify_subscribers()
        self.thread_success = False
        self.view.warning_popup(error)

    def handle_new_data_loaded(self):
        """Handle when new data has been loaded into the interface."""
        self.view.plot_guess = False

    def handle_undo_fit_clicked(self):
        """Handle when undo fit is clicked."""
        self.model.single_fit_functions = self.model.single_fit_functions_cache
        self.model.remove_latest_fit_from_context()
        self._reset_fit_status_information()
        self._number_of_fits_cached = 0

    def handle_fit_generator_clicked(self):
        """Handle when the Fit Generator button has been clicked."""
        self._open_fit_script_generator_interface(self.model.dataset_names, self._get_fit_browser_options())

    def handle_fit_name_changed_by_user(self):
        """Handle when the fit name is changed by the user."""
        self.model.function_name_auto_update = False
        self.model.function_name = self.view.function_name

    def handle_minimizer_changed(self):
        """Handle when a minimizer is changed."""
        self.model.minimizer = self.view.minimizer

    def handle_evaluation_type_changed(self):
        """Handle when the evaluation type is changed."""
        self.model.evaluation_type = self.view.evaluation_type

    def handle_fitting_finished(self):
        """Handle when fitting is finished."""
        raise NotImplementedError("This method must be overridden by a child class.")

    def handle_plot_guess_changed(self):
        """Handle when plot guess is ticked or unticked."""
        raise NotImplementedError("This method must be overridden by a child class.")

    def handle_function_structure_changed(self):
        """Handle when the function structure is changed."""
        raise NotImplementedError("This method must be overridden by a child class.")

    def handle_function_parameter_changed(self):
        """Handle when a function parameter is changed."""
        raise NotImplementedError("This method must be overridden by a child class.")

    def handle_start_x_updated(self):
        """Handle when the start X is changed."""
        if self.view.start_time > self.view.end_time:
            self.view.start_time, self.view.end_time = self.view.end_time, self.view.start_time
            self.model.current_end_x = self.view.end_time

        self.model.current_start_x = self.view.start_time

    def handle_end_x_updated(self):
        """Handle when the end X is changed."""
        if self.view.end_time < self.view.start_time:
            self.view.start_time, self.view.end_time = self.view.end_time, self.view.start_time
            self.model.current_start_x = self.view.start_time

        self.model.current_end_x = self.view.end_time

    def handle_use_rebin_changed(self):
        """Handle the Fit to raw data checkbox state change."""
        if self._check_rebin_options():
            self.model.fit_to_raw = self.view.fit_to_raw

    def clear_and_reset_gui_state(self):
        """Clears all data in the view and updates the model."""
        self.view.set_datasets_in_function_browser(self.model.dataset_names)

        self._reset_fit_status_information()
        self._reset_start_time_to_first_good_data_value()
        #self._reset_fit_function()

    def set_current_dataset_index(self, dataset_index):
        self.model.current_dataset_index = dataset_index
        self.view.set_current_dataset_index(dataset_index)

    def current_single_fit_function_in_view(self):
        return self._get_single_fit_functions_from_view()[self.model.current_dataset_index]

    def update_single_fit_functions_in_model(self):
        self.model.single_fit_functions = self._get_single_fit_functions_from_view()

    def _get_single_fit_functions_from_view(self):
        """Returns the fit functions corresponding to each domain as a list."""
        if self.view.fit_object:
            if isinstance(self.view.fit_object, MultiDomainFunction):
                return [function.clone() for function in self.view.fit_object.createEquivalentFunctions()]
            return [self.view.fit_object]
        return [None] * self.view.number_of_domains()

    def _get_fit_browser_options(self):
        """Returns the fitting options to use in the Fit Script Generator interface."""
        return {"FittingType": "Simultaneous" if self.simultaneous_fitting_mode() else "Sequential",
                "Minimizer": self.model.minimizer,
                "EvaluationType": self.model.evaluation_type}

    @staticmethod
    def simultaneous_fitting_mode():
        """Returns true if the fitting mode is simultaneous. Override this method if you require simultaneous."""
        return False

    def _perform_fit(self):
        """Perform the fit in a thread."""
        self._number_of_fits_cached += 1
        try:
            logger.warning(str(self.model.get_active_workspace_names()))
            calculation_function = functools.partial(self.model.evaluate_single_fit,
                                                     self.model.get_active_workspace_names())
            self.calculation_thread = self._create_thread(calculation_function)
            self.calculation_thread.threadWrapperSetUp(self.handle_started,
                                                       self.handle_finished,
                                                       self.handle_error)
            self.calculation_thread.start()
        except ValueError as error:
            self.view.warning_popup(error)

    def automatically_update_function_name(self):
        """Updates the function name used within the outputted fit workspaces."""
        self.model.automatically_update_function_name()
        self.view.function_name = self.model.function_name

    def update_fit_statuses_and_chi_squared_in_view(self):
        """Updates the local and global fit status and chi squared in the view."""
        self.view.update_local_fit_status_and_chi_squared(self.model.current_fit_status,
                                                          self.model.current_fit_chi_squared)
        self.view.update_global_fit_status(self.model.fit_statuses, self.model.current_dataset_index)

    def _create_thread(self, callback):
        """Create a thread for fitting."""
        self.fitting_calculation_model = ThreadModelWrapperWithOutput(callback)
        return thread_model.ThreadModel(self.fitting_calculation_model)

    def _open_fit_script_generator_interface(self, workspaces, fit_options):
        """Open the Fit Script Generator interface."""
        self.fsg_model = FitScriptGeneratorModel()
        self.fsg_view = FitScriptGeneratorView(None, fit_options)
        self.fsg_presenter = FitScriptGeneratorPresenter(self.fsg_view, self.fsg_model, workspaces,
                                                         self.view.start_time, self.view.end_time)

        self.fsg_presenter.openFitScriptGenerator()

    # def _reset_fit_function(self):
    #     """Reset the fit function information."""
    #     self.model.single_fit_functions = self._get_fit_function()

    def _reset_fit_status_information(self):
        """Reset the fit status and chi squared information currently displayed."""
        number_of_datasets = self.model.number_of_datasets

        self.model.fit_statuses = [None] * number_of_datasets if number_of_datasets > 0 else [None]
        self.model.fit_chi_squares = [0.0] * number_of_datasets if number_of_datasets > 0 else [0.0]
        self.update_fit_statuses_and_chi_squared_in_view()
        self.view.enable_undo_fit(False)

    def _reset_start_time_to_first_good_data_value(self):
        """Reset the start and end X to the first good data value."""
        number_of_datasets = self.model.number_of_datasets

        self.model.start_xs = [self.model.retrieve_first_good_data_from_run(name)
                               for name in self.model.dataset_names] if number_of_datasets > 0 else [DEFAULT_START_X]
        self.model.end_xs = [self.view.end_time] * number_of_datasets if number_of_datasets > 0 else [DEFAULT_END_X]

        self.view.start_time = self.model.start_xs[0]
        self.view.end_time = self.model.end_xs[0]

    def _check_rebin_options(self):
        """Check that a rebin was indeed requested in the fitting tab or in the context."""
        if not self.view.fit_to_raw and not self.model.do_rebin:
            self.view.fit_to_raw = True
            self.view.warning_popup("No rebin options specified.")
            return False
        return True
