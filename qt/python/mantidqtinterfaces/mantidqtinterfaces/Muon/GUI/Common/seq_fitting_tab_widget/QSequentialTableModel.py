# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import QAbstractTableModel, Qt, Signal, QModelIndex

default_table_columns = ["Workspace", "Run", "Group/Pairs", "Fit status", "Chi squared"]
WORKSPACE_COLUMN = 0
RUN_COLUMN = 1
GROUP_COLUMN = 2
FIT_STATUS_COLUMN = 3
FIT_QUALITY_COLUMN = 4
NUM_DEFAULT_COLUMNS = len(default_table_columns)
default_fit_status = "No fit"
default_chi_squared = 0.0


class QSequentialTableModel(QAbstractTableModel):
    """Implements a QAbstractTableModel which holds the data for the Sequential table view.
     See the Qt documentation for further details https://doc.qt.io/qt-5/qabstracttablemodel.html"""
    parameterChanged = Signal(object)

    def __init__(self):
        super(QSequentialTableModel, self).__init__()
        self._defaultData = []  # holds the default data (run, group, fit quality)
        self._parameterData = []  # holds the parameter data
        self._defaultHeaders = default_table_columns.copy()  # default table columns
        self._parameterHeaders = []  # header of parameters

    def rowCount(self, parent=None):
        return len(self._defaultData)

    def columnCount(self, parent=None):
        return len(self._defaultHeaders) + len(self._parameterHeaders)

    def data(self, index, role):
        if role in (Qt.DisplayRole, Qt.EditRole):
            col = index.column()
            row = index.row()
            if col < len(default_table_columns):
                return self._defaultData[row][col]
            else:
                return self._parameterData[row][col - len(default_table_columns)]
        return None

    def setData(self, index, value, role):
        if index.isValid() and role == Qt.EditRole:
            col = index.column()
            row = index.row()
            if col < len(default_table_columns):
                self._defaultData[row][col] = value
            else:
                parameter_index = col - len(default_table_columns)
                self._parameterData[row][parameter_index] = value
                self.parameterChanged.emit(index)

            self.dataChanged.emit(index, index)
            return True
        else:
            return False

    def headerData(self, section, orientation, role):
        if role in (Qt.DisplayRole, Qt.EditRole) and orientation == Qt.Horizontal:
            if section < len(default_table_columns):
                return self._defaultHeaders[section]
            else:
                return self._parameterHeaders[section - len(default_table_columns)]
        else:
            return super(QSequentialTableModel, self).headerData(section, orientation, role)

    def setHeaderData(self, section, orientation, value, role=Qt.EditRole):
        if role == Qt.EditRole and orientation == Qt.Horizontal:
            if section < len(default_table_columns):
                self._defaultHeaders.insert(section, value)
            else:
                self._parameterHeaders.insert(section - len(default_table_columns), value)
            self.headerDataChanged.emit(Qt.Horizontal, section, section + 1)
            return True
        else:
            return False

    def insertRows(self, position, rows, parent=None):
        self.beginInsertRows(parent or QModelIndex(), position, position + rows - 1)
        for i in range(rows):
            default_row = [''] * len(default_table_columns)
            default_parameter_entries = [0] * len(self._parameterHeaders)
            self._defaultData.insert(position + i, default_row)
            self._parameterData.insert(position + i, default_parameter_entries)
        self.endInsertRows()

    def flags(self, index):
        if index.column() >= len(default_table_columns):
            return super().flags(index) | Qt.ItemIsEditable | Qt.ItemIsSelectable
        else:  # shouldn't be able to edit the default column values
            return super().flags(index) | Qt.ItemIsSelectable

    def clear_fit_parameters(self):
        self.beginResetModel()
        self._parameterData = []
        self._parameterHeaders = []
        self.endResetModel()

    def clear_fit_workspaces(self):
        self.beginResetModel()
        self._parameterData = []
        self._parameterHeaders = []
        self._defaultData = []
        self.endResetModel()

    def reset_fit_quality(self):
        leftIndex = self.index(0, FIT_STATUS_COLUMN)  # first parameter changed index
        rightIndex = self.index(self.rowCount(), FIT_QUALITY_COLUMN)  # last parameter changed index
        for row in range(self.rowCount()):
            self._defaultData[row][FIT_STATUS_COLUMN] = default_fit_status
            self._defaultData[row][FIT_QUALITY_COLUMN] = default_chi_squared
        self.dataChanged.emit(leftIndex, rightIndex)

    def set_fit_parameters_and_values(self, parameters, parameter_values):
        self._parameterData = []
        self._parameterHeaders = []
        for i in range(self.rowCount()):
            self._parameterData.insert(i, parameter_values[i])

        for i, parameter in enumerate(parameters):
            self.setHeaderData(len(default_table_columns) + i, Qt.Horizontal, parameter)

    def set_fit_parameter_values_for_row(self, row, parameter_values):
        if len(parameter_values) != self.number_of_parameters:
            return
        leftIndex = self.index(row, NUM_DEFAULT_COLUMNS)  # first parameter changed index
        rightIndex = self.index(row, self.columnCount())  # last parameter changed index
        self._parameterData[row] = parameter_values
        self.dataChanged.emit(leftIndex, rightIndex)

    def set_fit_parameter_values_for_column(self, column, parameter_value):
        parameter_index = column - NUM_DEFAULT_COLUMNS
        for row in range(self.rowCount()):
            self._parameterData[row][parameter_index] = parameter_value

    def set_fit_workspaces(self, workspace_names, runs, group_and_pairs):
        self.clear_fit_workspaces()
        if not workspace_names or not runs or not group_and_pairs:
            return
        self.beginInsertRows(QModelIndex(), 0, len(runs) - 1)
        for i in range(len(runs)):
            self._defaultData.insert(i, [workspace_names[i], runs[i], group_and_pairs[i], default_fit_status,
                                         default_chi_squared])
        self.endInsertRows()

    def set_run_information(self, row, run):
        index = self.index(row, RUN_COLUMN)
        self.setData(index, run, Qt.EditRole)

    def set_group_information(self, row, group):
        index = self.index(row, GROUP_COLUMN)
        self.setData(index, group, Qt.EditRole)

    def set_fit_quality(self, row, quality, chi_squared=0.0):
        index = self.index(row, FIT_STATUS_COLUMN)
        self.setData(index, quality, Qt.EditRole)
        index = self.index(row, FIT_QUALITY_COLUMN)
        self.setData(index, chi_squared, Qt.EditRole)

    def get_workspace_name_information(self, row):
        index = self.index(row, WORKSPACE_COLUMN)
        return self.data(index, Qt.DisplayRole)

    def get_run_information(self, row):
        index = self.index(row, RUN_COLUMN)
        return self.data(index, Qt.DisplayRole)

    def get_group_information(self, row):
        index = self.index(row, GROUP_COLUMN)
        return self.data(index, Qt.DisplayRole)

    def get_fit_quality(self, row):
        index = self.index(row, FIT_STATUS_COLUMN)
        return self.data(index, Qt.DisplayRole)

    def get_fit_parameters(self, row):
        return self._parameterData[row]

    @property
    def number_of_parameters(self):
        return len(self._parameterHeaders)

    @property
    def number_of_fits(self):
        return self.rowCount()
