# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore, QtGui
from os import path

from mantidqt.utils.qt import load_ui
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from fnmatch import fnmatch
from os.path import splitext

Ui_data, _ = load_ui(__file__, "data_widget.ui")


class FileFilterProxyModel(QtCore.QSortFilterProxyModel):
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
            return model.isDir(index0) or fnmatch(fname, self.text_filter)

    def sort(self, column, order):
        self.sourceModel().sort(column, order)


class FittingDataView(QtWidgets.QWidget, Ui_data):
    sig_enable_load_button = QtCore.Signal(bool)
    sig_enable_inspect_bg_button = QtCore.Signal(bool)
    proxy_model = None

    def __init__(self, parent=None):
        super(FittingDataView, self).__init__(parent)
        self.setupUi(self)
        # file finder
        self.finder_data.readSettings(output_settings.INTERFACES_SETTINGS_GROUP + "/" + output_settings.ENGINEERING_PREFIX)
        self.finder_data.setUseNativeWidget(False)
        self.proxy_model = FileFilterProxyModel()
        self.finder_data.setProxyModel(self.proxy_model)
        self.finder_data.setLabelText("")
        self.finder_data.isForRunFiles(False)
        self.finder_data.allowMultipleFiles(True)
        self.finder_data.setFileExtensions([".nxs"])
        # xunit combo box
        self.setup_combo_boxes()
        self.update_file_filter(self.combo_region.currentText(), self.combo_xunit.currentText())

    def saveSettings(self):
        self.finder_data.saveSettings(output_settings.INTERFACES_SETTINGS_GROUP + "/" + output_settings.ENGINEERING_PREFIX)

    # =================
    # Slot Connectors
    # =================

    def set_on_load_clicked(self, slot):
        self.button_load.clicked.connect(slot)

    def set_on_region_changed(self, slot):
        self.combo_region.currentIndexChanged.connect(lambda: slot(self.combo_region.currentText(), self.combo_xunit.currentText()))

    def set_on_xunit_changed(self, slot):
        self.combo_xunit.currentIndexChanged.connect(lambda: slot(self.combo_region.currentText(), self.combo_xunit.currentText()))

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

    def set_on_table_cell_changed(self, slot):
        # this signal gets triggered from a separate thread sometimes (eg load). So to make the handler
        # more simple, always issue as a queued signal
        self.table_selection.cellChanged.connect(slot, QtCore.Qt.QueuedConnection)

    def set_table_selection_changed(self, slot):
        self.table_selection.itemSelectionChanged.connect(slot)

    # =================
    # Component Setters
    # =================

    def set_default_files(self, filepaths, directory):
        if not filepaths:
            return
        self.finder_data.setUserInput(",".join(filepaths))
        directories = set()
        for filepath in filepaths:
            directory, discard = path.split(filepath)
            directories.add(directory)
        if directory:
            self.finder_data.setLastDirectory(directory)

    def set_load_button_enabled(self, enabled):
        self.button_load.setEnabled(enabled)

    def set_inspect_bg_button_enabled(self, enabled):
        self.button_plotBG.setEnabled(enabled)

    def add_table_row(self, run_no, bank, plotted, bgsub, niter, xwindow, SG):
        row_no = self.table_selection.rowCount()
        self.table_selection.insertRow(row_no)

        name_item = QtWidgets.QTableWidgetItem(str(run_no))
        name_item.setFlags(name_item.flags() & ~QtCore.Qt.ItemIsEditable)
        self.table_selection.setItem(row_no, 0, name_item)

        bank_item = QtWidgets.QTableWidgetItem(str(bank))
        bank_item.setFlags(name_item.flags() & ~QtCore.Qt.ItemIsEditable)
        self.table_selection.setItem(row_no, 1, bank_item)

        plotted_check_box = QtWidgets.QTableWidgetItem()

        plotted_check_box.setFlags(plotted_check_box.flags() & ~QtCore.Qt.ItemIsEditable)
        if plotted:
            plotted_check_box.setCheckState(QtCore.Qt.Checked)
        else:
            plotted_check_box.setCheckState(QtCore.Qt.Unchecked)
        # setItem last so that cellChanged signal only fired once
        self.table_selection.setItem(row_no, 2, plotted_check_box)

        bgsub_check_box = QtWidgets.QTableWidgetItem()
        bgsub_check_box.setFlags(bgsub_check_box.flags() & ~QtCore.Qt.ItemIsEditable)
        bgsub_check_box.setToolTip("Estimate the background using iterative low-pass (smoothing) filter algorithm")
        if bgsub:
            bgsub_check_box.setCheckState(QtCore.Qt.Checked)
        else:
            bgsub_check_box.setCheckState(QtCore.Qt.Unchecked)
        self.table_selection.setItem(row_no, 3, bgsub_check_box)

        niter_item = QtWidgets.QTableWidgetItem()
        niter_item.setData(QtCore.Qt.EditRole, int(niter))
        niter_item.setFlags(niter_item.flags() | QtCore.Qt.ItemIsEditable)
        niter_item.setToolTip("The number of iterations in the background estimation")
        self.table_selection.setItem(row_no, 4, niter_item)

        xwindow_item = QtWidgets.QTableWidgetItem()
        xwindow_item.setData(QtCore.Qt.EditRole, float(xwindow))
        xwindow_item.setFlags(xwindow_item.flags() | QtCore.Qt.ItemIsEditable)
        xwindow_item.setToolTip("The width of the convolution window used for finding the background (in x-axis units)")
        self.table_selection.setItem(row_no, 5, xwindow_item)

        SG_check_box = QtWidgets.QTableWidgetItem()
        SG_check_box.setFlags(SG_check_box.flags() & ~QtCore.Qt.ItemIsEditable)
        SG_check_box.setToolTip("Apply linear Savitzky–Golay filter before first iteration of background subtraction (recommended)")
        if SG:
            SG_check_box.setCheckState(QtCore.Qt.Checked)
        else:
            SG_check_box.setCheckState(QtCore.Qt.Unchecked)
        self.table_selection.setItem(row_no, 6, SG_check_box)

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

    def set_table_column(self, row, col, value):
        self.get_table_item(row, col).setData(QtCore.Qt.EditRole, int(value))

    def update_file_filter(self, region, xunit):
        self.proxy_model.text_filter = "*"
        if region == "1 (North)":
            self.proxy_model.text_filter += "bank_1"
        elif region == "2 (South)":
            self.proxy_model.text_filter += "bank_2"
        elif region == "Cropped" or region == "Custom":
            self.proxy_model.text_filter += region
        elif region == "Texture":
            self.proxy_model.text_filter += "Texture*"
        elif region == "Both Banks":
            self.proxy_model.text_filter += "bank*"
        if xunit != "No Unit Filter":
            self.proxy_model.text_filter += "_" + xunit
        self.proxy_model.text_filter += "*"  # Allows browse for '(No Unit Filter)' with a specified region

        # Keep "No Region/Unit Filter" text grey and other text black
        for combo_box, current_text in ((self.combo_region, region), (self.combo_xunit, xunit)):
            if current_text[0:2] == "No":  # No Unit or Region Filter
                combo_box.setStyleSheet("color: grey")
                for index in range(1, combo_box.count()):
                    combo_box.setItemData(index, QtGui.QColor("black"), QtCore.Qt.ForegroundRole)
            else:
                combo_box.setStyleSheet("color: black")
                combo_box.setItemData(0, QtGui.QColor("grey"), QtCore.Qt.ForegroundRole)

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

    def setup_combo_boxes(self):
        # set "No Region/Unit Filter" text grey and other text black
        for combo_box, type_name in ((self.combo_region, "Region"), (self.combo_xunit, "Unit")):
            combo_box.setEditable(True)
            combo_box.lineEdit().setReadOnly(True)
            combo_box.lineEdit().setEnabled(False)
            no_filter_index = combo_box.findText("No " + type_name + " Filter")
            combo_box.setItemData(no_filter_index, QtGui.QColor("grey"), QtCore.Qt.ForegroundRole)

        # make TOF default for combo_xunit
        index = self.combo_xunit.findText("TOF")
        self.combo_xunit.setCurrentIndex(index)


def create_workspace_table(table_column_headers, table_loaded_data, row_count):
    n_col = len(table_column_headers)
    table_loaded_data.setColumnCount(n_col)
    table_loaded_data.setHorizontalHeaderLabels(table_column_headers)
    table_loaded_data.setRowCount(row_count)

    header = table_loaded_data.horizontalHeader()
    [header.setSectionResizeMode(ind, QtWidgets.QHeaderView.Stretch) for ind in range(n_col - 1)]
    header.setSectionResizeMode(n_col - 1, QtWidgets.QHeaderView.ResizeToContents)
