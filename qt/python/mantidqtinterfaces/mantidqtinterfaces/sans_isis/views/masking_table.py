# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""View for the masking table.

The view for the masking table displays all available masks for a SANS reduction. It also allows to display the moved
and masked SANS workspace.
"""

from abc import ABCMeta, abstractmethod
from qtpy import QtWidgets
from mantidqt.utils.qt import load_ui

Ui_MaskingTable, _ = load_ui(__file__, "masking_table.ui")


class MaskingTable(QtWidgets.QWidget, Ui_MaskingTable):
    class MaskingTableListener(metaclass=ABCMeta):
        """
        Defines the elements which a presenter can listen to for the masking table
        """

        @abstractmethod
        def on_row_changed(self):
            pass

        @abstractmethod
        def on_update_rows(self):
            pass

        @abstractmethod
        def on_display(self):
            pass

    def __init__(self, parent=None):
        super(MaskingTable, self).__init__(parent)
        self.setupUi(self)
        # Hook up signal and slots
        self.connect_signals()
        self._masking_tab_listeners = []
        self.masking_table.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)

    def add_listener(self, listener):
        if not isinstance(listener, MaskingTable.MaskingTableListener):
            raise ValueError("The listener ist not of type MaskingTableListener but rather {}".format(type(listener)))
        self._masking_tab_listeners.append(listener)

    def clear_listeners(self):
        self._masking_tab_listeners = []

    def _call_masking_tab_listeners(self, target):
        for listener in self._masking_tab_listeners:
            target(listener)

    def on_row_changed(self):
        self._call_masking_tab_listeners(lambda listener: listener.on_row_changed())

    def on_update_rows(self):
        self._call_masking_tab_listeners(lambda listener: listener.on_update_rows())

    def on_display(self):
        self._call_masking_tab_listeners(lambda listener: listener.on_display())

    def connect_signals(self):
        self.select_row_combo_box.currentIndexChanged.connect(self.on_row_changed)
        self.display_mask_push_button.clicked.connect(self.on_display)
        self.select_row_push_button.clicked.connect(self.on_update_rows)

    # ------------------------------------------------------------------------------------------------------------------
    # Actions
    # ------------------------------------------------------------------------------------------------------------------
    def get_current_row(self):
        value = self.select_row_combo_box.currentText()
        if not value:
            value = -1
        return int(value)

    def set_row(self, index):
        found_index = self.select_row_combo_box.findText(str(index))
        if found_index and found_index != -1:
            self.select_row_combo_box.setCurrentIndex(found_index)

    def update_rows(self, indices):
        self.select_row_combo_box.blockSignals(True)
        self.select_row_combo_box.clear()
        for index in indices:
            self.select_row_combo_box.addItem(str(index))
        self.select_row_combo_box.blockSignals(False)

    def set_table(self, table_entries):
        # Remove all rows
        for index in reversed(range(self.masking_table.rowCount())):
            self.masking_table.removeRow(index)

        # Set the number of rows
        self.masking_table.setRowCount(len(table_entries))

        # Populate the rows
        for row, table_entry in enumerate(table_entries):
            entry_type = QtWidgets.QTableWidgetItem(table_entry.first)
            entry_detector = QtWidgets.QTableWidgetItem(table_entry.second)
            entry_detail = QtWidgets.QTableWidgetItem(table_entry.third)

            self.masking_table.setItem(row, 0, entry_type)
            self.masking_table.setItem(row, 1, entry_detector)
            self.masking_table.setItem(row, 2, entry_detail)

    def set_display_mask_button_to_processing(self):
        self.display_mask_push_button.setText("Processing ...")
        self.display_mask_push_button.setEnabled(False)

    def set_display_mask_button_to_normal(self):
        self.display_mask_push_button.setText("Display Mask")
        self.display_mask_push_button.setEnabled(True)
