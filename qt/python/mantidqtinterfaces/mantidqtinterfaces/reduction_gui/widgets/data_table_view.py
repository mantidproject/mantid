# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
DataTable Widget for data runs.
"""

from qtpy import QtCore, QtWidgets
from qtpy.QtCore import Qt


class DataTableModel(QtCore.QAbstractTableModel):
    """
    DataTable Model for the DataTableView widget.
    """

    def __init__(self, parent, headers=()):
        QtCore.QAbstractTableModel.__init__(self, parent)
        self._tableData = []
        self.headers = headers

    @property
    def tableData(self):
        return self._tableData

    @tableData.setter
    def tableData(self, data):
        def checkAndConvertRow(row):
            assert len(row) == self.columnCount()
            return list(row)

        self._tableData = list(map(checkAndConvertRow, data))

    def _numRows(self):
        """
        :return: number of rows with data
        """
        return len(self.tableData)

    def _getRow(self, row):
        """
        :param row: int of the row to get
        :return: data of the row
        """
        return self.tableData[row] if row < self._numRows() else self._createEmptyRow()

    def _isRowEmpty(self, row):
        """
        checks if the row is empty
        :param row: int of the row to check
        :return: true if row is empty
        """
        return all((v is None or not str(v).strip()) for v in self._getRow(row))

    def _createEmptyRow(self):
        return [self._textToData(self._numRows(), i, "") for i in range(self.columnCount())]

    def _removeTrailingEmptyRows(self):
        """
        remove all rows at the end of the table that are empty
        """
        for row in reversed(range(self._numRows())):
            if self._isRowEmpty(row):
                del self.tableData[row]
            else:
                break

    def _removeEmptyRows(self):
        """
        remove all empty rows
        """
        for row in reversed(range(self._numRows())):
            if self._isRowEmpty(row):
                del self.tableData[row]

    def _ensureHasRows(self, numRows):
        """
        ensure the table has numRows
        :param numRows:  number of rows that should exist
        """
        while self._numRows() < numRows:
            self.tableData.append(self._createEmptyRow())

    def _dataToText(self, row, col, value):
        """
        converts the stored data to a displayable text.
        Override this function if you need data types other than str in your table.
        """
        return str(value)

    def _textToData(self, row, col, text):
        """
        converts a displayable text back to stored data.
        Override this function if you need data types other than str in your table.
        """
        return text  # just return the value, it is already str.

    def _setCellText(self, row, col, text):
        """
        set the text of a cell
        :param row: row of the cell
        :param col: column of the cell
        :param text: text for the cell
        """
        self._ensureHasRows(row + 1)
        self.tableData[row][col] = self._textToData(row, col, str(text).strip())

    def _getCellText(self, row, col):
        """
        get the text of a cell
        :param row: row of the cell
        :param col: column of the cell
        :return: text of the cell
        """
        rowData = self._getRow(row)
        return self._dataToText(row, col, rowData[col]).strip() if len(rowData) > col else None

    # reimplemented QAbstractTableModel methods

    selectCell = QtCore.Signal(QtCore.QModelIndex)

    def emptyCells(self, indexes):
        """
        empty the cells with the indexes
        :param indexes: indexes of the cells to be emptied
        """
        for index in indexes:
            row = index.row()
            col = index.column()

            self._setCellText(row, col, "")

        self._removeEmptyRows()
        self.beginResetModel()
        self.endResetModel()
        # indexes is never empty
        self.selectCell.emit(indexes[0])

    def rowCount(self, _=QtCore.QModelIndex()):
        """
        number of rows
        :return: returns the number of rows
        """
        # one additional row for new data
        return self._numRows() + 1

    def columnCount(self, _=QtCore.QModelIndex()):
        """
        number of columns
        :return: number of columns
        """
        return len(self.headers)

    def headerData(self, selection, orientation, role):
        """
        header of the selection
        :param selection: selected cells
        :param orientation: orientation of selection
        :param role: role of the selection
        :return: header of the selection
        """
        if Qt.Horizontal == orientation and Qt.DisplayRole == role:
            return self.headers[selection]
        return None

    def data(self, index, role):
        """
        data of the cell
        :param index: index of the cell
        :param role: role of the cell
        :return: data of the cell
        """
        if Qt.DisplayRole == role or Qt.EditRole == role:
            return self._getCellText(index.row(), index.column())
        return None

    def setData(self, index, text, _):
        """
        set text in the cell
        :param index: index of the cell
        :param text: text for the cell
        :return: true if data is set
        """
        row = index.row()
        col = index.column()

        self._setCellText(row, col, text)
        self._removeTrailingEmptyRows()

        self.beginResetModel()
        self.endResetModel()

        # move selection to the next column or row
        col = col + 1

        if col >= self.columnCount():
            row = row + 1
            col = 0

        row = min(row, self.rowCount() - 1)
        self.selectCell.emit(self.index(row, col))

        return True

    def flags(self, _):
        """
        flags for the table
        :return: flags
        """
        return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable


class DataTableView(QtWidgets.QTableView):
    """
    DataTable Widget for data runs.
    """

    def __init__(self, parent, headers, model_cls=None):
        """
        :param headers: tuple of strings of the column headers
        :param model: a DataTableModel if an external model should be used. if not specified a new DataTableModel is created
        :return: a brand new DataTableView
        """
        super(DataTableView, self).__init__(parent)
        if model_cls is None:
            model_cls = DataTableModel
        model = model_cls(self, headers)

        self.setModel(model)
        self.verticalHeader().setVisible(False)
        self.horizontalHeader().setStretchLastSection(True)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)

    def keyPressEvent(self, QKeyEvent):
        """
        reimplemented keyPressEvent for deleting cells and arrows in editing cells
        :param QKeyEvent:
        :return:
        """
        if self.state() == QtWidgets.QAbstractItemView.EditingState:
            index = self.currentIndex()
            if QKeyEvent.key() in [Qt.Key_Down, Qt.Key_Up]:
                self.setFocus()
                self.setCurrentIndex(self.model().index(index.row(), index.column()))
            else:
                QtWidgets.QTableView.keyPressEvent(self, QKeyEvent)
        if QKeyEvent.key() in [Qt.Key_Delete, Qt.Key_Backspace]:
            self.model().emptyCells(self.selectedIndexes())
        else:
            QtWidgets.QTableView.keyPressEvent(self, QKeyEvent)
