# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from os import path

from mantidqt.utils.qt import load_ui
from Engineering.gui.engineering_diffraction.tabs.common import output_settings
from fnmatch import fnmatch
from os.path import splitext

Ui_data, _ = load_ui(__file__, "data_widget.ui")


class FileFilterProxyModel (QtCore.QSortFilterProxyModel):
    text_filter = None

    def __init__(self, parent=None):
        super(FileFilterProxyModel, self).__init__(parent)

    def filterAcceptsRow(self, source_row, source_parent):
        model = self.sourceModel()
        index0 = model.index(source_row, 0, source_parent)
        fname = model.fileName(index0)
        fname = splitext(fname)[0]

        if self.text_filter is None:
            return True
        else:
            return model.isDir(index0) or fnmatch(fname,self.text_filter)


class FittingDataView(QtWidgets.QWidget, Ui_data):
    sig_enable_load_button = QtCore.Signal(bool)
    sig_enable_inspect_bg_button = QtCore.Signal(bool)
    proxy_model = None

    def __init__(self, parent=None):
        super(FittingDataView, self).__init__(parent)
        self.setupUi(self)
        # file finder
        self.finder_data.readSettings(output_settings.INTERFACES_SETTINGS_GROUP + '/' + output_settings.ENGINEERING_PREFIX)
        self.finder_data.setUseNativeWidget(False)
        self.proxy_model = FileFilterProxyModel()
        self.finder_data.setProxyModel(self.proxy_model)
        self.finder_data.setLabelText("")
        self.finder_data.isForRunFiles(False)
        self.finder_data.allowMultipleFiles(True)
        self.finder_data.setFileExtensions([".nxs"])
        # xunit combo box
        self.setup_xunit_combobox()
        self.update_file_filter(self.combo_bank.currentText(), self.combo_xunit.currentText())

    def saveSettings(self):
        self.finder_data.saveSettings(output_settings.INTERFACES_SETTINGS_GROUP + '/' + output_settings.ENGINEERING_PREFIX)

    # =================
    # Slot Connectors
    # =================

    def set_on_load_clicked(self, slot):
        self.button_load.clicked.connect(slot)

    def set_on_bank_changed(self, slot):
        self.combo_bank.currentIndexChanged.connect(lambda: slot(self.combo_bank.currentText(), self.combo_xunit.currentText()))

    def set_on_xunit_changed(self, slot):
        self.combo_xunit.currentIndexChanged.connect(lambda: slot(self.combo_bank.currentText(), self.combo_xunit.currentText()))

    def set_enable_load_button_connection(self, slot):
        self.sig_enable_load_button.connect(slot)

    def set_enable_inspect_bg_button_connection(self, slot):
        self.sig_enable_inspect_bg_button.connect(slot)

    def set_on_remove_all_clicked(self, slot):
        self.button_removeAll.clicked.connect(slot)

    def set_on_remove_selected_clicked(self, slot):
        self.button_removeSelected.clicked.connect(slot)

    def set_on_plotBG_clicked(self, slot):
        self.button_plotBG.clicked.connect(slot)

    def set_on_seq_fit_clicked(self, slot):
        self.button_SeqFit.clicked.connect(slot)

    def set_on_serial_fit_clicked(self, slot):
        self.button_SerialFit.clicked.connect(slot)

    def set_on_table_cell_changed(self, slot):
        self.table_selection.cellChanged.connect(slot)  # Row, Col

    def set_table_selection_changed(self, slot):
        self.table_selection.itemSelectionChanged.connect(slot)

    # =================
    # Component Setters
    # =================

    def set_default_files(self, filepaths):
        if not filepaths:
            return
        self.finder_data.setUserInput(",".join(filepaths))
        directories = set()
        for filepath in filepaths:
            directory, discard = path.split(filepath)
            directories.add(directory)
        if len(directories) == 1:
            self.finder_data.setLastDirectory(directory)

    def set_load_button_enabled(self, enabled):
        self.button_load.setEnabled(enabled)

    def set_inspect_bg_button_enabled(self, enabled):
        self.button_plotBG.setEnabled(enabled)

    def set_fit_buttons_enabled(self, enabled):
        self.button_SeqFit.setEnabled(enabled)
        self.button_SerialFit.setEnabled(enabled)

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

    def set_item_checkstate(self, row, col, checked):
        if checked:
            self.get_table_item(row, col).setCheckState(QtCore.Qt.Checked)
        else:
            self.get_table_item(row, col).setCheckState(QtCore.Qt.Unchecked)

    def update_file_filter(self, bank, xunit):
        self.proxy_model.text_filter = "*"
        if bank == "1 (North)":
            self.proxy_model.text_filter += "bank_1"
        elif bank == "2 (South)":
            self.proxy_model.text_filter += "bank_2"
        if xunit != "All":
            self.proxy_model.text_filter += "_" + xunit

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

    def read_bg_params_from_table(self, row):
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

    # =================
    # Internal Setup
    # =================

    def setup_xunit_combobox(self):
        self.combo_xunit.setEditable(False)
        # make TOF default
        index = self.combo_xunit.findText("TOF")
        self.combo_xunit.setCurrentIndex(index)
