# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui
from mantidqtinterfaces.Muon.GUI.Common.data_selectors.cyclic_data_selector_view import CyclicDataSelectorView
from mantidqtinterfaces.Muon.GUI.Common.results_tab_widget.results_tab_model import TableColumnType

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

        self.result_table_selector = CyclicDataSelectorView(self)
        self.result_table_selector.set_data_combo_box_label("Results table")
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

    def result_table_names(self) -> list:
        """Returns a list of result table names currently loaded into model fitting."""
        return self.result_table_selector.dataset_names

    def add_results_table_name(self, results_table_name: str) -> None:
        """Add a results table to the results table combo box."""
        self.result_table_selector.add_dataset_name(results_table_name)
        self.result_table_selector.set_current_dataset_index(self.result_table_selector.number_of_datasets()-1)

    def update_result_table_names(self, table_names: list) -> None:
        """Update the data in the parameter display combo box."""
        self.result_table_selector.update_dataset_name_combo_box(table_names)

    def update_x_parameters(self, x_parameters: list, x_parameter_types: list, emit_signal: bool = False) -> None:
        """Update the available X parameters."""
        old_x_parameter = self.x_selector.currentText()

        self.x_selector.blockSignals(True)
        self.x_selector.clear()
        self.x_selector.addItems(x_parameters)
        self.x_selector.blockSignals(False)

        new_x_parameter = self._get_x_parameter_for_update(old_x_parameter, x_parameters, x_parameter_types)
        new_index = self.set_selected_x_parameter(new_x_parameter)

        if emit_signal:
            # Signal is emitted manually in case the index has not changed (but the loaded parameter may be different)
            self.x_selector.currentIndexChanged.emit(new_index)

    def update_y_parameters(self, y_parameters: list, y_parameter_types: list, emit_signal: bool = False) -> None:
        """Update the available Y parameters."""
        old_y_parameter = self.y_selector.currentText()

        self.y_selector.blockSignals(True)
        self.y_selector.clear()
        self.y_selector.addItems(y_parameters)
        self.y_selector.blockSignals(False)

        new_y_parameter = self._get_y_parameter_for_update(old_y_parameter, y_parameters, y_parameter_types)
        new_index = self.set_selected_y_parameter(new_y_parameter)

        if emit_signal:
            # Signal is emitted manually in case the index has not changed (but the loaded parameter may be different)
            self.y_selector.currentIndexChanged.emit(new_index)

    def set_selected_x_parameter(self, x_parameter: str) -> int:
        """Sets the selected X parameter."""
        new_index = self.x_selector.findText(x_parameter)
        self.x_selector.blockSignals(True)
        self.x_selector.setCurrentIndex(new_index if new_index != -1 else 0)
        self.x_selector.blockSignals(False)
        return new_index

    def set_selected_y_parameter(self, y_parameter: str) -> int:
        """Sets the selected Y parameter."""
        new_index = self.y_selector.findText(y_parameter)
        self.y_selector.blockSignals(True)
        self.y_selector.setCurrentIndex(new_index if new_index != -1 else 0)
        self.y_selector.blockSignals(False)
        return new_index

    def _get_x_parameter_for_update(self, old_x_parameter: str, x_parameters: list, x_parameter_types: list) -> str:
        if self.y_selector.findText(old_x_parameter) != -1:
            return old_x_parameter
        elif TableColumnType.X.value in x_parameter_types:
            return x_parameters[x_parameter_types.index(1)]
        else:
            return ''

    def _get_y_parameter_for_update(self, old_y_parameter: str, y_parameters: list, y_parameter_types: list) -> str:
        if self.x_selector.findText(old_y_parameter) != -1:
            return old_y_parameter
        elif TableColumnType.Y.value in y_parameter_types:
            return y_parameters[y_parameter_types.index(2)]
        else:
            return ''

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
