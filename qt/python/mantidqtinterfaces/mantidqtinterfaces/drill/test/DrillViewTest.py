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
from qtpy.QtGui import QCloseEvent
from qtpy.QtCore import Qt
from qtpy.QtTest import QTest

from mantidqtinterfaces.drill.view.DrillView import DrillView


app = QApplication(sys.argv)


class DrillViewTest(unittest.TestCase):
    def setUp(self):
        patch = mock.patch("mantidqtinterfaces.drill.view.DrillView.QMessageBox")
        self.mMsgBox = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch("mantidqtinterfaces.drill.view.DrillView.manageuserdirectories")
        self.mUserDir = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch("mantidqtinterfaces.drill.view.DrillView.DrillPresenter")
        self.mPresenter = patch.start()
        self.addCleanup(patch.stop)

        self.view = DrillView()
        self.view.table = mock.Mock()

    def test_closeEvent(self):
        self.view.findChildren = mock.Mock()
        c1 = mock.Mock()
        c2 = mock.Mock()
        c3 = mock.Mock()
        self.view.findChildren.return_value = [c1, c2]
        self.view.closeEvent(QCloseEvent())
        c1.close.assert_called_once()
        c2.close.assert_called_once()
        c3.close.assert_not_called()
        self.mPresenter.return_value.onClose.assert_called_once()

    def test_changeCycleOrExperiment(self):
        self.view.cycleAndExperimentChanged = mock.Mock()
        self.view.cycleNumber = mock.Mock()
        self.view.cycleNumber.text.return_value = "cycle1"
        self.view.experimentId = mock.Mock()
        self.view.experimentId.text.return_value = ""
        self.view._changeCycleOrExperiment()
        self.view.cycleAndExperimentChanged.assert_not_called()
        self.view.experimentId.text.return_value = "exp1"
        self.view._changeCycleOrExperiment()
        self.view.cycleAndExperimentChanged.emit.assert_called_once_with("cycle1", "exp1")

    def test_copySelectedCells(self):
        # no selection
        self.view.table.getSelectedCells.return_value = []
        self.view.copySelectedCells()
        self.assertEqual(self.view.buffer, [])
        self.assertEqual(self.view.bufferShape, ())
        self.view.table.getCellContents.assert_not_called()

        # valid selection
        self.view.table.getSelectedCells.return_value = [(0, 0)]
        self.view.table.getSelectionShape.return_value = (1, 1)
        self.view.table.getCellContents.return_value = "test"
        self.view.copySelectedCells()
        self.assertEqual(self.view.buffer, ["test"])
        self.assertEqual(self.view.bufferShape, (1, 1))
        self.view.table.getSelectedCells.return_value = [(0, 0), (0, 1), (1, 0), (1, 1)]
        self.view.table.getSelectionShape.return_value = (2, 2)
        self.view.table.getCellContents.return_value = "test"
        self.view.copySelectedCells()
        self.view.table.getCellContents.assert_called()
        self.assertEqual(self.view.buffer, ["test", "test", "test", "test"])
        self.assertEqual(self.view.bufferShape, (2, 2))

        # invalid selection
        self.view.table.reset_mock()
        self.view.table.getSelectedCells.return_value = [(0, 0), (10, 10)]
        self.view.table.getSelectionShape.return_value = (0, 0)
        self.view.copySelectedCells()
        self.view.table.getCellContents.assert_not_called()

        # empty values
        self.view.table.reset_mock()
        self.view.buffer = ["test"]
        self.view.bufferShape = (1, 1)
        self.view.table.getSelectedCells.return_value = [(0, 0), (0, 1), (1, 0), (1, 1)]
        self.view.table.getSelectionShape.return_value = (1, 1)
        self.view.table.getCellContents.return_value = ""
        self.view.copySelectedCells()
        self.view.table.getCellContents.assert_called()
        self.assertEqual(self.view.buffer, ["test"])
        self.assertEqual(self.view.bufferShape, (1, 1))

    def test_cutSelectedCells(self):
        # no selection
        self.view.table.getSelectedCells.return_value = []
        self.view.table.getSelectionShape.return_value = (0, 0)
        self.view.cutSelectedCells()
        self.assertEqual(self.view.buffer, [])
        self.assertEqual(self.view.bufferShape, ())
        self.view.table.getCellContents.assert_not_called()

        # valid selection
        self.view.table.getSelectedCells.return_value = [(0, 0)]
        self.view.table.getSelectionShape.return_value = (1, 1)
        self.view.table.getCellContents.return_value = "test"
        self.view.cutSelectedCells()
        self.assertEqual(self.view.buffer, ["test"])
        self.assertEqual(self.view.bufferShape, (1, 1))
        self.view.table.eraseCell.assert_called_once_with(0, 0)

        self.view.table.reset_mock()
        self.view.table.getSelectedCells.return_value = [(0, 0), (0, 1), (1, 0), (1, 1)]
        self.view.table.getSelectionShape.return_value = (2, 2)
        self.view.table.getCellContents.return_value = "test"
        self.view.cutSelectedCells()
        self.assertEqual(self.view.buffer, ["test", "test", "test", "test"])
        self.assertEqual(self.view.bufferShape, (2, 2))
        calls = [mock.call(0, 0), mock.call(0, 1), mock.call(1, 0), mock.call(1, 1)]
        self.view.table.eraseCell.assert_has_calls(calls)

        # invalid selection
        self.view.table.reset_mock()
        self.view.table.getSelectedCells.return_value = [(0, 0), (10, 10)]
        self.view.table.getSelectionShape.return_value = (0, 0)
        self.view.table.getCellContents.assert_not_called()
        self.view.table.eraseCell.assert_not_called()

    def test_pasteCells(self):
        # no selection
        self.view.buffer = ["test"]
        self.view.bufferShape = (1, 1)
        self.view.table.getSelectedCells.return_value = []
        self.view.table.getSelectionShape.return_value = (0, 0)
        self.view.pasteCells()
        self.view.table.setCellContents.assert_not_called()

        # empty buffer
        self.view.table.reset_mock()
        self.view.buffer = []
        self.view.bufferShape = ()
        self.view.pasteCells()
        self.view.table.setCellContents.assert_not_called()

        # valid selection
        self.view.buffer = ["test"]
        self.view.bufferShape = (1, 1)
        self.view.table.reset_mock()
        self.view.table.getSelectedCells.return_value = [(1, 1)]
        self.view.table.getSelectionShape.return_value = (1, 1)
        self.view.pasteCells()
        self.view.table.setCellContents.assert_called_once()

        self.view.table.reset_mock()
        self.view.table.getSelectedCells.return_value = [(0, 0), (0, 1), (1, 0), (1, 1)]
        self.view.table.getSelectionShape.return_value = (2, 2)
        self.view.pasteCells()
        calls = [mock.call(0, 0, "test"), mock.call(0, 1, "test"), mock.call(1, 0, "test"), mock.call(1, 1, "test")]
        self.view.table.setCellContents.assert_has_calls(calls)

        self.view.buffer = ["test00", "test01", "test10", "test11"]
        self.view.bufferShape = (2, 2)
        self.view.table.reset_mock()
        self.view.table.getSelectedCells.return_value = [(0, 0), (0, 1), (1, 0), (1, 1)]
        self.view.table.getSelectionShape.return_value = (2, 2)
        self.view.pasteCells()
        calls = [mock.call(0, 0, "test00"), mock.call(0, 1, "test01"), mock.call(1, 0, "test10"), mock.call(1, 1, "test11")]
        self.view.table.setCellContents.assert_has_calls(calls)

    def test_eraseSelectedCells(self):
        # no selection
        self.view.table.getSelectedCells.return_value = []
        self.view.eraseSelectedCells()
        self.view.table.eraseCell.assert_not_called()
        self.view.table.getSelectedCells.return_value = [(0, 0), (0, 1), (2, 1)]
        # selection
        self.view.eraseSelectedCells()
        calls = [mock.call(0, 0), mock.call(0, 1), mock.call(2, 1)]
        self.view.table.eraseCell.assert_has_calls(calls)

    def test_addRowAfter(self):
        self.view.table.getLastSelectedRow.return_value = 0
        self.view.addRowAfter()
        self.mPresenter.return_value.onRowAdded.assert_called_once_with(1)

    def test_addRowsAdter(self):
        self.view.table.getLastSelectedRow.return_value = 0
        self.view.nrows = mock.Mock()
        self.view.nrows.value.return_value = 10
        self.view.addRowsAfter()
        self.mPresenter.assert_called()

    def test_delSelectedRows(self):
        # no selection
        self.view.table.getSelectedRows.return_value = []
        self.view.del_selected_rows()
        self.view.table.deleteRow.assert_not_called()
        # selction
        self.view.table.getSelectedRows.return_value = [0, 1, 2]
        self.view.del_selected_rows()
        calls = [mock.call(2), mock.call(1), mock.call(0)]
        self.view.table.deleteRow.assert_has_calls(calls)

    def test_keyPressEvent(self):
        self.view.copySelectedCells = mock.Mock()
        self.view.cutSelectedCells = mock.Mock()
        self.view.pasteCells = mock.Mock()
        self.view.eraseSelectedCells = mock.Mock()
        self.view.ungroupSelectedRows = mock.Mock()
        self.view.groupSelectedRows = mock.Mock()
        self.view.setMasterRow = mock.Mock()
        self.view.table.getRowsFromSelectedCells.return_value = [1]

        QTest.keyClick(self.view, Qt.Key_C, Qt.ControlModifier)
        self.view.copySelectedCells.assert_called_once()
        QTest.keyClick(self.view, Qt.Key_X, Qt.ControlModifier)
        self.view.cutSelectedCells.assert_called_once()
        QTest.keyClick(self.view, Qt.Key_V, Qt.ControlModifier)
        self.view.pasteCells.assert_called_once()
        QTest.keyClick(self.view, Qt.Key_Delete, Qt.NoModifier)
        self.view.eraseSelectedCells.assert_called_once()
        QTest.keyClick(self.view, Qt.Key_G, Qt.ControlModifier)
        self.view.groupSelectedRows.emit.assert_called_once()
        QTest.keyClick(self.view, Qt.Key_G, Qt.ControlModifier | Qt.ShiftModifier)
        self.view.ungroupSelectedRows.emit.assert_called_once()
        QTest.keyClick(self.view, Qt.Key_M, Qt.ControlModifier)
        self.view.setMasterRow.emit.assert_called_once()

    def test_showDirectoryManager(self):
        self.view.show_directory_manager()
        self.mUserDir.ManageUserDirectories.assert_called_once()

    def test_setInstrument(self):
        self.view.instrumentselector = mock.Mock()
        self.view.setInstrument("i1")
        self.view.instrumentselector.setCurrentText.assert_called_once_with("i1")

    def test_setAvailableModes(self):
        self.view.modeSelector = mock.Mock()
        self.view.set_available_modes(["test", "test"])
        self.view.modeSelector.clear.assert_called_once()
        self.view.modeSelector.addItems.assert_called_once_with(["test", "test"])

    def test_setAcquisitionMode(self):
        self.view.modeSelector = mock.Mock()
        self.view.set_acquisition_mode("test")
        self.view.modeSelector.setCurrentText.assert_called_once_with("test")

    def test_getAcquisitionMode(self):
        self.view.modeSelector = mock.Mock()
        self.view.modeSelector.currentText.return_value = "mode"
        mode = self.view.getAcquisitionMode()
        self.assertEqual(mode, "mode")

    def test_setCycleAndExperiment(self):
        self.view.setCycleAndExperiment("test1", "test2")
        self.assertEqual(self.view.cycleNumber.text(), "test1")
        self.assertEqual(self.view.experimentId.text(), "test2")

    def test_setTable(self):
        self.view.set_table(["test", "test"])
        self.view.table.setColumnCount.assert_called_with(2)

    def test_setProgress(self):
        self.view.set_progress(0, 100)
        self.assertEqual(self.view.progressBar.maximum(), 100)
        self.assertEqual(self.view.progressBar.value(), 0)
        self.view.set_progress(100, 0)
        self.assertEqual(self.view.progressBar.maximum(), 0)
        self.assertEqual(self.view.progressBar.value(), 100)

    def test_setDisabled(self):
        # only visual. here we just go through the function
        self.view.set_disabled(True)
        self.view.set_disabled(False)

    def test_setVisualSettings(self):
        self.view.columns = ["test"]
        self.view.setVisualSettings(dict())
        self.view.table.setFoldedColumns.assert_not_called()
        self.view.setVisualSettings({"FoldedColumns": {}})
        self.view.table.setFoldedColumns.assert_called_once_with([])
        self.view.table.reset_mock()
        self.view.setVisualSettings({"FoldedColumns": {"test": True}})
        self.view.table.setFoldedColumns.assert_called_once_with(["test"])
        self.view.table.reset_mock()
        self.view.setVisualSettings({"FoldedColumns": {"test1": True}})
        self.view.table.setFoldedColumns.assert_called_once_with(["test1"])
        self.view.table.reset_mock()
        self.view.setVisualSettings({"FoldedColumns": ["test"]})
        self.view.table.setFoldedColumns.assert_called_once_with(["test"])

    def test_getVisualSettings(self):
        self.view.columns = ["test1", "test2", "test3"]
        self.view.table.getFoldedColumns.return_value = ["test1", "test3"]
        self.view.table.getHiddenColumns.return_value = ["test1", "test2"]
        self.view.table.getColumnsOrder.return_value = ["test1", "test2", "test3"]
        d = {"FoldedColumns": ["test1", "test3"], "HiddenColumns": ["test1", "test2"], "ColumnsOrder": ["test1", "test2", "test3"]}
        self.assertDictEqual(self.view.getVisualSettings(), d)


if __name__ == "__main__":
    unittest.main()
