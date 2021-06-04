# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
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

        self.view.set_slot_for_results_table_changed(self.handle_results_table_changed)
        self.view.set_slot_for_selected_x_changed(self.handle_selected_x_and_y_changed)
        self.view.set_slot_for_selected_y_changed(self.handle_selected_x_and_y_changed)

    def initialize_model_options(self) -> None:
        """Returns the fitting options to be used when initializing the model."""
        super().initialize_model_options()

    def handle_results_table_changed(self) -> None:
        """Handles when the selected results table has changed, and discovers the possible X's and Y's."""
        self.model.current_result_table_index = self.view.current_result_table_index
        self._create_parameter_combination_workspaces()

    def handle_selected_x_and_y_changed(self) -> None:
        """Handles when the selected X and Y parameters are changed."""
        dataset_name = self.model.parameter_combination_workspace_name(self.view.x_parameter(), self.view.y_parameter())
        self.model.current_dataset_index = self.model.dataset_names.index(dataset_name)
        self.view.current_dataset_name = dataset_name

    def handle_parameter_combinations_started(self) -> None:
        """Handle when the creation of matrix workspaces starts for all the different parameter combinations."""
        self.disable_editing_notifier.notify_subscribers()
        self.parameter_combination_thread_success = True

    def handle_parameter_combinations_finished(self) -> None:
        """Handle when the creation of matrix workspaces finishes for all the different parameter combinations."""
        self.enable_editing_notifier.notify_subscribers()
        if not self.parameter_combination_thread_success:
            return

        x_parameters, y_parameters = self.parameter_combinations_creator.result
        if len(x_parameters) == 0 or len(y_parameters) == 0:
            return

        self.handle_parameter_combinations_created_successfully(x_parameters, y_parameters)

    def handle_parameter_combinations_created_successfully(self, x_parameters: list, y_parameters: list) -> None:
        """Handles when the parameter combination workspaces have been created successfully."""
        self.view.update_x_and_y_parameters(x_parameters, y_parameters)
        self.view.set_datasets_in_function_browser(self.model.dataset_names)
        self.view.update_dataset_name_combo_box(self.model.dataset_names)

        self.handle_selected_x_and_y_changed()

    def handle_parameter_combinations_error(self, error: str) -> None:
        """Handle when an error occurs while creating workspaces for the different parameter combinations."""
        self.disable_editing_notifier.notify_subscribers()
        self.parameter_combination_thread_success = False
        self.view.warning_popup(error)

    def update_dataset_names_in_view_and_model(self) -> None:
        """Updates the results tables currently displayed."""
        self.model.result_table_names = self.model.get_workspace_names_to_display_from_context()
        # Triggers handle_results_table_changed
        self.view.update_result_table_names(self.model.result_table_names)

    def _create_parameter_combination_workspaces(self) -> None:
        """Creates a matrix workspace for each possible parameter combination to be used for fitting."""
        try:
            self.parameter_combination_thread = self._create_parameter_combinations_thread(
                self.model.create_x_and_y_parameter_combination_workspaces)
            self.parameter_combination_thread.threadWrapperSetUp(self.handle_parameter_combinations_started,
                                                                 self.handle_parameter_combinations_finished,
                                                                 self.handle_parameter_combinations_error)
            self.parameter_combination_thread.start()
        except ValueError as error:
            self.view.warning_popup(error)

    def _create_parameter_combinations_thread(self, callback) -> ThreadModel:
        """Create a thread for fitting."""
        self.parameter_combinations_creator = ThreadModelWrapperWithOutput(callback)
        return ThreadModel(self.parameter_combinations_creator)
