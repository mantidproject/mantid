# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import QAbstractTableModel, Qt, Signal, QModelIndex

WORKSPACE_COLUMN = 0
RUN_COLUMN = 1
GROUP_COLUMN = 2


class QSelectionTableModel(QAbstractTableModel):
    """Implements a QAbstractTableModel which holds the data for the Sequential table view.
     See the Qt documentation for further details https://doc.qt.io/qt-5/qabstracttablemodel.html"""
    parameterChanged = Signal(object)

    def __init__(self):
        super(QSelectionTableModel, self).__init__()
        self._defaultData = []  # holds the default data (run, group, fit quality)
        self._defaultHeaders = ["Workspace", "Run", "Group/Pairs"]

    def rowCount(self, parent=None):
        return len(self._defaultData)

    def columnCount(self, parent=None):
        return len(self._defaultHeaders)

    def data(self, index, role):
        if role in (Qt.DisplayRole, Qt.EditRole):
            col = index.column()
            row = index.row()
            return self._defaultData[row][col]
        return None

    def setData(self, index, value, role):
        if index.isValid() and role == Qt.EditRole:
            col = index.column()
            row = index.row()
            self._defaultData[row][col] = value

            self.dataChanged.emit(index, index)
            return True
        else:
            return False

    def headerData(self, section, orientation, role):
        if role in (Qt.DisplayRole, Qt.EditRole) and orientation == Qt.Horizontal:
            return self._defaultHeaders[section]
        else:
            return super(QSelectionTableModel, self).headerData(section, orientation, role)

    def setHeaderData(self, section, orientation, value, role=Qt.EditRole):
        if role == Qt.EditRole and orientation == Qt.Horizontal:
            self._defaultHeaders.insert(section, value)
            self.headerDataChanged.emit(Qt.Horizontal, section, section + 1)
            return True
        else:
            return False

    def insertRows(self, position, rows, parent=None):
        self.beginInsertRows(parent or QModelIndex(), position, position + rows - 1)
        for i in range(rows):
            default_row = [''] * len(default_table_columns)
            self._defaultData.insert(position + i, default_row)
        self.endInsertRows()

    def flags(self, index):
        # shouldn't be able to edit the default column values
        return super().flags(index) | Qt.ItemIsSelectable

    def clear_workspaces(self):
        self.beginResetModel()
        self._defaultData = []
        self.endResetModel()

    def set_workspaces(self, workspace_names, runs, group_and_pairs):
        self.clear_workspaces()
        if not workspace_names or not runs or not group_and_pairs:
            return
        self.beginInsertRows(QModelIndex(), 0, len(runs) - 1)
        for i in range(len(runs)):
            self._defaultData.insert(i, [workspace_names[i], runs[i], group_and_pairs[i]])
        self.endInsertRows()

    def set_run_information(self, row, run):
        index = self.index(row, RUN_COLUMN)
        self.setData(index, run, Qt.EditRole)

    def set_group_information(self, row, group):
        index = self.index(row, GROUP_COLUMN)
        self.setData(index, group, Qt.EditRole)

    def get_workspace_name_information(self, row):
        index = self.index(row, WORKSPACE_COLUMN)
        return self.data(index, Qt.DisplayRole)

    def get_run_information(self, row):
        index = self.index(row, RUN_COLUMN)
        return self.data(index, Qt.DisplayRole)

    def get_group_information(self, row):
        index = self.index(row, GROUP_COLUMN)
        return self.data(index, Qt.DisplayRole)

    def get_names_and_rows(self):
        names_and_rows = {data[0]:row for row, data in enumerate(self._defaultData)}
        return names_and_rows
