# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
import sys

from qtpy.QtWidgets import QApplication

from mantidqtinterfaces.drill.view.DrillTableWidget import DrillTableWidget
from mantidqtinterfaces.drill.view.DrillHeaderView import DrillHeaderView
from mantidqtinterfaces.drill.view.DrillItemDelegate import DrillItemDelegate
from mantidqtinterfaces.drill.view.DrillTableItem import DrillTableItem


app = QApplication(sys.argv)


class DrillTableWidgetTest(unittest.TestCase):

    def setUp(self):
        self.table = DrillTableWidget()

    def test_init(self):
        self.assertEqual(self.table._columns, [])
        self.assertFalse(self.table._disabled)
        self.assertTrue(isinstance(self.table.horizontalHeader(),
                        DrillHeaderView))
        self.assertTrue(isinstance(self.table.itemDelegate(),
                        DrillItemDelegate))
        self.assertTrue(isinstance(self.table.itemPrototype(),
                        DrillTableItem))
        self.assertEqual(self.table._samplePresenters, [])

    def test_addSamplePresenter(self):
        p1 = mock.Mock()
        p2 = mock.Mock()
        self.assertEqual(self.table._samplePresenters, [])
        self.table.addSamplePresenter(p1, 0)
        self.assertEqual(self.table._samplePresenters, [p1])
        self.table.addSamplePresenter(p2, 0)
        self.assertEqual(self.table._samplePresenters, [p2, p1])

    def test_setHorizontalHeaderLabels(self):
        self.table.setHorizontalHeaderLabels(["test1", "test2"])
        self.assertEqual(self.table._columns, ["test1", "test2"])

    def test_addRow(self):
        self.table.addRow(0)
        self.assertEqual(self.table.rowCount(), 1)
        self.table.addRow(10)
        self.assertEqual(self.table.rowCount(), 1)
        self.table.addRow(-1)
        self.assertEqual(self.table.rowCount(), 1)

    def test_deleteRow(self):
        self.table.deleteRow(-1)
        self.table.deleteRow(10)
        self.assertEqual(self.table.rowCount(), 0)
        self.table.setRowCount(1)
        self.table._samplePresenters = [None]
        self.table.deleteRow(0)
        self.assertEqual(self.table.rowCount(), 0)

    def test_onItemChanged(self):
        item = mock.Mock()
        item.row.return_value = 0
        item.column.return_value = 0
        itemPresenter = mock.Mock()
        samplePresenter = mock.Mock()
        self.table._samplePresenters = [samplePresenter]
        self.table._columns = ["test"]
        item.getPresenter.return_value = itemPresenter
        self.table.onItemChanged(item)
        item.getPresenter.return_value = None
        self.table.onItemChanged(item)
        samplePresenter.onNewItem.assert_called_once()

    def test_itemFromName(self):
        self.table.rowCount = mock.Mock()
        self.table.rowCount.return_value = 5
        self.table._columns = ["col1", "col2", "col3"]
        self.table.item = mock.Mock()
        item = self.table.itemFromName(1, "col1")
        self.assertEqual(item, self.table.item.return_value)
        item = self.table.itemFromName(1, "col10")
        self.assertIsNone(item)
        item = self.table.itemFromName(10, "col1")
        self.assertIsNone(item)
        self.table.item.return_value = None
        item = self.table.itemFromName(1, "col1")
        self.assertIsNotNone(item)

    def test_eraseRow(self):
        self.table.item = mock.Mock()
        self.table.rowCount = mock.Mock()
        self.table.rowCount.return_value = 5
        self.table.columnCount = mock.Mock()
        self.table.columnCount.return_value = 1
        self.table.cellChanged = mock.Mock()
        self.table._disabled = True
        self.table.eraseRow(0)
        self.table.item.return_value.setData.assert_not_called()
        self.table._disabled = False
        self.table.eraseRow(0)
        self.table.item.return_value.setData.assert_called_once()

    def test_eraseCell(self):
        self.table.rowCount = mock.Mock()
        self.table.rowCount.return_value = 5
        self.table.columnCount = mock.Mock()
        self.table.columnCount.return_value = 5
        self.table.item = mock.Mock()
        self.table.cellChanged = mock.Mock()
        self.table.eraseCell(-1, -1)
        self.table.item.return_value.setData.assert_not_called()
        self.table.eraseCell(0, 0)
        self.table.item.return_value.setData.assert_called_once()

    def test_getSelectedRows(self):
        mSelectionModel = mock.Mock()
        mSelectedRows = mock.Mock()
        mR1 = mock.Mock()
        mR1.row.return_value = 1
        mR2 = mock.Mock()
        mR2.row.return_value = 2
        mR3 = mock.Mock()
        mR3.row.return_value = 3
        mSelectedRows.return_value = [mR1, mR2, mR3]
        mSelectionModel.return_value.selectedRows = mSelectedRows
        self.table.selectionModel = mSelectionModel
        rows = self.table.getSelectedRows()
        self.assertEqual(rows, [1, 2, 3])

    def test_getLastSelectedRow(self):
        self.table.getSelectedRows = mock.Mock()
        self.table.getSelectedRows.return_value = [1, 2, 3]
        row = self.table.getLastSelectedRow()
        self.assertEqual(row, 3)
        self.table.getSelectedRows.return_value = []
        row = self.table.getLastSelectedRow()
        self.assertEqual(row, -1)

    def test_getAllRows(self):
        self.table.rowCount = mock.Mock()
        self.table.rowCount.return_value = 3
        rows = self.table.getAllRows()
        self.assertEqual(rows, [0, 1, 2])
        self.table.rowCount.return_value = 0
        rows = self.table.getAllRows()
        self.assertEqual(rows, [])

    def test_getLastRow(self):
        self.table.getAllRows = mock.Mock()
        self.table.getAllRows.return_value = [0, 1, 2, 3]
        row = self.table.getLastRow()
        self.assertEqual(row, 3)
        self.table.getAllRows.return_value = []
        row = self.table.getLastRow()
        self.assertEqual(row, -1)

    def test_getSelectedCells(self):
        mSelectionModel = mock.Mock()
        mSelectedIndexes = mock.Mock()
        mI1 = mock.Mock()
        mI1.row.return_value = 0
        mI1.column.return_value = 1
        mI2 = mock.Mock()
        mI2.row.return_value = 5
        mI2.column.return_value = 2
        mI3 = mock.Mock()
        mI3.row.return_value = 8
        mI3.column.return_value = 7
        mSelectedIndexes.return_value = [mI1, mI2, mI3]
        mSelectionModel.return_value.selectedIndexes = mSelectedIndexes
        mHeader = mock.Mock()
        mHeader.return_value.isSectionHidden.return_value = False
        self.table.horizontalHeader = mHeader
        self.table.selectionModel = mSelectionModel
        cells = self.table.getSelectedCells()
        self.assertEqual(cells, [(0, 1), (5, 2), (8, 7)])
        mHeader.return_value.isSectionHidden.return_value = True
        cells = self.table.getSelectedCells()
        self.assertEqual(cells, [])

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
        mSelectionModel = mock.Mock()
        mSelectedIndexes = mock.Mock()
        mI1 = mock.Mock()
        mI1.row.return_value = 0
        mI1.column.return_value = 1
        mI2 = mock.Mock()
        mI2.row.return_value = 5
        mI2.column.return_value = 2
        mI3 = mock.Mock()
        mI3.row.return_value = 8
        mI3.column.return_value = 7
        mSelectedIndexes.return_value = [mI1, mI2, mI3]
        mSelectionModel.return_value.selectedIndexes = mSelectedIndexes
        self.table.selectionModel = mSelectionModel
        self.table.getAllRows = mock.Mock()
        self.table.getAllRows.return_value = [0, 1, 2, 3, 4, 5, 6, 7, 8]
        rows = self.table.getRowsFromSelectedCells()
        self.assertEqual(rows, [0, 5, 8])
        mI1.row.return_value = 5
        rows = self.table.getRowsFromSelectedCells()
        self.assertEqual(rows, [5, 8])

    def test_getCellContents(self):
        self.table.item = mock.Mock()
        self.table.item.return_value = None
        contents = self.table.getCellContents(0, 0)
        self.table.item.assert_called_once_with(0, 0)
        self.assertEqual(contents, "")
        self.table.item.return_value = mock.Mock()
        self.table.item.return_value.text.return_value = "test"
        contents = self.table.getCellContents(0, 0)
        self.assertEqual(contents, "test")

    def test_setCellContents(self):
        self.table.rowCount = mock.Mock()
        self.table.rowCount.return_value = 5
        self.table.columnCount = mock.Mock()
        self.table.columnCount.return_value = 5
        self.table.itemPrototype = mock.Mock()
        self.table.setItem = mock.Mock()
        self.table._disabled = True
        self.table.setCellContents(0, 0, "test")
        self.table.setItem.assert_not_called()
        self.table._disabled = False
        self.table.setCellContents(0, 0, "test")
        self.table.itemPrototype.assert_called
        self.table.itemPrototype.return_value.clone.return_value \
            .setData.assert_called_once_with(2, "test")
        self.table.setItem.assert_called_once()

    @mock.patch("mantidqtinterfaces.drill.view.DrillTableWidget.QBrush")
    @mock.patch("mantidqtinterfaces.drill.view.DrillTableWidget.QColor")
    def test_setRowBackground(self, mColor, mBrush):
        self.table.rowCount = mock.Mock()
        self.table.rowCount.return_value = 5
        self.table.columnCount = mock.Mock()
        self.table.columnCount.return_value = 5
        self.table.itemPrototype = mock.Mock()
        self.table.setItem = mock.Mock()
        self.table.item = mock.Mock()
        mItem = self.table.item.return_value
        self.table.setRowBackground(0, "color")
        mColor.assert_called_once_with("color")
        mBrush.assert_called_once_with(mColor.return_value)
        mItem.setBackground.assert_called()

    def test_removeRowBackground(self):
        self.table.rowCount = mock.Mock()
        self.table.rowCount.return_value = 5
        self.table.columnCount = mock.Mock()
        self.table.columnCount.return_value = 5
        self.table.item = mock.Mock()
        mItem = self.table.item.return_value
        self.table.removeRowBackground(0)
        mItem.setData.assert_called()

    def test_setColumnHeaderToolTips(self):
        self.table.columnCount = mock.Mock()
        self.table.columnCount.return_value = 3
        self.table.horizontalHeaderItem = mock.Mock()
        mItem = self.table.horizontalHeaderItem.return_value
        calls = [mock.call("t1"), mock.call("t2"), mock.call("t3")]
        self.table.setColumnHeaderToolTips(["t1", "t2", "t3"])
        mItem.setToolTip.assert_has_calls(calls)

    def test_setRowLabel(self):
        self.table.verticalHeaderItem = mock.Mock()
        mItem = self.table.verticalHeaderItem.return_value
        self.table.setRowLabel(0, "label", True, "tooltip")
        mItem.setText.assert_called_once_with("label")
        mItem.font.return_value.setBold.assert_called_once_with(True)
        mItem.setToolTip.assert_called_once_with("tooltip")

    def test_delRowLabel(self):
        self.table.setVerticalHeaderItem = mock.Mock()
        self.table.verticalHeader = mock.Mock()
        self.table.delRowLabel(0)
        self.table.setVerticalHeaderItem.assert_called_once_with(0, None)
        self.table.verticalHeader.return_value.headerDataChanged \
            .assert_called_once()

    def test_setFoldedColumns(self):
        self.table.horizontalHeader = mock.Mock()
        mHeader = self.table.horizontalHeader.return_value
        self.table._columns = ["test1", "test2", "test3"]
        self.table.setFoldedColumns(["test"])
        mHeader.foldSection.assert_not_called()
        self.table.setFoldedColumns(["test2"])
        mHeader.foldSection.assert_called_with(1)

    def test_getFoldedColumns(self):
        self.table.horizontalHeader = mock.Mock()
        mHeader = self.table.horizontalHeader.return_value
        mHeader.isSectionFolded.return_value = True
        self.table._columns = ["test1", "test2", "test3"]
        folded = self.table.getFoldedColumns()
        self.assertEqual(folded, ["test1", "test2", "test3"])

    def test_setHiddenColumns(self):
        self.table.horizontalHeader = mock.Mock()
        mHeader = self.table.horizontalHeader.return_value
        self.table._columns = ["test1", "test2", "test3"]
        self.table.setHiddenColumns(["test"])
        mHeader.hideSection.assert_not_called()
        self.table.setHiddenColumns(["test1"])
        mHeader.hideSection.assert_called_with(0)

    @mock.patch("mantidqtinterfaces.drill.view.DrillTableWidget.QMessageBox")
    def test_toggleColumnVisibility(self, mDialog):
        self.table._columns = ["test1", "test2", "test3"]
        self.table.horizontalHeader = mock.Mock()
        mHeader = self.table.horizontalHeader.return_value
        self.table.toggleColumnVisibility("test4")
        mHeader.isSectionHidden.assert_not_called()
        mHeader.showSection.assert_not_called()
        mHeader.hideSection.assert_not_called()
        mHeader.isSectionHidden.return_value = True
        self.table.toggleColumnVisibility("test1")
        mHeader.isSectionHidden.assert_called_once()
        mHeader.showSection.assert_called_once()
        mHeader.reset_mock()
        mHeader.isSectionHidden.return_value = False
        self.table.getCellContents = mock.Mock()
        self.table.getCellContents.return_value = ""
        self.table.rowCount = mock.Mock()
        self.table.rowCount.return_value = 1
        self.table.toggleColumnVisibility("test2")
        mHeader.isSectionHidden.assert_called_once()
        mDialog.question.assert_not_called()
        self.table.getCellContents.return_value = "test"
        self.table.toggleColumnVisibility("test3")
        mDialog.question.assert_called_once()

    def test_getHiddenColumns(self):
        self.table.horizontalHeader = mock.Mock()
        mHeader = self.table.horizontalHeader.return_value
        mHeader.isSectionHidden.return_value = True
        self.table._columns = ["test1", "test2"]
        hidden = self.table.getHiddenColumns()
        self.assertEqual(hidden, ["test1", "test2"])

    def test_setColumnsOrder(self):
        self.table._columns = ["test1", "test2", "test3"]
        self.table.horizontalHeader = mock.Mock()
        mHeader = self.table.horizontalHeader.return_value
        mHeader.visualIndex.side_effect = [3, 1, 0]
        self.table.setColumnsOrder(["test3", "test2", "test1"])
        calls = [mock.call(3, 0), mock.call(1, 1), mock.call(0, 2)]
        mHeader.moveSection.assert_has_calls(calls)

    def test_getColumnsOrder(self):
        self.table._columns = ["test1", "test2", "test3"]
        self.table.horizontalHeader = mock.Mock()
        mHeader = self.table.horizontalHeader.return_value
        mHeader.visualIndex.side_effect = [2, 1, 0]
        self.assertEqual(self.table.getColumnsOrder(),
                         ["test3", "test2", "test1"])


if __name__ == "__main__":
    unittest.main()
