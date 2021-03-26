# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
import sys

from qtpy.QtWidgets import QApplication, QTableWidgetItem
from qtpy.QtTest import QTest
from qtpy.QtCore import Qt, QPoint

from Interface.ui.drill.view.DrillTableWidget import DrillTableWidget

app = QApplication(sys.argv)


class DrillTableWidgetTest(unittest.TestCase):
    def selectCell(self, row, column, modifier):
        # find the middle of the cell
        y = self.table.rowViewportPosition(row) \
            + self.table.rowHeight(row) / 2
        x = self.table.columnViewportPosition(column) \
            + self.table.columnWidth(column) / 2

        QTest.mouseClick(self.table.viewport(), Qt.LeftButton, modifier, QPoint(x, y))

    def selectRow(self, row, modifier):
        # find the middle of the row header
        vertical_header = self.table.verticalHeader()
        x = 0 + vertical_header.length() / 2
        y = vertical_header.sectionPosition(row) \
            + vertical_header.sectionSize(row) / 2

        QTest.mouseClick(vertical_header.viewport(), Qt.LeftButton, modifier, QPoint(x, y))

    def setUp(self):
        self.table = DrillTableWidget()
        self.table.setColumnCount(1)
        self.table.setRowCount(1)

    def test_addRow(self):
        self.table.addRow(0)
        self.assertEqual(self.table.rowCount(), 2)
        self.table.addRow(10)
        self.assertEqual(self.table.rowCount(), 2)
        self.table.addRow(-1)
        self.assertEqual(self.table.rowCount(), 2)

    def test_deleteRow(self):
        self.table.deleteRow(-1)
        self.table.deleteRow(10)
        self.assertEqual(self.table.rowCount(), 1)
        self.table.deleteRow(0)
        self.assertEqual(self.table.rowCount(), 0)

    def test_eraseRow(self):
        self.assertIsNone(self.table.item(0, 0))
        cell = QTableWidgetItem("test")
        self.table.setItem(0, 0, cell)
        self.assertIsNotNone(self.table.item(0, 0))
        self.table.eraseRow(0)
        self.assertIsNone(self.table.item(0, 0))

    def test_eraseCell(self):
        self.assertIsNone(self.table.item(0, 0))
        cell = QTableWidgetItem("test")
        self.table.setItem(0, 0, cell)
        self.assertIsNotNone(self.table.item(0, 0))
        self.table.eraseCell(0, 0)
        self.assertIsNone(self.table.item(0, 0))

    def test_getSelectedRows(self):
        self.assertEqual(self.table.getSelectedRows(), [])
        self.selectRow(0, Qt.NoModifier)
        self.assertEqual(self.table.getSelectedRows(), [0])
        self.table.setRowCount(5)
        self.selectRow(0, Qt.NoModifier)
        self.selectRow(2, Qt.ControlModifier)
        self.assertEqual(self.table.getSelectedRows(), [0, 2])
        self.selectRow(0, Qt.NoModifier)
        self.selectRow(2, Qt.ShiftModifier)
        self.assertEqual(self.table.getSelectedRows(), [0, 1, 2])

    def test_getLastSelectedRow(self):
        self.assertEqual(self.table.getLastSelectedRow(), -1)
        self.selectRow(0, Qt.NoModifier)
        self.assertEqual(self.table.getLastSelectedRow(), 0)
        self.table.setRowCount(5)
        self.selectRow(0, Qt.NoModifier)
        self.selectRow(2, Qt.ShiftModifier)
        self.assertEqual(self.table.getLastSelectedRow(), 2)

    def test_getAllRows(self):
        self.table.setRowCount(5)
        self.assertEqual(self.table.getAllRows(), [0, 1, 2, 3, 4])
        self.table.setRowCount(0)
        self.assertEqual(self.table.getAllRows(), [])

    def test_getLastRow(self):
        self.table.setRowCount(5)
        self.assertEqual(self.table.getLastRow(), 4)
        self.table.setRowCount(0)
        self.assertEqual(self.table.getLastRow(), -1)

    def test_getSelectedCells(self):
        self.assertEqual(self.table.getSelectedCells(), [])
        self.selectCell(0, 0, Qt.NoModifier)
        self.assertEqual(self.table.getSelectedCells(), [(0, 0)])
        self.table.setRowCount(5)
        self.table.setColumnCount(5)
        self.selectCell(0, 0, Qt.NoModifier)
        self.selectCell(4, 0, Qt.ControlModifier)
        self.selectCell(1, 1, Qt.ControlModifier)
        self.selectCell(2, 2, Qt.ControlModifier)
        self.assertEqual(self.table.getSelectedCells(), [(0, 0), (4, 0), (1, 1), (2, 2)])

    def test_getSelectionShape(self):
        self.table.getSelectedCells = mock.Mock()
        self.table.setRowCount(3)
        self.table.setColumnCount(1)
        self.table.getSelectedCells.return_value = [(0, 0)]
        self.assertEqual(self.table.getSelectionShape(), (1, 1))
        self.table.getSelectedCells.return_value = [(0, 0), (1, 0), (2, 0)]
        self.assertEqual(self.table.getSelectionShape(), (3, 1))
        self.table.getSelectedCells.return_value = [(0, 0), (10, 10)]
        self.assertEqual(self.table.getSelectionShape(), (0, 0))

    def test_getRowsFromSelectedCells(self):
        self.assertEqual(self.table.getSelectedCells(), [])
        self.table.setRowCount(5)
        self.table.setColumnCount(5)
        self.selectCell(0, 0, Qt.NoModifier)
        self.selectCell(0, 1, Qt.ControlModifier)
        self.assertEqual(self.table.getRowsFromSelectedCells(), [0])
        self.selectCell(0, 0, Qt.NoModifier)
        self.selectCell(1, 1, Qt.ControlModifier)
        self.selectCell(2, 2, Qt.ControlModifier)
        self.assertEqual(self.table.getRowsFromSelectedCells(), [0, 1, 2])

    def test_getCellContents(self):
        self.assertEqual(self.table.getCellContents(0, 0), "")
        self.assertEqual(self.table.getCellContents(-1, -1), "")
        cell = QTableWidgetItem("test")
        self.table.setItem(0, 0, cell)
        self.assertEqual(self.table.getCellContents(0, 0), "test")

    def test_setCellContents(self):
        self.assertIsNone(self.table.item(0, 0))
        self.table.setCellContents(0, 0, "test")
        self.assertIsNotNone(self.table.item(0, 0))
        self.assertEqual(self.table.item(0, 0).text(), "test")
        self.table.setCellContents(-1, -1, "test")

    def test_getRowContents(self):
        self.assertEqual(self.table.getRowContents(0), [""])
        self.assertEqual(self.table.getRowContents(-1), [""])
        cell = QTableWidgetItem("test")
        self.table.setItem(0, 0, cell)
        self.assertEqual(self.table.getRowContents(0), ["test"])
        self.table.setColumnCount(2)
        self.assertEqual(self.table.getRowContents(0), ["test", ""])

    def test_setRowContents(self):
        self.assertIsNone(self.table.item(0, 0))
        self.table.setRowContents(0, ["test"])
        self.assertIsNotNone(self.table.item(0, 0))
        self.assertEqual(self.table.item(0, 0).text(), "test")
        self.table.setRowContents(0, ["test2"])
        self.assertEqual(self.table.item(0, 0).text(), "test2")
        self.table.setColumnCount(2)
        self.table.setRowContents(0, ["test", "test2"])
        self.assertEqual(self.table.item(0, 0).text(), "test")
        self.assertEqual(self.table.item(0, 1).text(), "test2")
        self.table.setRowContents(0, ["test", "test2", "test3"])

    def test_setRowBackground(self):
        self.table.setColumnCount(2)
        self.table.setRowBackground(0, "#11223344")
        for c in range(self.table.columnCount()):
            item = self.table.item(0, c)
            a = item.background().color().alpha()
            r = item.background().color().red()
            g = item.background().color().green()
            b = item.background().color().blue()
            self.assertEqual(a, int("11", 16))
            self.assertEqual(r, int("22", 16))
            self.assertEqual(g, int("33", 16))
            self.assertEqual(b, int("44", 16))

    def test_removeRowBackground(self):
        self.table.setColumnCount(2)
        self.table.setRowBackground(0, "#11223344")
        self.table.removeRowBackground(0)
        for c in range(self.table.columnCount()):
            item = self.table.item(0, c)
            a = item.background().color().alpha()
            r = item.background().color().red()
            g = item.background().color().green()
            b = item.background().color().blue()
            self.assertEqual(a, 255)
            self.assertEqual(r, 0)
            self.assertEqual(g, 0)
            self.assertEqual(b, 0)

    def test_setCellBackground(self):
        self.table.setColumnCount(2)
        self.table.setCellBackground(0, 0, "#11223344")
        item = self.table.item(0, 0)
        a = item.background().color().alpha()
        r = item.background().color().red()
        g = item.background().color().green()
        b = item.background().color().blue()
        self.assertEqual(a, int("11", 16))
        self.assertEqual(r, int("22", 16))
        self.assertEqual(g, int("33", 16))
        self.assertEqual(b, int("44", 16))
        item = self.table.item(0, 1)
        self.assertIsNone(item)

    def test_removeCellBackground(self):
        self.table.setColumnCount(2)
        self.table.setRowBackground(0, "#11223344")
        self.table.removeCellBackground(0, 0)
        item = self.table.item(0, 0)
        a = item.background().color().alpha()
        r = item.background().color().red()
        g = item.background().color().green()
        b = item.background().color().blue()
        self.assertEqual(a, 255)
        self.assertEqual(r, 0)
        self.assertEqual(g, 0)
        self.assertEqual(b, 0)
        item = self.table.item(0, 1)
        a = item.background().color().alpha()
        r = item.background().color().red()
        g = item.background().color().green()
        b = item.background().color().blue()
        self.assertEqual(a, int("11", 16))
        self.assertEqual(r, int("22", 16))
        self.assertEqual(g, int("33", 16))
        self.assertEqual(b, int("44", 16))

    def test_setCellToolTip(self):
        self.table.setCellToolTip(0, 0, "test")
        self.assertEqual(self.table.item(0, 0).toolTip(), "test")

    def test_setColumnHeaderToolTips(self):
        self.table.setHorizontalHeaderLabels(["label"])
        self.table.setColumnHeaderToolTips(["tooltip"])
        self.assertEqual(self.table.horizontalHeaderItem(0).toolTip(), "tooltip")

    def test_setRowLabel(self):
        self.table.setRowLabel(0, "test", False)
        self.assertEqual(self.table.verticalHeaderItem(0).text(), "test")
        self.table.setRowLabel(0, "test", False, "tooltip")
        self.assertEqual(self.table.verticalHeaderItem(0).text(), "test")
        self.assertEqual(self.table.verticalHeaderItem(0).toolTip(), "tooltip")

    def test_getRowLabel(self):
        self.assertEqual(self.table.getRowLabel(0), "1")
        self.table.setRowLabel(0, "test")
        self.assertEqual(self.table.getRowLabel(0), "test")

    def test_delRowLabel(self):
        self.table.setRowLabel(0, "test")
        self.assertEqual(self.table.getRowLabel(0), "test")
        self.table.delRowLabel(0)
        self.assertEqual(self.table.getRowLabel(0), "1")

    def test_setFoldedColumns(self):
        self.table.horizontalHeader = mock.Mock()
        mHeader = self.table.horizontalHeader.return_value
        self.table.columns = ["test1", "test2", "test3"]
        self.table.setFoldedColumns(["test"])
        mHeader.foldSection.assert_not_called()
        self.table.setFoldedColumns(["test2"])
        mHeader.foldSection.assert_called_with(1)

    def test_getFoldedColumns(self):
        self.table.horizontalHeader = mock.Mock()
        mHeader = self.table.horizontalHeader.return_value
        mHeader.isSectionFolded.return_value = True
        self.table.columns = ["test1", "test2", "test3"]
        folded = self.table.getFoldedColumns()
        self.assertEqual(folded, ["test1", "test2", "test3"])

    def test_setHiddenColumns(self):
        self.table.horizontalHeader = mock.Mock()
        mHeader = self.table.horizontalHeader.return_value
        self.table.columns = ["test1", "test2", "test3"]
        self.table.setHiddenColumns(["test"])
        mHeader.hideSection.assert_not_called()
        self.table.setHiddenColumns(["test1"])
        mHeader.hideSection.assert_called_with(0)

    def test_getHiddenColumns(self):
        self.table.horizontalHeader = mock.Mock()
        mHeader = self.table.horizontalHeader.return_value
        mHeader.isSectionHidden.return_value = True
        self.table.columns = ["test1", "test2"]
        hidden = self.table.getHiddenColumns()
        self.assertEqual(hidden, ["test1", "test2"])

    def test_setColumnsOrder(self):
        self.table.columns = ["test1", "test2", "test3"]
        self.table.horizontalHeader = mock.Mock()
        mHeader = self.table.horizontalHeader.return_value
        mHeader.visualIndex.side_effect = [3, 1, 0]
        self.table.setColumnsOrder(["test3", "test2", "test1"])
        calls = [mock.call(3, 0), mock.call(1, 1), mock.call(0, 2)]
        mHeader.moveSection.assert_has_calls(calls)

    def test_getColumnsOrder(self):
        self.table.columns = ["test1", "test2", "test3"]
        self.table.horizontalHeader = mock.Mock()
        mHeader = self.table.horizontalHeader.return_value
        mHeader.visualIndex.side_effect = [2, 1, 0]
        self.assertEqual(self.table.getColumnsOrder(), ["test3", "test2", "test1"])


if __name__ == "__main__":
    unittest.main()
