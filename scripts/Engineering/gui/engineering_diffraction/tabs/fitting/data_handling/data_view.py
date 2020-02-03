# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy import QtWidgets, QtCore

from mantidqt.utils.qt import load_ui

Ui_data, _ = load_ui(__file__, "data_widget.ui")


class FittingDataView(QtWidgets.QWidget, Ui_data):
    sig_enable_load_button = QtCore.Signal(bool)

    def __init__(self, parent=None):
        super(FittingDataView, self).__init__(parent)
        self.setupUi(self)

        self.finder_data.setLabelText("Focused Run Files")
        self.finder_data.isForRunFiles(False)
        self.finder_data.allowMultipleFiles(True)

    # =================
    # Slot Connectors
    # =================

    def set_on_load_clicked(self, slot):
        self.button_load.clicked.connect(slot)

    def set_enable_button_connection(self, slot):
        self.sig_enable_load_button.connect(slot)

    def set_on_remove_all_clicked(self, slot):
        self.button_removeAll.clicked.connect(slot)

    def set_on_remove_selected_clicked(self, slot):
        self.button_removeSelected.clicked.connect(slot)

    def set_on_table_cell_changed(self, slot):
        self.table_selection.cellChanged.connect(slot)  # Row, Col

    # =================
    # Component Setters
    # =================

    def set_load_button_enabled(self, enabled):
        self.button_load.setEnabled(enabled)

    def add_table_row(self, run_no, bank):
        row_no = self.table_selection.rowCount()
        self.table_selection.insertRow(row_no)
        self.table_selection.setItem(row_no, 0, QtWidgets.QTableWidgetItem(str(run_no)))
        self.table_selection.setItem(row_no, 1, QtWidgets.QTableWidgetItem(str(bank)))
        check_box = QtWidgets.QTableWidgetItem()
        check_box.setCheckState(QtCore.Qt.Unchecked)
        self.table_selection.setItem(row_no, 2, check_box)

    def remove_table_row(self, row_no):
        self.table_selection.removeRow(row_no)

    def remove_all(self):
        self.table_selection.setRowCount(0)

    def remove_selected(self):
        for row in reversed(sorted(self.get_selected_rows())):
            self.remove_table_row(row)

    # =================
    # Component Getters
    # =================

    def get_filenames_to_load(self):
        return self.finder_data.getText()

    def get_filenames_valid(self):
        return self.finder_data.isValid()

    def get_add_to_plot(self):
        return self.check_addToPlot.isChecked()

    def get_selected_rows(self):
        return list(set(index.row() for index in self.table_selection.selectedIndexes()))

    # =================
    # State Getters
    # =================

    def is_searching(self):
        return self.finder_data.isSearching()
