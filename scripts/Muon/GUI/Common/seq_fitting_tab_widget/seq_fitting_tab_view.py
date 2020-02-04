# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets, QtCore, QtGui
from mantidqt.utils.qt import load_ui

ui_seq_fitting_tab, _ = load_ui(__file__, "seq_fitting_tab.ui")
allowed_minimizers = ['Levenberg-Marquardt', 'BFGS', 'Conjugate gradient (Fletcher-Reeves imp.)',
                      'Conjugate gradient (Polak-Ribiere imp.)',
                      'Damped GaussNewton', 'Levenberg-MarquardtMD', 'Simplex',
                      'SteepestDescent', 'Trust Region']

default_columns = {"Run": 0, "Groups/Pairs": 1, "Start Time": 2, "End Time": 3, "Fit quality": 4}

default_time_range = ["0.11", "15"]


class SeqFittingTabView(QtWidgets.QWidget, ui_seq_fitting_tab):

    @staticmethod
    def create_default_start_and_end_widgets():
        start_widget = QtWidgets.QTableWidgetItem(default_time_range[0])
        end_widget = QtWidgets.QTableWidgetItem(default_time_range[1])

        return start_widget, end_widget

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

    def setup_default_fit_results_table(self):
        self.fit_results_table.blockSignals(True)
        self.fit_results_table.clear()
        self.fit_results_table.setColumnCount(len(default_columns))
        self.fit_results_table.setHorizontalHeaderLabels(list(default_columns.keys()))
        self.fit_results_table.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        self.fit_results_table.blockSignals(False)

        # row manipulation options
        self.fit_results_table.verticalHeader().setSectionsMovable(True)
        self.fit_results_table.verticalHeader().setDragEnabled(True)
        self.fit_results_table.verticalHeader().setDragDropMode(QtWidgets.QAbstractItemView.InternalMove)

        self.fit_results_table.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectItems)
        self.fit_results_table.setSelectionMode(QtWidgets.QAbstractItemView.SingleSelection)

    def setup_fit_results_table_parameters(self, parameter_names):
        fit_table_columns = list(default_columns.keys()) + parameter_names
        self.fit_results_table.blockSignals(True)
        self.fit_results_table.clear()
        self.fit_results_table.setColumnCount(len(fit_table_columns))
        self.fit_results_table.setHorizontalHeaderLabels(fit_table_columns)
        self.fit_results_table.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        self.fit_results_table.blockSignals(False)

    def set_fit_table_workspaces(self, runs, group_and_pairs):
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

            start_x_widget, end_x_widget = self.create_default_start_and_end_widgets()

            self.fit_results_table.setItem(i, default_columns["Start Time"], start_x_widget)
            self.fit_results_table.setItem(i, default_columns["End Time"], end_x_widget)

            fitItem = QtWidgets.QTableWidgetItem("N/A")
            fitItem.setFlags(QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable)
            self.fit_results_table.setItem(i, default_columns["Fit quality"], fitItem)

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
        for i in range(self.number_of_entries()):
            for j, parameter in enumerate(parameter_values):
                parameterItem = QtWidgets.QTableWidgetItem(str(parameter))
                self.fit_results_table.setItem(i, len(default_columns) + j, parameterItem)

    def update_fit_function_parameters(self, row, parameter_values):
        for j, parameter in enumerate(parameter_values):
            parameter_item = self.fit_results_table.item(row, len(default_columns) + j)
            parameter_item.setText(str(parameter))

    def update_fit_quality(self, row, fit_status, fit_quality):
        fit_quality_item = self.fit_results_table.item(row, default_columns["Fit quality"])
        fit_quality_item.setText("{0:.3f}".format(fit_quality))
        if fit_status == 'success':
            fit_quality_item.setForeground(QtGui.QBrush(QtCore.Qt.green))
        elif fit_status is None:
            fit_quality_item.setForeground(QtGui.QBrush(QtCore.Qt.black))
        else:
            fit_quality_item.setForeground(QtGui.QBrush(QtCore.Qt.red))

    def get_workspace_entries(self):
        workspace_entries = []
        for row_index in range(self.fit_results_table.rowCount()):
            workspace_entries += [self.fit_results_table.item(row_index, 0).text()]

        return workspace_entries

    def get_workspace_info_from_fit_table_row(self, row_index):
        if row_index > self.fit_results_table.rowCount():
            return [], []

        run_numbers = self.fit_results_table.item(row_index, 0).text()
        group_and_pairs = self.fit_results_table.item(row_index, 1).text()
        return run_numbers, group_and_pairs

    def get_fit_x_range(self, row_index):
        xmin = float(self.fit_results_table.item(row_index, 2).text())
        xmax = float(self.fit_results_table.item(row_index, 3).text())

        return [xmin, xmax]

    def number_of_entries(self):
        return self.fit_results_table.rowCount()

    def setup_slot_for_fit_selected_button(self, slot):
        self.fit_selected_button.clicked.connect(slot)

    def setup_slot_for_sequential_fit_button(self, slot):
        self.seq_fit_button.clicked.connect(slot)

    @property
    def selected_row(self):
        return self.fit_results_table.selectionModel().currentIndex().row()
