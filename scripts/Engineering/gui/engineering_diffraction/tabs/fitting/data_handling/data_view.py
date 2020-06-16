# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
        self.finder_data.setFileExtensions([".nxs"])

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

    def set_on_plotBG_clicked(self, slot):
        self.button_plotBG.clicked.connect(slot)

    def set_on_table_cell_changed(self, slot):
        self.table_selection.cellChanged.connect(slot)  # Row, Col

    # =================
    # Component Setters
    # =================

    def set_load_button_enabled(self, enabled):
        self.button_load.setEnabled(enabled)

    def add_table_row(self, run_no, bank, checked, bgsub, niter, xwindow, SG):
        row_no = self.table_selection.rowCount()
        self.table_selection.insertRow(row_no)

        name_item = QtWidgets.QTableWidgetItem(str(run_no))
        name_item.setFlags(name_item.flags() & ~QtCore.Qt.ItemIsEditable)
        self.table_selection.setItem(row_no, 0, name_item)

        bank_item = QtWidgets.QTableWidgetItem(str(bank))
        bank_item.setFlags(name_item.flags() & ~QtCore.Qt.ItemIsEditable)
        self.table_selection.setItem(row_no, 1, bank_item)

        check_box = QtWidgets.QTableWidgetItem()
        check_box.setFlags(check_box.flags() & ~QtCore.Qt.ItemIsEditable)
        self.table_selection.setItem(row_no, 2, check_box)
        if checked:
            check_box.setCheckState(QtCore.Qt.Checked)
        else:
            check_box.setCheckState(QtCore.Qt.Unchecked)

        bgsub_check_box = QtWidgets.QTableWidgetItem()
        bgsub_check_box.setFlags(bgsub_check_box.flags() & ~QtCore.Qt.ItemIsEditable)
        bgsub_check_box.setToolTip('Estimate the background using iterative low-pass (smoothing) filter algorithm')
        self.table_selection.setItem(row_no, 3, bgsub_check_box)
        if bgsub:
            bgsub_check_box.setCheckState(QtCore.Qt.Checked)
        else:
            bgsub_check_box.setCheckState(QtCore.Qt.Unchecked)

        niter_item = QtWidgets.QTableWidgetItem()
        niter_item.setData(QtCore.Qt.EditRole, int(niter))
        niter_item.setFlags(niter_item.flags() | QtCore.Qt.ItemIsEditable)
        niter_item.setToolTip('The number of iterations in the background estimation')
        self.table_selection.setItem(row_no, 4, niter_item)

        xwindow_item = QtWidgets.QTableWidgetItem()
        xwindow_item.setData(QtCore.Qt.EditRole, float(xwindow))
        xwindow_item.setFlags(xwindow_item.flags() | QtCore.Qt.ItemIsEditable)
        xwindow_item.setToolTip('The width of the convolution window used for finding the background (in x-axis units)')
        self.table_selection.setItem(row_no, 5, xwindow_item)

        SG_check_box = QtWidgets.QTableWidgetItem()
        SG_check_box.setFlags(SG_check_box.flags() & ~QtCore.Qt.ItemIsEditable)
        SG_check_box.setToolTip(
            'Apply linear Savitzky–Golay filter before first iteration of background subtraction (recommended)')
        self.table_selection.setItem(row_no, 6, SG_check_box)
        if SG:
            SG_check_box.setCheckState(QtCore.Qt.Checked)
        else:
            SG_check_box.setCheckState(QtCore.Qt.Unchecked)

    def remove_table_row(self, row_no):
        self.table_selection.removeRow(row_no)

    def remove_all(self):
        self.table_selection.setRowCount(0)

    def remove_selected(self):
        selected = self.get_selected_rows()
        for row in reversed(sorted(selected)):
            self.remove_table_row(row)
        return selected

    def toggle_checkbox(self,row,col):
        checkbox = self.get_table_item(row, col)
        checkbox.setCheckState(QtCore.Qt.Unchecked)
        # get checkbox again as sometimes overwritten
        checkbox = self.get_table_item(row, col)
        checkbox.setCheckState(QtCore.Qt.Checked)

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
        return set(index.row() for index in self.table_selection.selectedIndexes())

    def get_table_item(self, row, col):
        return self.table_selection.item(row, col)

    def get_item_checked(self, row, col):
        return self.get_table_item(row, col).checkState() == QtCore.Qt.Checked

    def get_background_params(self, row):
        isBGsub = self.get_item_checked(row, 3)
        niter = int(self.get_table_item(row, 4).text())
        xwindow = float(self.get_table_item(row, 5).text())
        doSGfilter = self.get_item_checked(row, 6)
        return [isBGsub, niter, xwindow, doSGfilter]

    # =================
    # State Getters
    # =================

    def is_searching(self):
        return self.finder_data.isSearching()

    # # =================
    # # validators
    # # =================
    # class IntDelegate(QtWidgets.QItemDelegate):
    #     def createEditor(self, parent, option, index):
    #         w = QtWigdets.Q
    #         w.setInputMask("HH")
    #         return w
