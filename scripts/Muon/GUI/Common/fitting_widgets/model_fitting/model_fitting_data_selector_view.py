# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui
from Muon.GUI.Common.fitting_widgets.basic_fitting.workspace_selector_view import WorkspaceSelectorView

from qtpy.QtWidgets import QWidget

ui_form, base_widget = load_ui(__file__, "model_fitting_data_selector.ui")


class ModelFittingDataSelectorView(ui_form, base_widget):
    """
    The ModelFittingDataSelectorView includes the cyclic results table data selector, and two combo boxes to select X
    and Y data.
    """

    def __init__(self, parent: QWidget = None):
        """Initializes the ModelFittingDataSelectorView."""
        super(ModelFittingDataSelectorView, self).__init__(parent)
        self.setupUi(self)

        self.result_table_selector = WorkspaceSelectorView(self)
        self.result_table_selector.set_workspace_combo_box_label("Results table")
        self.result_table_selector_layout.addWidget(self.result_table_selector)

    def set_slot_for_results_table_changed(self, slot) -> None:
        """Connect the slot for the result tables combo box being changed."""
        self.result_table_selector.set_slot_for_dataset_changed(slot)

    def set_slot_for_selected_x_changed(self, slot) -> None:
        """Connect the slot for when the selected X changes."""
        self.x_selector.currentIndexChanged.connect(slot)

    def set_slot_for_selected_y_changed(self, slot) -> None:
        """Connect the slot for when the selected Y changes."""
        self.y_selector.currentIndexChanged.connect(slot)

    def add_results_table_name(self, results_table_name: str) -> None:
        """Add a results table to the results table combo box."""
        self.result_table_selector.add_dataset_name(results_table_name)

    def update_result_table_names(self, table_names: list) -> None:
        """Update the data in the parameter display combo box."""
        self.result_table_selector.update_dataset_name_combo_box(table_names)

    def update_x_and_y_parameters(self, x_parameters: list, y_parameters: list) -> None:
        """Update the available X and Y parameters in the relevant combo boxes."""
        self.x_selector.blockSignals(True)
        self.y_selector.blockSignals(True)

        self.x_selector.clear()
        self.y_selector.clear()
        self.x_selector.addItems(x_parameters)
        self.y_selector.addItems(y_parameters)

        self.x_selector.blockSignals(False)
        self.y_selector.blockSignals(False)

    def number_of_result_tables(self) -> int:
        """Returns the number of result tables loaded into the widget."""
        return self.result_table_selector.number_of_datasets()

    @property
    def current_result_table_index(self) -> str:
        """Returns the index of the currently displayed result table."""
        return self.result_table_selector.current_dataset_index

    @property
    def x_parameter(self) -> str:
        """Returns the selected X parameter name."""
        return str(self.x_selector.currentText())

    @property
    def y_parameter(self) -> str:
        """Returns the selected Y parameter name."""
        return str(self.y_selector.currentText())
