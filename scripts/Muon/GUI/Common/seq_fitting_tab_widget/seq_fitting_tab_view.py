# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets, QtCore, QtGui
from mantidqt.utils.qt import load_ui
from Muon.GUI.Common.message_box import warning


ui_seq_fitting_tab, _ = load_ui(__file__, "seq_fitting_tab.ui")

default_columns = {"Run": 0, "Groups/Pairs": 1, "Fit quality": 2}
default_fit_status = "No fit"


class SeqFittingTabView(QtWidgets.QWidget, ui_seq_fitting_tab):

    @staticmethod
    def scale_fit_table_columns(header, number_of_columns):
        for i in range(number_of_columns):
            if i == 0:
                header.setSectionResizeMode(i, QtWidgets.QHeaderView.Stretch)
            else:
                header.setSectionResizeMode(i, QtWidgets.QHeaderView.ResizeToContents)

    def __init__(self, parent=None):
        super(SeqFittingTabView, self).__init__(parent)
        self.setupUi(self)
        self.fit_function = None

        self.setup_default_fit_results_table()

    def warning_popup(self, message):
        warning(message, parent=self)

    def setup_default_fit_results_table(self):
        self.fit_results_table.blockSignals(True)
        self.fit_results_table.clear()
        self.fit_results_table.setColumnCount(len(default_columns))
        self.fit_results_table.setHorizontalHeaderLabels(list(default_columns.keys()))
        self.fit_results_table.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        self.fit_results_table.blockSignals(False)

        self.fit_results_table.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.fit_results_table.setSelectionMode(QtWidgets.QAbstractItemView.SingleSelection)

    def set_fit_table_workspaces(self, runs, group_and_pairs):
        self.fit_results_table.blockSignals(True)
        self.fit_results_table.clearContents()
        self.fit_results_table.setRowCount(0)

        for i in range(len(runs)):
            runItem = QtWidgets.QTableWidgetItem(runs[i])
            runItem.setFlags(QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable)
            groupItem = QtWidgets.QTableWidgetItem(group_and_pairs[i])
            groupItem.setFlags(QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable)

            self.fit_results_table.insertRow(i)
            self.fit_results_table.setItem(i, default_columns["Run"], runItem)
            self.fit_results_table.setItem(i, default_columns["Groups/Pairs"], groupItem)

            fitItem = QtWidgets.QTableWidgetItem(default_fit_status)
            fitItem.setFlags(QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable)
            self.fit_results_table.setItem(i, default_columns["Fit quality"], fitItem)
        self.fit_results_table.blockSignals(False)

    def set_fit_table_function_parameters(self, fit_function_parameters, parameter_values):
        self.fit_results_table.blockSignals(True)
        # create columns
        self.fit_results_table.setColumnCount(len(fit_function_parameters) + len(list(default_columns.keys())))
        self.fit_results_table.setHorizontalHeaderLabels(list(default_columns.keys()) + fit_function_parameters)
        # rescale columns
        self.scale_fit_table_columns(self.fit_results_table.horizontalHeader(), self.fit_results_table.columnCount())
        # set table fit parameters
        self.initialise_fit_function_parameters(parameter_values)
        self.fit_results_table.blockSignals(False)

    def initialise_fit_function_parameters(self, parameter_values):
        self.fit_results_table.blockSignals(True)
        for i in range(self.get_number_of_entries()):
            for j, parameter in enumerate(parameter_values[i]):
                parameterItem = QtWidgets.QTableWidgetItem(str(parameter))
                self.fit_results_table.setItem(i, len(default_columns) + j, parameterItem)
                self.fit_results_table.item(i, default_columns["Fit quality"]).setText("No fit")
                self.fit_results_table.item(i, default_columns["Fit quality"]).setForeground(QtGui.QBrush(
                                                                                             QtCore.Qt.black))
        self.fit_results_table.blockSignals(False)

    def set_fit_function_parameters(self, row, parameter_values):
        self.fit_results_table.blockSignals(True)
        for j, parameter in enumerate(parameter_values):
            parameter_item = self.fit_results_table.item(row, len(default_columns) + j)
            parameter_item.setText("{0:.5f}".format(parameter))
        self.fit_results_table.blockSignals(False)

    def set_fit_quality_to_default(self):
        self.fit_results_table.blockSignals(True)
        for i in range(self.get_number_of_entries()):
            fit_quality_item = self.fit_results_table.item(i, default_columns["Fit quality"])
            fit_quality_item.setForeground(QtGui.QBrush(QtCore.Qt.black))
            fit_quality_item.setText(default_fit_status)
        self.fit_results_table.blockSignals(False)

    def set_fit_quality(self, row, fit_status, fit_quality):
        self.fit_results_table.blockSignals(True)
        fit_quality_item = self.fit_results_table.item(row, default_columns["Fit quality"])
        fit_quality_item.setText("{0:.3f}".format(fit_quality))
        if fit_status == 'success':
            fit_quality_item.setForeground(QtGui.QBrush(QtCore.Qt.green))
        elif fit_status is None:
            fit_quality_item.setForeground(QtGui.QBrush(QtCore.Qt.black))
        else:
            fit_quality_item.setForeground(QtGui.QBrush(QtCore.Qt.red))
        self.fit_results_table.blockSignals(False)

    def get_workspace_info_from_fit_table_row(self, row_index):
        if row_index > self.fit_results_table.rowCount():
            return [], []

        run_numbers = self.fit_results_table.item(row_index, default_columns["Run"]).text()
        group_and_pairs = self.fit_results_table.item(row_index, default_columns["Groups/Pairs"]).text()
        return run_numbers, group_and_pairs

    def get_fit_quality_from_fit_table_row(self, row_index):
        if row_index > self.fit_results_table.rowCount():
            return None

        fit_quality = self.fit_results_table.item(row_index, default_columns["Fit quality"]).text()
        return fit_quality

    def get_entry_fit_parameter_values(self, row_index):
        if row_index > self.fit_results_table.rowCount():
            return []

        parameter_values = []
        for j in range(len(default_columns), self.fit_results_table.columnCount()):
            parameter_item = self.fit_results_table.item(row_index, j)
            parameter_values += [float(parameter_item.text())]
        return parameter_values

    def get_number_of_entries(self):
        return self.fit_results_table.rowCount()

    def get_selected_row(self):
        return self.fit_results_table.selectionModel().currentIndex().row()

    def is_plotting_checked(self):
        return self.plot_fit_results_checkbox.isChecked()

    def use_initial_values_for_fits(self):
        return self.initial_fit_values_radio.isChecked()

    def setup_slot_for_fit_selected_button(self, slot):
        self.fit_selected_button.clicked.connect(slot)

    def setup_slot_for_sequential_fit_button(self, slot):
        self.seq_fit_button.clicked.connect(slot)

    def setup_slot_for_table_parameter_changed(self,slot):
        self.fit_results_table.cellChanged.connect(slot)

