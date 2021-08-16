# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QWidget

ui_form, base_widget = load_ui(__file__, "cyclic_data_selector.ui")


class CyclicDataSelectorView(ui_form, base_widget):
    """
    The CyclicDataSelectorView is a combobox used for selecting data with << and >> buttons which allow you to cycle
    through each data item.
    """

    def __init__(self, parent: QWidget = None):
        """Initialize the CyclicDataSelectorView."""
        super(CyclicDataSelectorView, self).__init__(parent)
        self.setupUi(self)

        self.increment_parameter_display_button.clicked.connect(self.increment_dataset_name_combo_box)
        self.decrement_parameter_display_button.clicked.connect(self.decrement_dataset_name_combo_box)

    def set_slot_for_dataset_changed(self, slot) -> None:
        """Connect the slot for the combo box being changed."""
        self.dataset_name_combo_box.currentIndexChanged.connect(slot)

    def setEnabled(self, state):
        self.dataset_name_combo_box.setEnabled(state)
        self.increment_parameter_display_button.setEnabled(state)
        self.decrement_parameter_display_button.setEnabled(state)

    @property
    def dataset_names(self) -> list:
        """Returns a list of dataset names currently in the combobox."""
        return [self.dataset_name_combo_box.itemText(i) for i in range(self.dataset_name_combo_box.count())]

    def add_dataset_name(self, dataset_name: str) -> None:
        """Adds a dataset to the combo box. Only emits currentIndexChanged if the new dataset is now selected."""
        self.dataset_name_combo_box.blockSignals(True)
        self.dataset_name_combo_box.addItem(dataset_name)
        self.update_dataset_names_combo_box_tooltips()
        self.dataset_name_combo_box.blockSignals(False)

        if self.dataset_name_combo_box.currentText() == dataset_name:
            self.dataset_name_combo_box.currentIndexChanged.emit(self.dataset_name_combo_box.currentIndex())

    def update_dataset_name_combo_box(self, dataset_names: list, emit_signal: bool = True) -> None:
        """Update the data in the combo box."""
        old_name = self.dataset_name_combo_box.currentText()

        self.update_dataset_names_combo_box(dataset_names)

        new_index = self.dataset_name_combo_box.findText(old_name)
        new_index = new_index if new_index != -1 else 0

        self.dataset_name_combo_box.blockSignals(True)
        self.dataset_name_combo_box.setCurrentIndex(new_index)
        self.dataset_name_combo_box.blockSignals(False)

        if emit_signal:
            # Emit manually in case the dataset index has not changed (but the loaded dataset may be different)
            self.dataset_name_combo_box.currentIndexChanged.emit(new_index)

    def update_dataset_names_combo_box(self, dataset_names: list) -> None:
        """Update the datasets displayed in the combobox."""
        self.dataset_name_combo_box.blockSignals(True)
        self.dataset_name_combo_box.clear()
        self.dataset_name_combo_box.addItems(dataset_names)
        self.update_dataset_names_combo_box_tooltips()
        self.dataset_name_combo_box.blockSignals(False)

    def update_dataset_names_combo_box_tooltips(self) -> None:
        """Update the tooltips for the combobox."""
        for i, dataset_name in enumerate(self.dataset_names):
            self.dataset_name_combo_box.setItemData(i, dataset_name, Qt.ToolTipRole)

    def increment_dataset_name_combo_box(self) -> None:
        """Increment the parameter display combo box."""
        index = self.dataset_name_combo_box.currentIndex()
        count = self.dataset_name_combo_box.count()

        if index < count - 1:
            self.dataset_name_combo_box.setCurrentIndex(index + 1)
        else:
            self.dataset_name_combo_box.setCurrentIndex(0)

    def decrement_dataset_name_combo_box(self) -> None:
        """Decrement the parameter display combo box."""
        index = self.dataset_name_combo_box.currentIndex()
        count = self.dataset_name_combo_box.count()

        if index != 0:
            self.dataset_name_combo_box.setCurrentIndex(index - 1)
        else:
            self.dataset_name_combo_box.setCurrentIndex(count - 1)

    @property
    def current_dataset_name(self) -> str:
        """Returns the selected dataset name."""
        return str(self.dataset_name_combo_box.currentText())

    @current_dataset_name.setter
    def current_dataset_name(self, dataset_name: str) -> None:
        """Sets the currently selected dataset name."""
        index = self.dataset_name_combo_box.findText(dataset_name)
        if index != -1:
            self.dataset_name_combo_box.blockSignals(True)
            self.dataset_name_combo_box.setCurrentIndex(index)
            self.dataset_name_combo_box.blockSignals(False)

            # Emit it manually in case the index hasn't changed, but the item has.
            self.dataset_name_combo_box.currentIndexChanged.emit(index)

    def number_of_datasets(self) -> int:
        """Returns the number of dataset names loaded into the widget."""
        return self.dataset_name_combo_box.count()

    @property
    def current_dataset_index(self) -> int:
        """Returns the index of the currently displayed dataset."""
        current_index = self.dataset_name_combo_box.currentIndex()
        return current_index if current_index != -1 else None

    def set_data_combo_box_label(self, text: str) -> None:
        """Sets the label text next to the data selector combobox."""
        self.data_combo_box_label.setText(text)

    def set_data_combo_box_label_width(self, width: int) -> None:
        """Sets the width of the label next to the data selector combobox."""
        self.data_combo_box_label.setMinimumWidth(width)
        self.data_combo_box_label.setMaximumWidth(width)
