# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import MultiDomainFunction
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing, GenericObservable, GenericObserver
from mantidqt.widgets.fitscriptgenerator import (FittingMode, FitScriptGeneratorModel, FitScriptGeneratorPresenter,
                                                 FitScriptGeneratorView)

from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import BasicFittingModel
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_view import BasicFittingView
from Muon.GUI.Common.thread_model import ThreadModel
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput


class BasicFittingPresenter:
    """
    The BasicFittingPresenter holds a BasicFittingView and BasicFittingModel.
    """

    def __init__(self, view: BasicFittingView, model: BasicFittingModel):
        """Initialize the BasicFittingPresenter. Sets up the slots and event observers."""
        self.view = view
        self.model = model

        self.initialize_model_options()

        self.thread_success = True
        self.enable_editing_notifier = GenericObservable()
        self.disable_editing_notifier = GenericObservable()
        self.reset_tab_notifier = GenericObservable()
        self.fitting_calculation_model = None

        self.fit_function_changed_notifier = GenericObservable()
        self.fit_parameter_changed_notifier = GenericObservable()
        self.selected_fit_results_changed = GenericObservable()

        self.input_workspace_observer = GenericObserver(self.handle_new_data_loaded)
        self.gui_context_observer = GenericObserverWithArgPassing(self.handle_gui_changes_made)
        self.update_view_from_model_observer = GenericObserverWithArgPassing(
            self.handle_ads_clear_or_remove_workspace_event)

        self.fsg_model = None
        self.fsg_view = None
        self.fsg_presenter = None

        self.view.set_slot_for_fit_generator_clicked(self.handle_fit_generator_clicked)
        self.view.set_slot_for_fit_button_clicked(self.handle_fit_clicked)
        self.view.set_slot_for_undo_fit_clicked(self.handle_undo_fit_clicked)
        self.view.set_slot_for_plot_guess_changed(self.handle_plot_guess_changed)
        self.view.set_slot_for_fit_name_changed(self.handle_function_name_changed_by_user)
        self.view.set_slot_for_function_structure_changed(self.handle_function_structure_changed)
        self.view.set_slot_for_function_parameter_changed(self.handle_function_parameter_changed)
        self.view.set_slot_for_start_x_updated(self.handle_start_x_updated)
        self.view.set_slot_for_end_x_updated(self.handle_end_x_updated)
        self.view.set_slot_for_minimizer_changed(self.handle_minimizer_changed)
        self.view.set_slot_for_evaluation_type_changed(self.handle_evaluation_type_changed)
        self.view.set_slot_for_use_raw_changed(self.handle_use_rebin_changed)

    def initialize_model_options(self) -> None:
        """Initialise the model with the default fitting options."""
        self.model.minimizer = self.view.minimizer
        self.model.evaluation_type = self.view.evaluation_type
        self.model.fit_to_raw = self.view.fit_to_raw

    def handle_ads_clear_or_remove_workspace_event(self, _: str = None) -> None:
        """Handle when there is a clear or remove workspace event in the ADS."""
        self.update_and_reset_all_data()

        if self.model.number_of_datasets == 0:
            self.reset_tab_notifier.notify_subscribers()
        else:
            self.enable_editing_notifier.notify_subscribers()

    def handle_gui_changes_made(self, changed_values: dict) -> None:
        """Handle when the good data checkbox is changed in the home tab."""
        for key in changed_values.keys():
            if key in ["FirstGoodDataFromFile", "FirstGoodData"]:
                self.reset_start_xs_and_end_xs()

    def handle_new_data_loaded(self) -> None:
        """Handle when new data has been loaded into the interface."""
        self.update_and_reset_all_data()

        self.view.plot_guess = False
        self.clear_cached_fit_functions()

        if self.model.number_of_datasets == 0:
            self.reset_tab_notifier.notify_subscribers()
        else:
            self.enable_editing_notifier.notify_subscribers()

    def handle_plot_guess_changed(self) -> None:
        """Handle when plot guess is ticked or un-ticked."""
        self.model.update_plot_guess(self.view.plot_guess)

    def handle_undo_fit_clicked(self) -> None:
        """Handle when undo fit is clicked."""
        self.model.use_cached_function()
        self.clear_cached_fit_functions()

        self.reset_fit_status_and_chi_squared_information()

        self.update_fit_function_in_view_from_model()

        self.model.update_plot_guess(self.view.plot_guess)

        self.selected_fit_results_changed.notify_subscribers(self.model.get_active_fit_results())

    def handle_fit_clicked(self) -> None:
        """Handle when the fit button is clicked."""
        if self.model.number_of_datasets < 1:
            self.view.warning_popup("No data selected for fitting.")
            return
        if not self.view.fit_object:
            return

        self.model.cache_the_current_fit_functions()
        self._perform_fit()

    def handle_started(self) -> None:
        """Handle when fitting has started."""
        self.disable_editing_notifier.notify_subscribers()
        self.thread_success = True

    def handle_finished(self) -> None:
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

    def handle_fitting_finished(self) -> None:
        """Handle when fitting is finished."""
        raise NotImplementedError("This method must be overridden by a child class.")

    def handle_error(self, error: str) -> None:
        """Handle when an error occurs while fitting."""
        self.enable_editing_notifier.notify_subscribers()
        self.thread_success = False
        self.view.warning_popup(error)

    def handle_fit_generator_clicked(self) -> None:
        """Handle when the Fit Generator button has been clicked."""
        fitting_mode = FittingMode.SIMULTANEOUS if self.model.simultaneous_fitting_mode else FittingMode.SEQUENTIAL
        self._open_fit_script_generator_interface(self.model.dataset_names, fitting_mode,
                                                  self._get_fit_browser_options())

    def handle_function_name_changed_by_user(self) -> None:
        """Handle when the fit name is changed by the user."""
        self.model.function_name_auto_update = False
        self.model.function_name = self.view.function_name

    def handle_minimizer_changed(self) -> None:
        """Handle when a minimizer is changed."""
        self.model.minimizer = self.view.minimizer

    def handle_evaluation_type_changed(self) -> None:
        """Handle when the evaluation type is changed."""
        self.model.evaluation_type = self.view.evaluation_type

    def handle_function_structure_changed(self) -> None:
        """Handle when the function structure is changed."""
        self.update_fit_functions_in_model_from_view()
        self.automatically_update_function_name()

        if self.model.get_active_fit_function() is None:
            self.clear_cached_fit_functions()
            self.selected_fit_results_changed.notify_subscribers(self.model.get_active_fit_results())

        self.reset_fit_status_and_chi_squared_information()

        self.model.update_plot_guess(self.view.plot_guess)

        self.fit_function_changed_notifier.notify_subscribers()

    def handle_function_parameter_changed(self) -> None:
        """Handle when the value of a parameter in a function is changed."""
        self.update_fit_functions_in_model_from_view()

        self.model.update_plot_guess(self.view.plot_guess)

        self.fit_parameter_changed_notifier.notify_subscribers()

    def handle_start_x_updated(self) -> None:
        """Handle when the start X is changed."""
        if self.view.start_x > self.view.end_x:
            self.view.start_x, self.view.end_x = self.view.end_x, self.view.start_x
            self.model.current_end_x = self.view.end_x

        self.model.current_start_x = self.view.start_x

    def handle_end_x_updated(self) -> None:
        """Handle when the end X is changed."""
        if self.view.end_x < self.view.start_x:
            self.view.start_x, self.view.end_x = self.view.end_x, self.view.start_x
            self.model.current_start_x = self.view.start_x

        self.model.current_end_x = self.view.end_x

    def handle_use_rebin_changed(self) -> None:
        """Handle the Fit to raw data checkbox state change."""
        if self._check_rebin_options():
            self.model.fit_to_raw = self.view.fit_to_raw

    def clear_cached_fit_functions(self) -> None:
        """Clear the cached fit functions."""
        self.view.enable_undo_fit(False)
        self.model.remove_all_fits_from_context()
        self.model.clear_cached_fit_functions()

    def reset_fit_status_and_chi_squared_information(self) -> None:
        """Clear the fit status and chi squared information in the view and model."""
        self.model.reset_fit_statuses_and_chi_squared()
        self.update_fit_statuses_and_chi_squared_in_view_from_model()

    def reset_start_xs_and_end_xs(self) -> None:
        """Reset the start Xs and end Xs using the data stored in the context."""
        self.model.reset_start_xs_and_end_xs()
        self.view.start_x = self.model.current_start_x
        self.view.end_x = self.model.current_end_x

    def set_current_dataset_index(self, dataset_index: int) -> None:
        """Set the current dataset index in the model and view."""
        self.model.current_dataset_index = dataset_index
        self.view.set_current_dataset_index(dataset_index)

    def automatically_update_function_name(self) -> None:
        """Updates the function name used within the outputted fit workspaces."""
        self.model.automatically_update_function_name()
        self.view.function_name = self.model.function_name

    def update_and_reset_all_data(self) -> None:
        """Updates the various data displayed in the fitting widget. Resets and clears previous fit information."""
        raise NotImplementedError("This method must be overridden by a child class.")

    def update_dataset_names_in_view_and_model(self) -> None:
        """Updates the datasets currently displayed. The simultaneous fit by specifier must be updated before this."""
        self.model.dataset_names = self.model.get_workspace_names_to_display_from_context()
        self.view.set_datasets_in_function_browser(self.model.dataset_names)

    def update_fit_function_in_view_from_model(self) -> None:
        """Updates the parameters of a fit function shown in the view."""
        self.view.update_fit_function(self.model.get_active_fit_function(), self.model.global_parameters)
        self.view.set_current_dataset_index(self.model.current_dataset_index)

    def update_fit_functions_in_model_from_view(self) -> None:
        """Updates the fit functions stored in the model using the view."""
        self.update_single_fit_functions_in_model()
        self.fit_function_changed_notifier.notify_subscribers()

    def update_single_fit_functions_in_model(self) -> None:
        """Updates the single fit functions in the model using the view."""
        self.model.single_fit_functions = self._get_single_fit_functions_from_view()

    def update_fit_statuses_and_chi_squared_in_view_from_model(self) -> None:
        """Updates the local and global fit status and chi squared in the view."""
        self.view.update_local_fit_status_and_chi_squared(self.model.current_fit_status,
                                                          self.model.current_chi_squared)
        self.view.update_global_fit_status(self.model.fit_statuses, self.model.current_dataset_index)

    def _get_single_fit_functions_from_view(self) -> list:
        """Returns the fit functions corresponding to each domain as a list."""
        if self.view.fit_object:
            if isinstance(self.view.fit_object, MultiDomainFunction):
                return [function.clone() for function in self.view.fit_object.createEquivalentFunctions()]
            return [self.view.fit_object]
        return [None] * self.view.number_of_datasets()

    def _get_fit_browser_options(self) -> dict:
        """Returns the fitting options to use in the Fit Script Generator interface."""
        return {"Minimizer": self.model.minimizer, "EvaluationType": self.model.evaluation_type}

    def _perform_fit(self) -> None:
        """Perform the fit in a thread."""
        try:
            self.calculation_thread = self._create_thread(self.model.perform_fit)
            self.calculation_thread.threadWrapperSetUp(self.handle_started,
                                                       self.handle_finished,
                                                       self.handle_error)
            self.calculation_thread.start()
        except ValueError as error:
            self.view.warning_popup(error)

    def _create_thread(self, callback) -> ThreadModel:
        """Create a thread for fitting."""
        self.fitting_calculation_model = ThreadModelWrapperWithOutput(callback)
        return ThreadModel(self.fitting_calculation_model)

    def _open_fit_script_generator_interface(self, workspaces: list, fitting_mode: FittingMode,
                                             fit_options: dict) -> None:
        """Open the Fit Script Generator interface."""
        self.fsg_model = FitScriptGeneratorModel()
        self.fsg_view = FitScriptGeneratorView(None, fitting_mode, fit_options)
        self.fsg_presenter = FitScriptGeneratorPresenter(self.fsg_view, self.fsg_model, workspaces,
                                                         self.view.start_x, self.view.end_x)

        self.fsg_presenter.openFitScriptGenerator()

    def _check_rebin_options(self) -> bool:
        """Check that a rebin was indeed requested in the fitting tab or in the context."""
        if not self.view.fit_to_raw and not self.model.do_rebin:
            self.view.fit_to_raw = True
            self.view.warning_popup("No rebin options specified.")
            return False
        return True
