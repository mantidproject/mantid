# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_view import BasicFittingView
from Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_data_selector_view import ModelFittingDataSelectorView

from qtpy.QtWidgets import QWidget


class ModelFittingView(BasicFittingView):
    """
    The ModelFittingView derives from the BasicFittingView. It adds the ModelFittingDataSelectorView to the widget.
    """

    def __init__(self, parent: QWidget = None):
        """Initializes the ModelFittingView, and adds the ModelFittingDataSelectorView widget."""
        super(ModelFittingView, self).__init__(parent)

        self.model_fitting_data_selector = ModelFittingDataSelectorView(self)
        self.general_fitting_options_layout.addWidget(self.model_fitting_data_selector)

        # Hide the workspace selector which is used to store the generated matrix workspaces
        self.workspace_selector.hide()

    def set_slot_for_results_table_changed(self, slot) -> None:
        """Connect the slot for the result tables combo box being changed."""
        self.model_fitting_data_selector.set_slot_for_results_table_changed(slot)

    def set_slot_for_selected_x_changed(self, slot) -> None:
        """Connect the slot for when the selected X changes."""
        self.model_fitting_data_selector.set_slot_for_selected_x_changed(slot)

    def set_slot_for_selected_y_changed(self, slot) -> None:
        """Connect the slot for when the selected Y changes."""
        self.model_fitting_data_selector.set_slot_for_selected_y_changed(slot)

    def result_table_names(self) -> list:
        """Returns a list of result table names currently loaded into model fitting."""
        return self.model_fitting_data_selector.result_table_names()

    def add_results_table_name(self, results_table_name: str) -> None:
        """Add a results table to the results table combo box."""
        self.model_fitting_data_selector.add_results_table_name(results_table_name)

    def update_result_table_names(self, table_names: list) -> None:
        """Update the data in the results table combo box."""
        self.model_fitting_data_selector.update_result_table_names(table_names)

    def update_x_parameters(self, x_parameters: list, emit_signal: bool = False) -> None:
        """Update the available X parameters."""
        self.model_fitting_data_selector.update_x_parameters(x_parameters, emit_signal)

    def update_y_parameters(self, y_parameters: list, emit_signal: bool = False) -> None:
        """Update the available Y parameters."""
        self.model_fitting_data_selector.update_y_parameters(y_parameters, emit_signal)

    def update_fit_function(self, fit_function: IFunction) -> None:
        """Updates the fit function shown in the view."""
        self.fit_function_options.set_fit_function(fit_function)

    def set_selected_x_parameter(self, x_parameter: str) -> None:
        """Sets the selected X parameter."""
        self.model_fitting_data_selector.set_selected_x_parameter(x_parameter)

    def set_selected_y_parameter(self, y_parameter: str) -> None:
        """Sets the selected Y parameter."""
        self.model_fitting_data_selector.set_selected_y_parameter(y_parameter)

    @property
    def current_result_table_index(self) -> str:
        """Returns the index of the currently displayed result table."""
        return self.model_fitting_data_selector.current_result_table_index

    def x_parameter(self) -> str:
        """Returns the selected X parameter name."""
        return self.model_fitting_data_selector.x_parameter

    def y_parameter(self) -> str:
        """Returns the selected Y parameter name."""
        return self.model_fitting_data_selector.y_parameter

    def enable_view(self) -> None:
        """Enable all widgets in this fitting widget."""
        if self.model_fitting_data_selector.number_of_result_tables() > 0:
            self.setEnabled(True)
