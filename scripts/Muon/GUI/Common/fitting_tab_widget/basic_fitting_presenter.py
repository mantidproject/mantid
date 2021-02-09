# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing, GenericObservable
from mantidqt.widgets.fitscriptgenerator import (FitScriptGeneratorModel, FitScriptGeneratorPresenter,
                                                 FitScriptGeneratorView)

from Muon.GUI.Common import thread_model
from Muon.GUI.Common.contexts.frequency_domain_analysis_context import FrequencyDomainAnalysisContext
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput

import functools
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
        self.update_view_from_model_observer = GenericObserverWithArgPassing(self.update_view_from_model)

        self.view.setEnabled(False)

        self.fsg_model = None
        self.fsg_view = None
        self.fsg_presenter = None

        self.initialise_model_options()

        self.view.set_slot_for_fit_generator_clicked(self.handle_fit_generator_clicked)
        self.view.set_slot_for_fit_button_clicked(self.handle_fit_clicked)
        self.view.set_slot_for_undo_fit_clicked(self.handle_undo_fit_clicked)
        self.view.set_slot_for_plot_guess_changed(self.handle_plot_guess_changed)
        self.view.set_slot_for_fit_name_changed(self.handle_fit_name_changed_by_user)
        self.view.set_slot_for_function_structure_changed(self.handle_function_structure_changed)
        self.view.set_slot_for_function_parameter_changed(self.handle_function_parameter_changed)
        self.view.set_slot_for_start_x_updated(self.handle_start_x_updated)
        self.view.set_slot_for_end_x_updated(self.handle_end_x_updated)
        self.view.set_slot_for_minimiser_changed(self.handle_minimiser_changed)
        self.view.set_slot_for_evaluation_type_changed(self.handle_evaluation_type_changed)
        self.view.set_slot_for_use_raw_changed(self.handle_use_rebin_changed)

    def initialise_model_options(self):
        """Initialise the model with the default fitting options."""
        self.model.update_model_fit_options(**self.get_model_fitting_options())

    def get_model_fitting_options(self):
        """Returns the fitting options to be used when initializing the model. Override in child classes."""
        fitting_options = {"fit_function": self._fit_function[0], "startX": self.start_x[0], "endX": self.end_x[0],
                           "minimiser": self.view.minimizer, "evaluation_type": self.view.evaluation_type,
                           "fit_to_raw": self.view.fit_to_raw}
        return fitting_options

    def disable_view(self):
        """Disable the widgets in the view if data is not loaded."""
        if not self.selected_data:
            self.view.setEnabled(False)

    def enable_view(self):
        """Enable the widgets in the view if data is loaded."""
        if self.selected_data:
            self.view.setEnabled(True)

    def update_model_from_view(self, **kwargs):
        """Update the fit options stored in the model."""
        self.model.update_model_fit_options(**kwargs)

    def update_view_from_model(self, workspace_removed=None):
        """Update the view using the data stored in the model."""
        if workspace_removed:
            self.selected_data = [item for item in self.selected_data if item != workspace_removed]
        else:
            self.selected_data = []

    @property
    def selected_data(self):
        """Return the names of the currently selected workspaces."""
        return self._selected_data

    @selected_data.setter
    def selected_data(self, selected_data):
        """Set the currently selected data. Clear and reset the GUI."""
        if self._selected_data == selected_data:
            return

        self._selected_data = selected_data
        self.clear_and_reset_gui_state()

    @property
    def start_x(self):
        """Return the stored start X's."""
        return self._start_x

    @property
    def end_x(self):
        """Return the stored end X's."""
        return self._end_x

    def update_start_x(self, index, value):
        """Updates a start X at a specific index."""
        self._start_x[index] = value

    def update_end_x(self, index, value):
        """Updates a end X at a specific index."""
        self._end_x[index] = value

    def handle_gui_changes_made(self, changed_values):
        """Handle when changes to the context have been made."""
        for key in changed_values.keys():
            if key in ['FirstGoodDataFromFile', 'FirstGoodData']:
                self._reset_start_time_to_first_good_data_value()

    def handle_fit_clicked(self):
        """Handle when the fit button is clicked."""
        if len(self.selected_data) < 1:
            self.view.warning_popup("No data selected to fit")
            return
        if not self.view.fit_object:
            return

        self._update_fit_function_cache()
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
        self.model.create_ws_fit_function_map()
        self.enable_view()

    def handle_undo_fit_clicked(self):
        """Handle when undo fit is clicked."""
        self._fit_function = self._fit_function_cache
        self._reset_fit_status_information()
        self.context.fitting_context.remove_latest_fit()
        self._number_of_fits_cached = 0

    def handle_fit_generator_clicked(self):
        """Handle when the Fit Generator button has been clicked."""
        self._open_fit_script_generator_interface(self.get_loaded_workspaces(), self._get_fit_browser_options())

    def handle_fit_name_changed_by_user(self):
        """Handle when the fit name is changed by the user."""
        self.automatically_update_fit_name = False
        self.model.function_name = self.view.function_name

    def handle_minimiser_changed(self):
        """Handle when a minimiser is changed."""
        self.update_model_from_view(minimiser=self.view.minimizer)

    def handle_evaluation_type_changed(self):
        """Handle when the evaluation type is changed."""
        self.update_model_from_view(evaluation_type=self.view.evaluation_type)

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
        raise NotImplementedError("This method must be overridden by a child class.")

    def handle_end_x_updated(self):
        """Handle when the end X is changed."""
        raise NotImplementedError("This method must be overridden by a child class.")

    def handle_use_rebin_changed(self):
        """Handle the Use Rebin state change."""
        raise NotImplementedError("This method must be overridden by a child class.")

    def get_loaded_workspaces(self):
        """Retrieve the names of the workspaces successfully loaded into the fitting interface."""
        raise NotImplementedError("This method must be overridden by a child class.")

    def get_fit_input_workspaces(self):
        """Retrieve the names of the workspaces to be fitted."""
        raise NotImplementedError("This method must be overridden by a child class.")

    def get_x_data_type(self):
        """Returns the type of data in the x domain. Returns string None if it cannot be determined."""
        if isinstance(self.context, FrequencyDomainAnalysisContext):
            return self.context._frequency_context.plot_type
        return "None"

    def clear_and_reset_gui_state(self):
        """Clears all data in the view and updates the model."""
        self.view.set_datasets_in_function_browser(self.selected_data)

        self._reset_fit_status_information()
        self._reset_start_time_to_first_good_data_value()
        self._reset_fit_function()

    def _get_fit_browser_options(self):
        """Returns the fitting options to use in the Fit Script Generator interface."""
        return {"FittingType": "Simultaneous" if self._is_simultaneous_fitting() else "Sequential",
                "Minimizer": self.view.minimizer,
                "EvaluationType": self.view.evaluation_type}

    @staticmethod
    def _is_simultaneous_fitting():
        """Returns true if the fitting mode is simultaneous. Override this method if you require simultaneous."""
        return False

    def _perform_fit(self):
        """Perform the fit in a thread."""
        self._number_of_fits_cached += 1
        try:
            calculation_function = functools.partial(self.model.evaluate_single_fit, self.get_fit_input_workspaces())
            self.calculation_thread = self._create_thread(calculation_function)
            self.calculation_thread.threadWrapperSetUp(self.handle_started,
                                                       self.handle_finished,
                                                       self.handle_error)
            self.calculation_thread.start()
        except ValueError as error:
            self.view.warning_popup(error)

    def _update_fit_function_cache(self):
        """Update the fit function cache with the next fit function to be fitted."""
        self._fit_function_cache = [func.clone() for func in self._fit_function]

    def _update_function_name(self):
        """Updates the function name used within the outputted fit workspaces."""
        if self.automatically_update_fit_name:
            name = self._get_fit_function()[0]
            self.view.function_name = self.model.get_function_name(name)
            self.model.function_name = self.view.function_name

    def _update_fit_status_information_in_view(self):
        """Updates the fit status and chi squared information in the view."""
        current_index = self._fit_function_index()
        self.view.update_with_fit_outputs(self._fit_function[current_index],
                                          self._fit_status[current_index],
                                          self._fit_chi_squared[current_index])
        self.view.update_global_fit_state(self._fit_status, current_index)

    def _fit_function_index(self):
        """Return the index of the currently displayed fit function (assume it is zero for BasicFitting for now)."""
        return 0

    def _create_thread(self, callback):
        """Create a thread for fitting."""
        self.fitting_calculation_model = ThreadModelWrapperWithOutput(callback)
        return thread_model.ThreadModel(self.fitting_calculation_model)

    def _open_fit_script_generator_interface(self, loaded_workspaces, fit_options):
        """Open the Fit Script Generator interface."""
        self.fsg_model = FitScriptGeneratorModel()
        self.fsg_view = FitScriptGeneratorView(None, fit_options)
        self.fsg_presenter = FitScriptGeneratorPresenter(self.fsg_view, self.fsg_model, loaded_workspaces,
                                                         self.view.start_time, self.view.end_time)

        self.fsg_presenter.openFitScriptGenerator()

    def _reset_fit_function(self):
        """Reset the fit function information."""
        if self.view.fit_object:
            self._fit_function = [func.clone() for func in self._get_fit_function()]
        else:
            self._fit_function = [None] * len(self.selected_data) if self.selected_data else [None]

        self.update_model_from_view(fit_function=self._fit_function[0])

    def _reset_fit_status_information(self):
        """Reset the fit status and chi squared information currently displayed."""
        self._fit_status = [None] * len(self.selected_data) if self.selected_data else [None]
        self._fit_chi_squared = [0.0] * len(self.selected_data) if self.selected_data else [0.0]
        self._update_fit_status_information_in_view()
        self.view.enable_undo_fit(False)

    def _reset_start_time_to_first_good_data_value(self):
        """Reset the start and end X to the first good data value."""
        self._start_x = [self._retrieve_first_good_data_from_run_name(run_name)
                         for run_name in self.selected_data] if self.selected_data else [0.0]
        self._end_x = [self.view.end_time] * len(self.selected_data) if self.selected_data else [15.0]

        self.view.start_time = self.start_x[0] if 0 < len(self.start_x) else 0.0
        self.view.end_time = self.end_x[0] if 0 < len(self.end_x) else 15.0

        self.update_model_from_view(startX=self.start_x[0], endX=self.end_x[0])

    def _retrieve_first_good_data_from_run_name(self, workspace_name):
        """Return the name of the first good data in a workspace."""
        try:
            run = [float(re.search("[0-9]+", workspace_name).group())]
        except AttributeError:
            return 0.0

        return self.context.first_good_data(run)

    def _check_rebin_options(self):
        """Check that a rebin was indeed requested in the fitting tab or in the context."""
        if not self.view.fit_to_raw and not self.context._do_rebin():
            self.view.fit_to_raw = True
            self.view.warning_popup("No rebin options specified.")
            return False
        return True

    @staticmethod
    def _check_data_exists(workspace_names):
        """Returns only the workspace names that exist in the ADS."""
        return [workspace_name for workspace_name in workspace_names if AnalysisDataService.doesExist(workspace_name)]
