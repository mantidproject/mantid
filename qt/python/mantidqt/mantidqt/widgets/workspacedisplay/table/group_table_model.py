# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package.

from qtpy.QtCore import Qt

from mantidqt.widgets.workspacedisplay.table.table_model import TableModel

from mantid.kernel import V3D
from numpy import ndarray


class GroupTableModel(TableModel):
    def __init__(self, data_model, parent=None):
        super().__init__(data_model, parent=parent)

    def setData(self, index, value, role):
        if index.isValid() and role == Qt.EditRole:
            model = index.model()
            ws_index = int(model.data(model.index(index.row(), 0)))
            group_index = int(model.data(model.index(index.row(), 1)))
            row = (group_index, ws_index)

            try:
                self._data_model.set_cell_data(row, index.column(), value, False)
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
