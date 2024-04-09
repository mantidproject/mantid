# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction, MultiDomainFunction
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing, GenericObservable, GenericObserver
from mantidqt.widgets.fitscriptgenerator import FittingMode, FitScriptGeneratorModel, FitScriptGeneratorPresenter, FitScriptGeneratorView

from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.basic_fitting_context import (
    X_FROM_FIT_RANGE,
    X_FROM_DATA_RANGE,
    X_FROM_CUSTOM,
)
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import BasicFittingModel
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_view import BasicFittingView
from mantidqtinterfaces.Muon.GUI.Common.thread_model import ThreadModel
from mantidqtinterfaces.Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput
from mantidqtinterfaces.Muon.GUI.Common.utilities.workspace_data_utils import (
    check_exclude_start_x_is_valid,
    check_exclude_end_x_is_valid,
    check_start_x_is_valid,
    check_end_x_is_valid,
)

from PyQt5.QtCore import Qt


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
        self.fitting_calculation_model = None

        self.remove_plot_guess_notifier = GenericObservable()
        self.update_plot_guess_notifier = GenericObservable()

        self.fit_function_changed_notifier = GenericObservable()
        self.fit_parameter_changed_notifier = GenericObservable()
        self.selected_fit_results_changed = GenericObservable()

        self.input_workspace_observer = GenericObserver(self.handle_new_data_loaded)
        self.gui_context_observer = GenericObserverWithArgPassing(self.handle_gui_changes_made)
        self.update_view_from_model_observer = GenericObserverWithArgPassing(self.handle_ads_clear_or_remove_workspace_event)
        self.instrument_changed_observer = GenericObserver(self.handle_instrument_changed)
        self.selected_group_pair_observer = GenericObserver(self.handle_selected_group_pair_changed)
        self.double_pulse_observer = GenericObserverWithArgPassing(self.handle_pulse_type_changed)
        self.sequential_fit_finished_observer = GenericObserver(self.handle_sequential_fit_finished)
        self.fit_parameter_updated_observer = GenericObserver(self.update_fit_function_in_view_from_model)

        self.fsg_model = None
        self.fsg_view = None
        self.fsg_presenter = None

        self.view.set_slot_for_fit_generator_clicked(self.handle_fit_generator_clicked)
        self.view.set_slot_for_fit_button_clicked(self.handle_fit_clicked)
        self.view.set_slot_for_undo_fit_clicked(self.handle_undo_fit_clicked)
        self.view.set_slot_for_plot_guess_changed(self.handle_plot_guess_changed)
        self.view.set_slot_for_fit_name_changed(self.handle_function_name_changed_by_user)
        self.view.set_slot_for_dataset_changed(self.handle_dataset_name_changed)
        self.view.set_slot_for_covariance_matrix_clicked(self.handle_covariance_matrix_clicked)
        self.view.set_slot_for_function_structure_changed(self.handle_function_structure_changed)
        self.view.set_slot_for_function_parameter_changed(
            lambda function_index, parameter: self.handle_function_parameter_changed(function_index, parameter)
        )
        self.view.set_slot_for_function_attribute_changed(lambda attribute: self.handle_function_attribute_changed(attribute))
        self.view.set_slot_for_start_x_updated(self.handle_start_x_updated)
        self.view.set_slot_for_end_x_updated(self.handle_end_x_updated)
        self.view.set_slot_for_exclude_range_state_changed(self.handle_exclude_range_state_changed)
        self.view.set_slot_for_exclude_start_x_updated(self.handle_exclude_start_x_updated)
        self.view.set_slot_for_exclude_end_x_updated(self.handle_exclude_end_x_updated)
        self.view.set_slot_for_minimizer_changed(self.handle_minimizer_changed)
        self.view.set_slot_for_evaluation_type_changed(self.handle_evaluation_type_changed)
        self.view.set_slot_for_plot_guess_type_changed(self.handle_plot_guess_type_changed)
        self.view.set_slot_for_plot_guess_points_updated(self.handle_plot_guess_points_changed)
        self.view.set_slot_for_plot_guess_start_x_updated(self.handle_plot_guess_start_x_changed)
        self.view.set_slot_for_plot_guess_end_x_updated(self.handle_plot_guess_end_x_changed)
        self.view.set_slot_for_use_raw_changed(self.handle_use_rebin_changed)

    def initialize_model_options(self) -> None:
        """Initialise the model with the default fitting options."""
        self.model.minimizer = self.view.minimizer
        self.model.evaluation_type = self.view.evaluation_type
        self.model.fit_to_raw = self.view.fit_to_raw

    def handle_ads_clear_or_remove_workspace_event(self, name: str) -> None:
        """Handle when there is a clear or remove workspace event in the ADS."""
        if name not in self.model.dataset_names:
            return

        self.update_and_reset_all_data()

    def handle_gui_changes_made(self, changed_values: dict) -> None:
        """Handle when the good data checkbox is changed in the home tab."""
        for key in changed_values.keys():
            if key in ["FirstGoodDataFromFile", "FirstGoodData"]:
                self.reset_start_xs_and_end_xs()

    def handle_new_data_loaded(self) -> None:
        """Handle when new data has been loaded into the interface."""
        self.update_and_reset_all_data()

        self.view.plot_guess, self.model.plot_guess = False, False
        self.clear_undo_data()

    def handle_instrument_changed(self) -> None:
        """Handles when an instrument is changed and switches to normal fitting mode. Overridden by child."""
        self.update_and_reset_all_data()
        self.clear_undo_data()
        self.model.remove_all_fits_from_context()

    def handle_selected_group_pair_changed(self) -> None:
        """Update the displayed workspaces when the selected group/pairs change in grouping tab."""
        self.update_and_reset_all_data()

    def handle_pulse_type_changed(self, updated_variables: dict) -> None:
        """Handles when double pulse mode is switched on and switches to normal fitting mode."""
        if "DoublePulseEnabled" in updated_variables:
            self.update_and_reset_all_data()

    def handle_plot_guess_changed(self) -> None:
        """Handle when plot guess is ticked or un-ticked."""
        self.model.plot_guess = self.view.plot_guess
        self.update_guess_parameters()
        self.update_plot_guess()

    def handle_undo_fit_clicked(self) -> None:
        """Handle when undo fit is clicked."""
        self.model.undo_previous_fit()
        self.view.set_number_of_undos(self.model.number_of_undos())

        self.update_fit_function_in_view_from_model()
        self.update_fit_statuses_and_chi_squared_in_view_from_model()
        self.update_covariance_matrix_button()

        self.update_plot_fit()
        self.update_plot_guess()

    def handle_fit_clicked(self) -> None:
        """Handle when the fit button is clicked."""
        if self.model.number_of_datasets < 1:
            self.view.warning_popup("No data selected for fitting.")
            return
        if not self.view.fit_object:
            return

        self.model.save_current_fit_function_to_undo_data()
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

        fit_result = self.fitting_calculation_model.result
        if fit_result is None:
            return

        fit_function, fit_status, fit_chi_squared = fit_result
        if any([not fit_function, not fit_status, fit_chi_squared != 0.0 and not fit_chi_squared]):
            return

        self.handle_fitting_finished(fit_function, fit_status, fit_chi_squared)
        self.view.set_number_of_undos(self.model.number_of_undos())
        self.view.plot_guess, self.model.plot_guess = False, False

    def handle_fitting_finished(self, fit_function, fit_status, chi_squared) -> None:
        """Handle when fitting is finished."""
        self.update_fit_statuses_and_chi_squared_in_model(fit_status, chi_squared)
        self.update_fit_function_in_model(fit_function)

        self.update_fit_statuses_and_chi_squared_in_view_from_model()
        self.update_covariance_matrix_button()
        self.update_fit_function_in_view_from_model()

        self.update_plot_fit()
        self.fit_parameter_changed_notifier.notify_subscribers()

    def handle_error(self, error: str) -> None:
        """Handle when an error occurs while fitting."""
        self.enable_editing_notifier.notify_subscribers()
        self.thread_success = False
        self.view.warning_popup(error)

    def handle_fit_generator_clicked(self) -> None:
        """Handle when the Fit Generator button has been clicked."""
        fitting_mode = FittingMode.SIMULTANEOUS if self.model.simultaneous_fitting_mode else FittingMode.SEQUENTIAL
        self._open_fit_script_generator_interface(self.model.dataset_names, fitting_mode, self._get_fit_browser_options())

    def handle_dataset_name_changed(self) -> None:
        """Handle when the display workspace combo box is changed."""
        self.model.current_dataset_index = self.view.current_dataset_index

        self.update_fit_statuses_and_chi_squared_in_view_from_model()
        self.update_covariance_matrix_button()
        self.update_fit_function_in_view_from_model()
        self.update_start_and_end_x_in_view_from_model()

        self.update_plot_fit()
        self.update_guess_parameters()
        self.update_plot_guess()

    def handle_covariance_matrix_clicked(self) -> None:
        """Handle when the Covariance Matrix button is clicked."""
        covariance_matrix = self.model.current_normalised_covariance_matrix()
        if covariance_matrix is not None:
            self.view.show_normalised_covariance_matrix(covariance_matrix.workspace, covariance_matrix.workspace_name)

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

    def handle_plot_guess_type_changed(self) -> None:
        """Handle when the evaluation type is changed."""
        self.model.plot_guess_type = self.view.plot_guess_type
        if self.model.plot_guess_type == X_FROM_FIT_RANGE:
            self.view.show_plot_guess_points(False)
            self.view.show_plot_guess_start_x(False)
            self.view.show_plot_guess_end_x(False)
        elif self.model.plot_guess_type == X_FROM_DATA_RANGE:
            self.view.show_plot_guess_points(True)
            self.view.show_plot_guess_start_x(False)
            self.view.show_plot_guess_end_x(False)
        elif self.model.plot_guess_type == X_FROM_CUSTOM:
            self.view.show_plot_guess_points(True)
            self.view.show_plot_guess_start_x(True)
            self.view.show_plot_guess_end_x(True)

        self.update_plot_guess()

    def handle_plot_guess_points_changed(self) -> None:
        """Handle when the evaluation type is changed."""
        self.model.plot_guess_points = self.view.plot_guess_points
        self.update_plot_guess()

    def handle_plot_guess_start_x_changed(self) -> None:
        """Handle when the evaluation type is changed."""
        self.model.plot_guess_start_x = self.view.plot_guess_start_x
        self.update_plot_guess()

    def handle_plot_guess_end_x_changed(self) -> None:
        """Handle when the evaluation type is changed."""
        self.model.plot_guess_end_x = self.view.plot_guess_end_x
        self.update_plot_guess()

    def handle_function_structure_changed(self) -> None:
        """Handle when the function structure is changed."""
        self.update_fit_functions_in_model_from_view()
        self.automatically_update_function_name()
        self.clear_undo_data()

        if self.model.get_active_fit_function() is None:
            self.update_plot_fit()

        self.reset_fit_status_and_chi_squared_information()

        self.update_plot_guess()

        self.fit_function_changed_notifier.notify_subscribers()

        # Required to update the function browser to display the errors when first adding a function.
        self.view.set_current_dataset_index(self.model.current_dataset_index)

    def handle_function_parameter_changed(self, function_index: str, parameter: str) -> None:
        """Handle when the value of a parameter in a function is changed."""
        full_parameter = f"{function_index}{parameter}"
        self.model.update_parameter_value(full_parameter, self.view.parameter_value(full_parameter))

        self.update_plot_guess()

        self.fit_function_changed_notifier.notify_subscribers()
        self.fit_parameter_changed_notifier.notify_subscribers()

    def handle_function_attribute_changed(self, attribute: str) -> None:
        """Handle when the value of a attribute in a function is changed."""
        self.model.update_attribute_value(attribute, self.view.attribute_value(attribute))

    def handle_start_x_updated(self) -> None:
        """Handle when the start X is changed."""
        new_start_x, new_end_x = check_start_x_is_valid(
            self.model.current_dataset_name, self.view.start_x, self.view.end_x, self.model.current_start_x
        )
        self.update_start_and_end_x_in_view_and_model(new_start_x, new_end_x)

    def handle_end_x_updated(self) -> None:
        """Handle when the end X is changed."""
        new_start_x, new_end_x = check_end_x_is_valid(
            self.model.current_dataset_name, self.view.start_x, self.view.end_x, self.model.current_end_x
        )
        self.update_start_and_end_x_in_view_and_model(new_start_x, new_end_x)

    def handle_exclude_range_state_changed(self) -> None:
        """Handles when Exclude Range is ticked or unticked."""
        self.model.exclude_range = self.view.exclude_range
        self.view.set_exclude_start_and_end_x_visible(self.model.exclude_range)

    def handle_exclude_start_x_updated(self) -> None:
        """Handle when the exclude start X is changed."""
        exclude_start_x, exclude_end_x = check_exclude_start_x_is_valid(
            self.view.start_x, self.view.end_x, self.view.exclude_start_x, self.view.exclude_end_x, self.model.current_exclude_start_x
        )
        self.update_exclude_start_and_end_x_in_view_and_model(exclude_start_x, exclude_end_x)

    def handle_exclude_end_x_updated(self) -> None:
        """Handle when the exclude end X is changed."""
        exclude_start_x, exclude_end_x = check_exclude_end_x_is_valid(
            self.view.start_x, self.view.end_x, self.view.exclude_start_x, self.view.exclude_end_x, self.model.current_exclude_end_x
        )
        self.update_exclude_start_and_end_x_in_view_and_model(exclude_start_x, exclude_end_x)

    def handle_use_rebin_changed(self) -> None:
        """Handle the Fit to raw data checkbox state change."""
        if self._check_rebin_options():
            self.model.fit_to_raw = self.view.fit_to_raw

    def handle_sequential_fit_finished(self) -> None:
        """Handles when a sequential fit has been performed and has finished executing in the sequential fitting tab."""
        self.update_fit_function_in_view_from_model()
        self.update_fit_statuses_and_chi_squared_in_view_from_model()

    def clear_undo_data(self) -> None:
        """Clear all the previously saved undo fit functions and other data."""
        self.model.clear_undo_data()
        self.view.set_number_of_undos(self.model.number_of_undos())

    def reset_fit_status_and_chi_squared_information(self) -> None:
        """Clear the fit status and chi squared information in the view and model."""
        self.model.reset_fit_statuses_and_chi_squared()
        self.update_fit_statuses_and_chi_squared_in_view_from_model()

    def reset_start_xs_and_end_xs(self) -> None:
        """Reset the start Xs and end Xs using the data stored in the context."""
        self.model.reset_start_xs_and_end_xs()
        self.view.start_x = self.model.current_start_x
        self.view.end_x = self.model.current_end_x
        self.view.exclude_start_x = self.model.current_exclude_start_x
        self.view.exclude_end_x = self.model.current_exclude_end_x

    def set_selected_dataset(self, dataset_name: str) -> None:
        """Sets the workspace to be displayed in the view programmatically."""
        # Triggers handle_dataset_name_changed which updates the model
        self.view.current_dataset_name = dataset_name

    def current_dataset(self) -> str:
        return self.view.current_dataset_name

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
        # Triggers handle_dataset_name_changed
        self.update_dataset_names_in_view_and_model()

    def update_dataset_names_in_view_and_model(self) -> None:
        """Updates the datasets currently displayed."""
        self.model.dataset_names = self.model.get_workspace_names_to_display_from_context()
        self.view.set_datasets_in_function_browser(self.model.dataset_names)
        self.view.update_dataset_name_combo_box(self.model.dataset_names)
        self.model.current_dataset_index = self.view.current_dataset_index

        if self.model.number_of_datasets == 0:
            self.view.disable_view()
        else:
            self.enable_editing_notifier.notify_subscribers()

    def update_fit_statuses_and_chi_squared_in_model(self, fit_status: str, chi_squared: float) -> None:
        """Updates the fit status and chi squared stored in the model. This is used after a fit."""
        self.model.current_fit_status = fit_status
        self.model.current_chi_squared = chi_squared

    def update_fit_function_in_model(self, fit_function: IFunction) -> None:
        """Updates the fit function stored in the model. This is used after a fit."""
        self.model.current_single_fit_function = fit_function

    def update_fit_function_in_view_from_model(self) -> None:
        """Updates the parameters of a fit function shown in the view."""
        self.view.set_current_dataset_index(self.model.current_dataset_index)
        self.view.update_fit_function(self.model.get_active_fit_function())

    def update_fit_functions_in_model_from_view(self) -> None:
        """Updates the fit functions stored in the model using the view."""
        self.update_single_fit_functions_in_model()
        self.fit_function_changed_notifier.notify_subscribers()

    def update_single_fit_functions_in_model(self) -> None:
        """Updates the single fit functions in the model using the view."""
        self.model.single_fit_functions = self._get_single_fit_functions_from_view()

    def update_fit_statuses_and_chi_squared_in_view_from_model(self) -> None:
        """Updates the local and global fit status and chi squared in the view."""
        self.view.update_local_fit_status_and_chi_squared(self.model.current_fit_status, self.model.current_chi_squared)
        self.view.update_global_fit_status(self.model.fit_statuses, self.model.current_dataset_index)

    def update_covariance_matrix_button(self) -> None:
        """Updates the covariance matrix button to be enabled if a covariance matrix exists for the selected data."""
        self.view.set_covariance_button_enabled(self.model.has_normalised_covariance_matrix())

    def update_start_and_end_x_in_view_from_model(self) -> None:
        """Updates the start and end x in the view using the current values in the model."""
        self.view.start_x = self.model.current_start_x
        self.view.end_x = self.model.current_end_x
        self.view.plot_guess_start_x = self.model.current_start_x
        self.view.plot_guess_end_x = self.model.current_end_x
        self.view.exclude_start_x = self.model.current_exclude_start_x
        self.view.exclude_end_x = self.model.current_exclude_end_x

    def update_exclude_start_and_end_x_in_view_and_model(self, exclude_start_x: float, exclude_end_x: float) -> None:
        """Updates the current exclude start and end X in the view and model."""
        self.view.exclude_start_x, self.view.exclude_end_x = exclude_start_x, exclude_end_x
        self.model.current_exclude_start_x, self.model.current_exclude_end_x = exclude_start_x, exclude_end_x

    def update_start_and_end_x_in_view_and_model(self, start_x: float, end_x: float) -> None:
        """Updates the start and end x in the model using the provided values."""
        self.view.start_x, self.view.end_x = start_x, end_x
        self.model.current_start_x, self.model.current_end_x = start_x, end_x

        self.update_plot_guess()

    def update_plot_guess(self) -> None:
        """Updates the guess plot using the current dataset and function."""
        self.remove_plot_guess_notifier.notify_subscribers()
        self.model.update_plot_guess()
        self.update_plot_guess_notifier.notify_subscribers()

    def update_guess_parameters(self) -> None:
        """Update the parameters for the guess plot"""
        self.model.plot_guess_type = self.view.plot_guess_type
        self.model.plot_guess_points = self.view.plot_guess_points
        self.model.plot_guess_start_x = self.view.plot_guess_start_x
        self.model.plot_guess_end_x = self.view.plot_guess_end_x

    def update_plot_fit(self) -> None:
        """Updates the fit results on the plot using the currently active fit results."""
        self.selected_fit_results_changed.notify_subscribers(self.model.get_active_fit_results())

    def _get_single_fit_functions_from_view(self) -> list:
        """Returns the fit functions corresponding to each domain as a list."""
        if self.view.fit_object:
            if isinstance(self.view.fit_object, MultiDomainFunction):
                return [function.clone() for function in self.view.fit_object.createEquivalentFunctions()]
            return [self.view.fit_object]
        return [None] * self.view.number_of_datasets()

    def _get_fit_browser_options(self) -> dict:
        """Returns the fitting options to use in the Fit Script Generator interface."""
        return {"Minimizer": self.model.minimizer, "Evaluation Type": self.model.evaluation_type}

    def _perform_fit(self) -> None:
        """Perform the fit in a thread."""
        try:
            self.calculation_thread = self._create_fitting_thread(self.model.perform_fit)
            self.calculation_thread.threadWrapperSetUp(self.handle_started, self.handle_finished, self.handle_error)
            self.calculation_thread.start()
        except ValueError as error:
            self.view.warning_popup(error)

    def _create_fitting_thread(self, callback) -> ThreadModel:
        """Create a thread for fitting."""
        self.fitting_calculation_model = ThreadModelWrapperWithOutput(callback)
        return ThreadModel(self.fitting_calculation_model)

    def _open_fit_script_generator_interface(self, workspaces: list, fitting_mode: FittingMode, fit_options: dict) -> None:
        """Open the Fit Script Generator interface."""
        self.fsg_model = FitScriptGeneratorModel()
        self.fsg_view = FitScriptGeneratorView(self.view, fitting_mode, fit_options)
        self.fsg_view.setWindowFlag(Qt.Window)
        self.fsg_presenter = FitScriptGeneratorPresenter(self.fsg_view, self.fsg_model, workspaces, self.view.start_x, self.view.end_x)

        self.fsg_presenter.openFitScriptGenerator()

    def _check_rebin_options(self) -> bool:
        """Check that a rebin was indeed requested in the fitting tab or in the context."""
        if not self.view.fit_to_raw and not self.model.do_rebin:
            self.view.fit_to_raw = True
            self.view.warning_popup("No rebin options specified.")
            return False
        return True
