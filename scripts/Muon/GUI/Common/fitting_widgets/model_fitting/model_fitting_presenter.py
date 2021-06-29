# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing

from Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_presenter import BasicFittingPresenter
from Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_model import ModelFittingModel
from Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_view import ModelFittingView
from Muon.GUI.Common.thread_model import ThreadModel
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput


class ModelFittingPresenter(BasicFittingPresenter):
    """
    The ModelFittingPresenter has a ModelFittingView and ModelFittingModel and derives from BasicFittingPresenter.
    """

    def __init__(self, view: ModelFittingView, model: ModelFittingModel):
        """Initialize the ModelFittingPresenter. Sets up the slots and event observers."""
        super(ModelFittingPresenter, self).__init__(view, model)

        self.parameter_combination_thread_success = True

        self.results_table_created_observer = GenericObserverWithArgPassing(self.handle_new_results_table_created)

        self.view.set_slot_for_results_table_changed(self.handle_results_table_changed)
        self.view.set_slot_for_selected_x_changed(self.handle_selected_x_changed)
        self.view.set_slot_for_selected_y_changed(self.handle_selected_y_changed)

    def handle_new_results_table_created(self, new_results_table_name: str) -> None:
        """Handles when a new results table is created and added to the results context."""
        if new_results_table_name not in self.model.result_table_names:
            self.model.result_table_names = self.model.result_table_names + [new_results_table_name]
            self.view.add_results_table_name(new_results_table_name)
        elif self.model.current_result_table_name == new_results_table_name:
            self.handle_results_table_changed()

    def handle_results_table_changed(self) -> None:
        """Handles when the selected results table has changed, and discovers the possible X's and Y's."""
        self.model.current_result_table_index = self.view.current_result_table_index
        self._create_parameter_combination_workspaces(self.handle_parameter_combinations_finished)

    def handle_selected_x_changed(self) -> None:
        """Handles when the selected X parameter is changed."""
        x_parameter = self.view.x_parameter()
        if x_parameter == self.view.y_parameter():
            self.view.set_selected_y_parameter(self.model.get_first_y_parameter_not(x_parameter))

        self.update_selected_parameter_combination_workspace()

    def handle_selected_y_changed(self) -> None:
        """Handles when the selected Y parameter is changed."""
        y_parameter = self.view.y_parameter()
        if y_parameter == self.view.x_parameter():
            self.view.set_selected_x_parameter(self.model.get_first_x_parameter_not(y_parameter))

        self.update_selected_parameter_combination_workspace()

    def handle_parameter_combinations_started(self) -> None:
        """Handle when the creation of matrix workspaces starts for all the different parameter combinations."""
        self.disable_editing_notifier.notify_subscribers()
        self.parameter_combination_thread_success = True

    def handle_parameter_combinations_finished(self) -> None:
        """Handle when the creation of matrix workspaces finishes for all the different parameter combinations."""
        self.enable_editing_notifier.notify_subscribers()
        if not self.parameter_combination_thread_success:
            return

        self.handle_parameter_combinations_created_successfully()

    def handle_parameter_combinations_finished_before_fit(self) -> None:
        """Handle when the creation of the parameter combinations finishes, and then performs a fit."""
        self.handle_parameter_combinations_finished()
        super().handle_fit_clicked()

    def handle_parameter_combinations_created_successfully(self) -> None:
        """Handles when the parameter combination workspaces have been created successfully."""
        self.view.set_datasets_in_function_browser(self.model.dataset_names)
        self.view.update_dataset_name_combo_box(self.model.dataset_names)

        # Initially, the y parameters should be updated before the x parameters.
        self.view.update_y_parameters(self.model.y_parameters())
        # Triggers handle_selected_x_changed
        self.view.update_x_parameters(self.model.x_parameters(), emit_signal=True)

    def handle_parameter_combinations_error(self, error: str) -> None:
        """Handle when an error occurs while creating workspaces for the different parameter combinations."""
        self.disable_editing_notifier.notify_subscribers()
        self.parameter_combination_thread_success = False
        self.view.warning_popup(error)

    def handle_dataset_name_changed(self) -> None:
        """Handle when the hidden dataset workspace combo box is changed."""
        super().handle_dataset_name_changed()
        self.automatically_update_function_name()

    def handle_function_structure_changed(self) -> None:
        """Handle when the function structure is changed."""
        self.update_fit_functions_in_model_from_view()
        self.automatically_update_function_name()

        if self.model.get_active_fit_function() is None:
            self.clear_current_fit_function_for_undo()
            self.selected_fit_results_changed.notify_subscribers(self.model.get_active_fit_results())

        self.reset_fit_status_and_chi_squared_information()

        self.update_plot_guess()

        self.fit_function_changed_notifier.notify_subscribers()

        # Required to update the function browser to display the errors when first adding a function.
        self.view.set_current_dataset_index(self.model.current_dataset_index)

    def handle_fit_clicked(self) -> None:
        """Handle when the fit button is clicked."""
        current_dataset_name = self.model.current_dataset_name
        if current_dataset_name is not None and not check_if_workspace_exist(current_dataset_name):
            self._create_parameter_combination_workspaces(self.handle_parameter_combinations_finished_before_fit)
        else:
            super().handle_fit_clicked()

    def update_dataset_names_in_view_and_model(self) -> None:
        """Updates the results tables currently displayed."""
        self.model.result_table_names = self.model.get_workspace_names_to_display_from_context()
        if self.model.result_table_names != self.view.result_table_names():
            # Triggers handle_results_table_changed
            self.view.update_result_table_names(self.model.result_table_names)

    def update_fit_functions_in_model_from_view(self) -> None:
        """Update the fit function in the model only for the currently selected dataset."""
        self.model.current_single_fit_function = self.view.current_fit_function()

    def update_selected_parameter_combination_workspace(self) -> None:
        """Updates the selected parameter combination based on the selected X and Y parameters."""
        dataset_name = self.model.parameter_combination_workspace_name(self.view.x_parameter(), self.view.y_parameter())
        if dataset_name is not None:
            self.model.current_dataset_index = self.model.dataset_names.index(dataset_name)
            self.view.current_dataset_name = dataset_name

    def reset_fit_status_and_chi_squared_information(self) -> None:
        """Reset the fit status and chi squared only for the currently selected dataset."""
        self.model.reset_current_fit_status_and_chi_squared()
        self.update_fit_statuses_and_chi_squared_in_view_from_model()

    def clear_current_fit_function_for_undo(self) -> None:
        """Clear the cached fit function for the currently selected dataset."""
        self.model.clear_undo_data_for_current_dataset_index()
        self.view.set_number_of_undos(self.model.number_of_undos())

    def _create_parameter_combination_workspaces(self, finished_callback) -> None:
        """Creates a matrix workspace for each possible parameter combination to be used for fitting."""
        try:
            self.parameter_combination_thread = self._create_parameter_combinations_thread(
                self.model.create_x_and_y_parameter_combination_workspaces)
            self.parameter_combination_thread.threadWrapperSetUp(self.handle_parameter_combinations_started,
                                                                 finished_callback,
                                                                 self.handle_parameter_combinations_error)
            self.parameter_combination_thread.start()
        except ValueError as error:
            self.view.warning_popup(error)

    def _create_parameter_combinations_thread(self, callback) -> ThreadModel:
        """Create a thread for fitting."""
        self.parameter_combinations_creator = ThreadModelWrapperWithOutput(callback)
        return ThreadModel(self.parameter_combinations_creator)
