# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QTableWidget, QTableWidgetItem, QHeaderView, QStyle
from qtpy.QtGui import QBrush, QColor
from qtpy.QtCore import *

from .DrillHeaderView import DrillHeaderView
from .DrillItemDelegate import DrillItemDelegate

class DrillTableWidget(QTableWidget):
    """
    Widget based on QTableWidget, used in the DrILL interface. It mainly binds
    the custom header and delegate and expose some useful functions.
    """

    def __init__(self, parent=None):
        super(DrillTableWidget, self).__init__(parent)
        header = DrillHeaderView(self)
        header.setSectionsClickable(True)
        header.setHighlightSections(True)
        header.setSectionResizeMode(QHeaderView.Interactive)
        self.setHorizontalHeader(header)

        delegate = DrillItemDelegate(self)
        self.setItemDelegate(delegate)

        # set the default row height to fit a text
        margin = self.style().pixelMetric(QStyle.PM_FocusFrameVMargin,
                                          None, self)
        minSize = self.fontMetrics().height() + 2 * margin
        self.verticalHeader().setDefaultSectionSize(minSize)

    def addRow(self, position):
        """
        Add a row in the table at a given valid postion.

        Args:
            position (int): row index
        """
        n_rows = self.rowCount()
        if ((position < 0) or (position > n_rows)):
            return
        self.insertRow(position)

    def deleteRow(self, position):
        """
        Delete a row at a given position (if this row exists).

        Args:
            position(int): row index
        """
        n_rows = self.rowCount()
        if ((position < 0) or (position >= n_rows)):
            return
        self.removeRow(position)

    def eraseRow(self, position):
        """
        Erase the contents of a whole row (if it exists).

        Args:
            position (int): row index
        """
        n_rows = self.rowCount()
        if ((position < 0) or (position >= n_rows)):
            return
        for column in range(self.columnCount()):
            self.takeItem(position, column)
            self.cellChanged.emit(position, column)

    def eraseCell(self, row, column):
        """
        Erase the contents of a single cell.

        Args:
            row (int): row index
            column (int): column index
        """
        n_rows = self.rowCount()
        n_cols = self.columnCount()
        if ((row < 0) or (row > n_rows) or (column < 0) or (column > n_cols)):
            return;
        self.takeItem(row, column)
        self.cellChanged.emit(row, column)

    def getSelectedRows(self):
        """
        Get the list of currently selected rows.

        Returns:
            list(int): list of selected rows indexes
        """
        selected_rows = self.selectionModel().selectedRows()
        rows = [row.row() for row in selected_rows]
        return rows

    def getLastSelectedRow(self):
        """
        Get the further down selected row.

        Returns:
            int: the row index, -1 if no row selected.
        """
        rows = self.getSelectedRows()
        if rows:
            return rows[-1]
        return -1

    def getAllRows(self):
        """
        Get the list of all rows indexes.

        Returns:
            list(int): list of rows indexes
        """
        return list(range(self.rowCount()))

    def getLastRow(self):
        """
        Get the further down row of the whole table.

        Returns:
            int: the row index, -1 if the table is empty
        """
        rows = self.getAllRows()
        if rows:
            return rows[-1]
        return -1

    def getSelectedCells(self):
        """
        Get the coordinates of the selected cells.

        Returns:
            list(tuple(int, int)): the coordinates (row, column) of the
                selected cells
        """
        selected_indexes = self.selectionModel().selectedIndexes()
        return [(i.row(), i.column()) for i in selected_indexes]

    def getRowsFromSelectedCells(self):
        """
        Get the row indexes of the selected cells.

        Return:
            list(int): list of unique rows indexes
        """
        selectedIndexes = self.selectionModel().selectedIndexes()
        selectedRows = [i.row() for i in selectedIndexes]
        allRows = self.getAllRows()
        return [r for r in allRows if r in selectedRows]

    def getCellContents(self, row, column):
        """
        Get the contents of a given cell as a string.

        Args:
            row (int): row index
            column (int): column index

        Returns:
            str: cell contents
        """
        cell = self.item(row, column)
        if cell:
            return cell.text()
        else:
            return ""

    def setCellContents(self, row, column, contents):
        """
        Set the content of an existing cell.

        Args:
            row (int): row index
            column (int): column index
            contents (str): cell contents
        """
        n_rows = self.rowCount()
        n_columns = self.columnCount()
        if ((row < 0) or (row >= n_rows) \
                or (column < 0) or (column >= n_columns)):
            return
        cell = QTableWidgetItem(contents)
        self.setItem(row, column, cell)

    def getRowContents(self, row):
        """
        Get the contents of a whole row.

        Args:
            row (int): row index

        Returns:
            list(str): the row contents
        """
        contents = list()
        for column in range(self.columnCount()):
            contents.append(self.getCellContents(row, column))
        return contents

    def setRowContents(self, row, contents):
        """
        Set the content of an existing row.

        Args:
            row (int): row index
            contents (list(str)): contents
        """
        n_rows = self.rowCount()
        if ((row < 0) or (row >= n_rows)):
            return
        column = 0
        for txt in contents:
            self.setCellContents(row, column, txt)
            column += 1

    def setRowBackground(self, row, color):
        """
        Set the background color of an existing row. If the row cells do not
        contain items, they will be created by this method.

        Args:
            row (int): the row index
            color (str): the RBG or ARGB string color
        """
        self.blockSignals(True)
        brush = QBrush(QColor(color))
        if (row >= self.rowCount()):
            return
        for c in range(self.columnCount()):
            item = self.item(row, c)
            if not item:
                item = QTableWidgetItem()
                self.setItem(row, c, item)

            self.item(row, c).setBackground(brush)
        self.blockSignals(False)

    def removeRowBackground(self, row):
        """
        Remove a previously set background color for an entire row.

        Args:
            row (int): row index
        """
        self.blockSignals(True)
        for c in range(self.columnCount()):
            item = self.item(row, c)
            if not item:
                continue
            self.setItem(row, c, QTableWidgetItem(item.text()))
        self.blockSignals(False)

    def setCellBackground(self, row, column, color):
        """
        Set the background color of an existing cell. If the cell does not
        contain item, it will be created by this method.

        Args:
            row (int): row index
            column (int): column index
            color (str): the RBG or ARGB color string
        """
        self.blockSignals(True)
        brush = QBrush(QColor(color))
        if (row >= self.rowCount()) or (column >= self.columnCount()):
            return
        item = self.item(row, column)
        if not item:
            self.setItem(row, column, QTableWidgetItem())

        self.item(row, column).setBackground(brush)
        self.blockSignals(False)

    def removeCellBackground(self, row, column):
        """
        Remove a previously set background color for a cell.

        Args:
            row (int): row index
            column (int): column index
        """
        self.blockSignals(True)
        item = self.item(row, column)
        if not item:
            return
        item = QTableWidgetItem(item.text())
        self.setItem(row, column, item)
        self.blockSignals(False)

    def setCellToolTip(self, row, column, contents):
        """
        Set a tooltip associated with a cell. If the cell does not contain
        item, it will be created by this method.

        Args:
            row (int): row index
            column (int): column index
            contents (str): tooltip contents
        """
        self.blockSignals(True)
        item = self.item(row, column)
        if not item:
            self.setItem(row, column, QTableWidgetItem())

        self.item(row, column).setToolTip(contents)
        self.blockSignals(False)

    def setHeaderFoldingState(self, columns):
        """
        Give a folding state for each column.

        Args:
            columns(list(bool)): column indexes
        """
        if (len(columns) != self.columnCount()):
            return
        for i in range(len(columns)):
            if columns[i]:
                self.horizontalHeader().foldSection(i)

    def getHeaderFoldingState(self):
        """
        Get the folding state of each column.

        Returns:
            list(bool): True if the column is folded
        """
        header = self.horizontalHeader()
        fold = list()
        for i in range(self.columnCount()):
            fold.append(header.isSectionFolded(i))
        return fold
