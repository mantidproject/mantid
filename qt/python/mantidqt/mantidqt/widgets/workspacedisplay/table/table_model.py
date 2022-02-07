# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package.

from qtpy.QtCore import Qt, QAbstractTableModel, QModelIndex

from mantid.kernel import V3D
from numpy import ndarray

BATCH_SIZE = 3000
MINIMUM_BATCH_SIZE_ROWS = 100


class TableModel(QAbstractTableModel):
    """
    A QAbstractTableModel for use with a QTableView
    This implementation loads rows of the table in batches
    More batches are loaded when the user scrolls down in the table
    """
    ITEM_CHANGED_INVALID_DATA_MESSAGE = "Error: Trying to set invalid data for the column."
    ITEM_CHANGED_UNKNOWN_ERROR_MESSAGE = "Unknown error occurred: {}"

    def __init__(self, data_model, parent=None):
        super().__init__(parent=parent)
        self._data_model = data_model
        self._row_count = 0
        self._headers = []
        self._row_batch_size = MINIMUM_BATCH_SIZE_ROWS

    def setHorizontalHeaderLabels(self, labels):
        self._headers = labels

    def canFetchMore(self, index):
        if index.isValid():
            return False
        return self._row_count < self._data_model.get_number_of_rows()

    def fetchMore(self, index):
        if index.isValid():
            return

        remainder = self._data_model.get_number_of_rows() - self._row_count
        items_to_fetch = min(self._row_batch_size, remainder)

        if items_to_fetch < 0:
            return

        self.beginInsertRows(QModelIndex(), self._row_count, self._row_count + items_to_fetch - 1)
        self._row_count += items_to_fetch
        self.endInsertRows()

    def rowCount(self, parent=QModelIndex()):
        if parent.isValid():
            return 0
        else:
            return self._row_count

    def columnCount(self, parent=QModelIndex()):
        if parent.isValid():
            return 0
        return self._data_model.get_number_of_columns()

    def headerData(self, section, orientation, role):
        if role in (Qt.DisplayRole, Qt.EditRole) and orientation == Qt.Horizontal:
            if section < len(self._headers):
                return self._headers[section]
        else:
            return super().headerData(section, orientation, role)

    def setHeaderData(self, section, orientation, value, role=Qt.EditRole):
        if role == Qt.EditRole and orientation == Qt.Horizontal:
            self._defaultHeaders.insert(section, value)
            self.headerDataChanged.emit(Qt.Horizontal, section, section + 1)
            return True
        else:
            return False

    def setData(self, index, value, role):
        if index.isValid() and role == Qt.EditRole:
            col = index.column()
            row = index.row()
            try:
                self._data_model.set_cell_data(row, col, value, self.is_v3d(index))
            except ValueError:
                print(self.ITEM_CHANGED_INVALID_DATA_MESSAGE)
                return False
            except Exception as x:
                print(self.ITEM_CHANGED_UNKNOWN_ERROR_MESSAGE.format(x))
                return False
            self.dataChanged.emit(index, index)
            return True
        else:
            return False

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None
        if index.row() >= self.max_rows() or index.row() < 0:
            return None
        if role in (Qt.DisplayRole, Qt.EditRole):
            data = self._data_model.get_cell(index.row(), index.column())
            return str(data) if isinstance(data, (V3D, ndarray)) else data
        return None

    def load_data(self, data_model):
        self.beginResetModel()
        self._data_model = data_model
        self._headers = self._data_model.get_column_headers()
        self._row_count = 0
        self._update_row_batch_size()
        self.endResetModel()

    def _update_row_batch_size(self):
        num_data_columns = self._data_model.get_number_of_columns()
        if num_data_columns > 0:
            self._row_batch_size = max(int(BATCH_SIZE/num_data_columns),
                                       MINIMUM_BATCH_SIZE_ROWS)
        else:
            self._row_batch_size = MINIMUM_BATCH_SIZE_ROWS

    def flags(self, index):
        col = index.column()
        editable = self._data_model.is_editable_column(col)
        if editable:
            return super().flags(index) | Qt.ItemIsEditable | Qt.ItemIsSelectable
        else:
            return super().flags(index) | Qt.ItemIsSelectable

    def max_rows(self):
        return self._data_model.get_number_of_rows()

    def is_v3d(self, index):
        col = index.column()
        row = index.row()
        return isinstance(self._data_model.get_cell(row, col), V3D)
