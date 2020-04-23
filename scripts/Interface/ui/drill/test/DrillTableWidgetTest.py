# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import sys

from qtpy.QtWidgets import QApplication, QTableWidgetItem
from qtpy.QtTest import QTest
from qtpy.QtCore import Qt, QPoint

from Interface.ui.drill.main.DrillTableWidget import DrillTableWidget

app = QApplication(sys.argv)

class DrillTableWidgetTest(unittest.TestCase):

    def selectCell(self, row, column, modifier):
        # find the middle of the cell
        y = self.table.rowViewportPosition(row) \
            + self.table.rowHeight(row) / 2
        x = self.table.columnViewportPosition(column) \
            + self.table.columnWidth(column) / 2

        QTest.mouseClick(self.table.viewport(),
                         Qt.LeftButton, modifier, QPoint(x, y))

    def selectRow(self, row, modifier):
        # find the middle of the row header
        vertical_header = self.table.verticalHeader()
        x = 0 + vertical_header.length() / 2
        y = vertical_header.sectionPosition(row) \
            + vertical_header.sectionSize(row) / 2

        QTest.mouseClick(vertical_header.viewport(),
                         Qt.LeftButton, modifier, QPoint(x, y))

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
        self.assertEqual(self.table.getSelectedCells(), [(0, 0),
                                                         (4, 0),
                                                         (1, 1),
                                                         (2, 2)])

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

if __name__ == "__main__":
    unittest.main()

