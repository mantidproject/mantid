# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QTableWidget, QTableWidgetItem, QHeaderView, QStyle, QAbstractItemView, QMessageBox
from qtpy.QtGui import QBrush, QColor
from qtpy.QtCore import Qt

from .DrillHeaderView import DrillHeaderView
from .DrillItemDelegate import DrillItemDelegate
from .DrillTableItem import DrillTableItem


class DrillTableWidget(QTableWidget):
    """
    Widget based on QTableWidget, used in the DrILL interface. It mainly binds
    the custom header and delegate and expose some useful functions.
    """

    """
    List of column names.
    """
    _columns = None

    """
    True if the table has been disabled.
    """
    _disabled = False

    def __init__(self, parent=None):
        super(DrillTableWidget, self).__init__(parent)
        self._columns = list()
        header = DrillHeaderView(self)
        header.setSectionsClickable(True)
        header.setSectionResizeMode(QHeaderView.Interactive)
        self.setHorizontalHeader(header)

        delegate = DrillItemDelegate(self)
        self.setItemDelegate(delegate)

        # set the default row height to fit a text
        margin = self.style().pixelMetric(QStyle.PM_FocusFrameVMargin, None, self)
        minSize = self.fontMetrics().height() + 2 * margin
        self.verticalHeader().setDefaultSectionSize(minSize)

        self.horizontalHeader().setHighlightSections(False)
        self.verticalHeader().setHighlightSections(False)

        self.setItemPrototype(DrillTableItem())

        self._samplePresenters = list()

        self.itemChanged.connect(self.onItemChanged)

    def setWindowModified(self, state):
        """
        Propagate the windowModified flag to the parents.

        Args:
            state (bool): if True, the window is modified
        """
        widget = self
        while widget.parent():
            widget = widget.parent()
            widget.setWindowModified(state)

    def addSamplePresenter(self, presenter, index):
        """
        Add a sample presenter.

        Args:
            presenter (DrillSamplePresenter): reference to the presenter
            index (int): sample index
        """
        self._samplePresenters.insert(index, presenter)

    def setHorizontalHeaderLabels(self, labels):
        """
        Overrides QTableWidget::setHorizontalHeaderLabels. This methods calls
        the base class method and keeps a list of columns label for later use.

        Args:
            labels (list(str)): columns labels
        """
        super(DrillTableWidget, self).setHorizontalHeaderLabels(labels)
        self._columns = list()
        for label in labels:
            self._columns.append(label)

    def addRow(self, position):
        """
        Add a row in the table at a given valid postion.

        Args:
            position (int): row index
        """
        if self._disabled:
            return
        n_rows = self.rowCount()
        if (position < 0) or (position > n_rows):
            return
        self.insertRow(position)

    def deleteRow(self, position):
        """
        Delete a row at a given position (if this row exists).

        Args:
            position(int): row index
        """
        if self._disabled:
            return
        n_rows = self.rowCount()
        if (position < 0) or (position >= n_rows):
            return
        self.removeRow(position)
        del self._samplePresenters[position]

    def onItemChanged(self, item):
        """
        Triggered when a item was changed in the table. This function create
        a new MVP for any new item and forward the modification signals to the
        item itself.

        Args:
            item (DrillTableItem): the item
        """
        if item.getPresenter() is None:
            row = item.row()
            col = item.column()
            self._samplePresenters[row].onNewItem(self._columns[col], item)
            item.signals.dataChanged.emit()

    def itemFromName(self, row, name):
        """
        Get table item from row number and column name.

        Args:
            row (int): row index
            name (str): name of the column

        Returns:
            QTableWidgetItem: the table item, None if the colum/row does not
                              exist
        """
        if name not in self._columns or row >= self.rowCount():
            return None
        col = self._columns.index(name)
        item = self.item(row, col)
        if item is None:
            item = self.itemPrototype().clone()
            self.blockSignals(True)
            self.setItem(row, col, item)
            self.blockSignals(False)
        return item

    def eraseRow(self, position):
        """
        Erase the contents of a whole row (if it exists).

        Args:
            position (int): row index
        """
        if self._disabled:
            return
        n_rows = self.rowCount()
        if (position < 0) or (position >= n_rows):
            return
        for column in range(self.columnCount()):
            item = self.item(position, column)
            if item:
                item.setData(Qt.EditRole, None)

    def eraseCell(self, row, column):
        """
        Erase the contents of a single cell.

        Args:
            row (int): row index
            column (int): column index
        """
        if self._disabled:
            return
        n_rows = self.rowCount()
        n_cols = self.columnCount()
        if (row < 0) or (row > n_rows) or (column < 0) or (column > n_cols):
            return
        item = self.item(row, column)
        if item:
            item.setData(Qt.EditRole, None)

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
        Get the coordinates of the selected cells. These coordinates are sorted
        by visual index to be able to copy(cut) paste in the same order.

        Returns:
            list(tuple(int, int)): the coordinates (row, column) of the
                selected cells
        """
        selected_indexes = self.selectionModel().selectedIndexes()
        header = self.horizontalHeader()
        cellsLi = []
        cellsVi = []
        for i in selected_indexes:
            if header.isSectionHidden(i.column()):
                continue
            cellsLi.append((i.row(), i.column()))
            cellsVi.append((self.visualRow(i.row()), self.visualColumn(i.column())))
        return sorted(cellsLi, key=lambda i: cellsVi[cellsLi.index(i)][1])

    def getSelectionShape(self):
        """
        Get the shape of the selection, the number of rows and the number of
        columns.

        Returns:
        tuple(int, int): selection shape (n_rows, n_col), (0, 0) if the
                         selection is empty or discontinuous
        """
        header = self.horizontalHeader()
        selection = self.getSelectedCells()
        if not selection:
            return (0, 0)
        for i in range(len(selection)):
            row = self.visualRow(selection[i][0])
            col = self.visualColumn(selection[i][1])
            for j in range(col):
                if header.isSectionHidden(header.logicalIndex(j)):
                    col -= 1
            selection[i] = (row, col)
        rmin = selection[0][0]
        rmax = rmin
        cmin = selection[0][1]
        cmax = cmin
        for item in selection:
            if item[0] > rmax:
                rmax = item[0]
            if item[1] > cmax:
                cmax = item[1]
        shape = (rmax - rmin + 1, cmax - cmin + 1)
        if shape[0] * shape[1] != len(selection):
            return (0, 0)
        else:
            return shape

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
        if self._disabled:
            return
        n_rows = self.rowCount()
        n_columns = self.columnCount()
        if (row < 0) or (row >= n_rows) or (column < 0) or (column >= n_columns):
            return
        cell = self.itemPrototype().clone()
        self.setItem(row, column, cell)
        cell.setData(Qt.EditRole, contents)

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
        if row >= self.rowCount():
            return
        for c in range(self.columnCount()):
            item = self.item(row, c)
            if not item:
                item = self.itemPrototype().clone()
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
            item.setData(Qt.BackgroundRole, None)
        self.blockSignals(False)

    def setColumnHeaderToolTips(self, tooltips):
        """
        Set the tooltip of each column header.

        Args:
            tooltips (list(str)): a list of tooltips
        """
        for c in range(self.columnCount()):
            item = self.horizontalHeaderItem(c)
            if item and c < len(tooltips):
                item.setToolTip(tooltips[c])

    def setRowLabel(self, row, label, bold=False, tooltip=None):
        """
        Set the label of a specific row.

        Args:
            row (int): row index
            label (str): row label
        """
        item = self.verticalHeaderItem(row)
        if item:
            item.setText(label)
        else:
            self.setVerticalHeaderItem(row, QTableWidgetItem(label))

        font = self.verticalHeaderItem(row).font()
        font.setBold(bold)
        self.verticalHeaderItem(row).setFont(font)
        if tooltip:
            self.verticalHeaderItem(row).setToolTip(tooltip)

    def delRowLabel(self, row):
        """
        Delete the row label.

        Args:
            ros (int): row index
        """
        self.setVerticalHeaderItem(row, None)
        self.verticalHeader().headerDataChanged(Qt.Vertical, row, row)

    def setFoldedColumns(self, columns):
        """
        Fold specific columns.

        Args:
            columns (list(str)): list of column labels
        """
        header = self.horizontalHeader()
        for i in range(len(self._columns)):
            name = self._columns[i]
            if name in columns:
                header.foldSection(i)

    def getFoldedColumns(self):
        """
        Get the list of folded columns.

        Returns:
            list(str): list of columns labels
        """
        header = self.horizontalHeader()
        folded = list()
        for i in range(len(self._columns)):
            if header.isSectionFolded(i):
                folded.append(self._columns[i])
        return folded

    def setHiddenColumns(self, columns):
        """
        Hide specific columns.

        Args:
            columns(list(str)): list of column labels
        """
        header = self.horizontalHeader()
        for i in range(len(self._columns)):
            name = self._columns[i]
            if name in columns:
                header.hideSection(i)

    def toggleColumnVisibility(self, column):
        """
        Change the visibility state of a column by giving its name. If the
        column is not empty it is emptied before being hidden. In that case, the
        user is asked for confirmation.

        Args:
            column (str): column name
        """
        if column not in self._columns:
            return
        header = self.horizontalHeader()
        i = self._columns.index(column)
        if header.isSectionHidden(i):
            header.showSection(i)
        else:
            empty = True
            for j in range(self.rowCount()):
                if self.getCellContents(j, i):
                    empty = False
            if not empty:
                q = QMessageBox.question(self, "Column is not empty", "Hiding the column will erase its content. Do you want to continue?")
                if q == QMessageBox.Yes:
                    for j in range(self.rowCount()):
                        self.setCellContents(j, i, "")
                    header.hideSection(i)
            else:
                header.hideSection(i)

    def getHiddenColumns(self):
        """
        Get the list of hidden columns.

        Returns:
            list(str): list of column labels
        """
        header = self.horizontalHeader()
        hidden = list()
        for i in range(len(self._columns)):
            if header.isSectionHidden(i):
                hidden.append(self._columns[i])
        return hidden

    def setColumnsOrder(self, columns):
        """
        Set the columns order by giving a list of labels.

        Args:
            columns (list(str)): list of labels whose table should follow the
                                 order
        """
        header = self.horizontalHeader()
        for c in columns:
            if c not in self._columns:
                continue
            indexFrom = header.visualIndex(self._columns.index(c))
            indexTo = columns.index(c)
            header.moveSection(indexFrom, indexTo)

    def getColumnsOrder(self):
        """
        Get the columns order as a list of labels.

        Returns:
            list(str): list of columns labels
        """
        columns = [""] * len(self._columns)
        header = self.horizontalHeader()
        for i in range(len(self._columns)):
            index = header.visualIndex(i)
            columns[index] = self._columns[i]
        return columns

    def setDisabled(self, state):
        """
        Override QTableWidget::setDisabled. This methods disables only the table
        cells and not the scrollbars or the headers.

        Args:
            state (bool): True to disable, False to enable
        """
        self._disabled = state
        self.blockSignals(True)
        if state:
            self.__editTriggers = self.editTriggers()
            self.setEditTriggers(QAbstractItemView.NoEditTriggers)
        else:
            self.setEditTriggers(self.__editTriggers)
        flags = Qt.ItemIsEditable
        for c in range(self.columnCount()):
            for r in range(self.rowCount()):
                item = self.item(r, c)
                if item:
                    if state:
                        item.setFlags(item.flags() & ~flags)
                    else:
                        item.setFlags(item.flags() | flags)
        self.blockSignals(False)
