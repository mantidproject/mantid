# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqtinterfaces.drill.presenter.DrillPresenter import DrillPresenter


class DrillPresenterTest(unittest.TestCase):
    def setUp(self):
        patch = mock.patch("mantidqtinterfaces.drill.presenter.DrillPresenter.DrillModel")
        self.mModel = patch.start()
        self.mModel.return_value.getColumnHeaderData.return_value = [], []
        self.mModel.return_value.getCycleAndExperiment.return_value = "", ""
        self.addCleanup(patch.stop)

        self.view = mock.Mock()
        self.view.windowTitle.return_value = ""
        self.mTable = mock.Mock()
        self.presenter = DrillPresenter(self.view, self.mTable)
        self.model = self.mModel.return_value

    def test_onRowAdded(self):
        self.presenter.onRowAdded(10)
        self.model.addSample.assert_called_once_with(10)
        self.view.setWindowModified.assert_called_once_with(True)

    @mock.patch("mantidqtinterfaces.drill.presenter.DrillPresenter.DrillSamplePresenter")
    def test_onNewSample(self, mSamplePresenter):
        sample = mock.Mock()
        sample.getIndex.return_value = 10
        self.presenter.onNewSample(sample)
        self.mTable.addRow.assert_called_once_with(10)
        mSamplePresenter.assert_called_once_with(self.mTable, sample)

    def test_onAutomaticFilling(self):
        # no selection
        self.view.increment.value.return_value = 1
        self.mTable.getSelectedCells.return_value = []
        self.mTable.getRowsFromSelectedCells.return_value = []
        self.presenter.onAutomaticFilling()
        self.mTable.setCellContents.assert_not_called()
        # single row
        self.view.increment.value.return_value = 1
        self.mTable.getSelectedCells.return_value = [(0, 0), (0, 1)]
        self.mTable.getRowsFromSelectedCells.return_value = [0]
        self.mTable.getCellContents.return_value = "10,100,1000"
        self.presenter.onAutomaticFilling()
        self.mTable.setCellContents.assert_called_with(0, 1, "11,101,1001")
        self.mTable.setCellContents.reset_mock()
        self.mTable.getCellContents.return_value = "test01,test,test_10"
        self.presenter.onAutomaticFilling()
        self.mTable.setCellContents.assert_called_with(0, 1, "test02,test,test_11")
        self.mTable.setCellContents.reset_mock()
        # multiple columns
        self.view.increment.value.return_value = 7
        self.mTable.getSelectedCells.return_value = [(0, 0), (1, 0), (0, 1), (1, 1)]
        self.mTable.getRowsFromSelectedCells.return_value = [0, 1]
        self.mTable.getCellContents.return_value = "10+15,100:200,1:10:2"
        self.presenter.onAutomaticFilling()
        calls = [mock.call(1, 0, "17+22,207:307,17:26:2"), mock.call(1, 1, "17+22,207:307,17:26:2")]
        self.mTable.setCellContents.assert_has_calls(calls)

    def test_onGroupSelectedRows(self):
        self.mTable.getRowsFromSelectedCells.return_value = [0, 2, 5]
        self.presenter.onGroupSelectedRows()
        self.model.groupSamples.assert_called_once_with([0, 2, 5])
        self.view.setWindowModified.assert_called_once_with(True)

    def test_onUngroupSelectedRows(self):
        self.mTable.getRowsFromSelectedCells.return_value = [0, 2, 5]
        self.presenter.onUngroupSelectedRows()
        self.model.ungroupSamples.assert_called_once_with([0, 2, 5])
        self.view.setWindowModified.assert_called_once_with(True)

    def test_onSetMasterRow(self):
        self.mTable.getRowsFromSelectedCells.return_value = [0, 2, 5]
        self.presenter.onSetMasterRow()
        self.model.setGroupMaster.assert_not_called()
        self.view.setWindowModified.assert_not_called()
        self.mTable.getRowsFromSelectedCells.return_value = [2]
        self.presenter.onSetMasterRow()
        self.model.setGroupMaster.assert_called_once_with(2, True)
        self.view.setWindowModified.assert_called_once_with(True)

    def test_onProcess(self):
        self.presenter._process = mock.Mock()
        self.mTable.getSelectedRows.return_value = [1, 2]
        self.presenter.onProcess()
        self.presenter._process.assert_called_once_with([1, 2])
        self.presenter._process.reset_mock()
        self.mTable.getSelectedRows.return_value = []
        self.mTable.getRowsFromSelectedCells.return_value = [0, 1]
        self.presenter.onProcess()
        self.presenter._process.assert_called_once_with([0, 1])
        self.presenter._process.reset_mock()
        self.mTable.getSelectedRows.return_value = []
        self.mTable.getRowsFromSelectedCells.return_value = []
        self.mTable.getAllRows.return_value = [0, 1, 2, 3, 4]
        self.presenter.onProcess()
        self.presenter._process.assert_called_once_with([0, 1, 2, 3, 4])

    def test_onProcessGroup(self):
        self.presenter._process = mock.Mock()
        self.mTable.getSelectedRows.return_value = [1]
        self.presenter.onProcessGroup()
        self.presenter._process.assert_called_once_with([1], True)
        self.presenter._process.reset_mock()
        self.mTable.getSelectedRows.return_value = []
        self.mTable.getRowsFromSelectedCells.return_value = [0, 1]
        self.presenter.onProcessGroup()
        self.presenter._process.assert_called_once_with([0, 1], True)
        self.presenter._process.reset_mock()
        self.mTable.getSelectedRows.return_value = []
        self.mTable.getRowsFromSelectedCells.return_value = []
        self.mTable.getAllRows.return_value = [0, 1, 2, 3, 4]
        self.presenter.onProcessGroup()
        self.presenter._process.assert_called_once_with([0, 1, 2, 3, 4], True)

    def test_onProcessAll(self):
        self.presenter._process = mock.Mock()
        self.mTable.getAllRows.return_value = [0, 1, 2, 3, 4]
        self.presenter.onProcessAll()
        self.presenter._process.assert_called_once_with([0, 1, 2, 3, 4])

    def test_process(self):
        self.presenter._process([])
        self.view.set_disabled.assert_not_called()
        self.model.process.assert_not_called()
        self.presenter._process([0, 1, 4])
        self.view.set_disabled.assert_called_once_with(True)
        self.model.process.assert_called_once_with([0, 1, 4])

    def test_stopProcessing(self):
        self.presenter.stopProcessing()
        self.model.stopProcess.assert_called_once()
        self.view.set_disabled.assert_called_once_with(False)
        self.view.set_progress.assert_called_once_with(0, 100)

    def test_onProcessingDone(self):
        self.presenter.onProcessingDone()
        self.view.set_disabled.assert_called_once_with(False)
        self.view.set_progress.assert_called_once_with(0, 100)

    def test_instrumentChanged(self):
        self.model.reset_mock()
        self.view.isWindowModified.return_value = True
        self.presenter._saveDataQuestion = mock.Mock()
        self.presenter._saveDataQuestion.return_value = True
        self.presenter.instrumentChanged("test")
        self.model.setInstrument.assert_called_once_with("test")
        self.model.setInstrument.reset_mock()
        self.presenter._saveDataQuestion.return_value = False
        self.presenter.instrumentChanged("test")
        self.model.setInstrument.assert_not_called()

    def test_acquisitionModeChanged(self):
        self.model.reset_mock()
        self.view.isWindowModified.return_value = True
        self.presenter._saveDataQuestion = mock.Mock()
        self.presenter._saveDataQuestion.return_value = True
        self.presenter.acquisitionModeChanged("test")
        self.model.setAcquisitionMode.assert_called_once_with("test")
        self.model.setAcquisitionMode.reset_mock()
        self.presenter._saveDataQuestion.return_value = False
        self.model.setAcquisitionMode.assert_not_called()

    @mock.patch("mantidqtinterfaces.drill.presenter.DrillPresenter.QFileDialog")
    def test_onLoad(self, mDialog):
        mDialog.getOpenFileName.return_value = ("test", "test")
        self.presenter.onLoad()
        self.model.setIOFile.assert_called_once_with("test")
        self.view.setWindowModified.assert_called()

    def test_onSave(self):
        self.presenter.onSaveAs = mock.Mock()
        self.model.getIOFile.return_value = 1
        self.presenter.onSave()
        self.model.exportRundexData.assert_called_once()
        self.view.setWindowModified.assert_called()
        self.presenter.onSaveAs.assert_not_called()
        self.model.getIOFile.return_value = None
        self.presenter.onSave()
        self.presenter.onSaveAs.assert_called_once()

    @mock.patch("mantidqtinterfaces.drill.presenter.DrillPresenter.QFileDialog")
    def test_onSaveAs(self, mDialog):
        mDialog.getSaveFileName.return_value = ("test", "test")
        self.presenter.onSaveAs()
        self.model.setIOFile.assert_called_once_with("test")
        self.model.exportRundexData.assert_called_once()
        self.view.setWindowModified.assert_called()

    @mock.patch("mantidqtinterfaces.drill.presenter.DrillPresenter.RundexSettings")
    @mock.patch("mantidqtinterfaces.drill.presenter.DrillPresenter.DrillSettingsPresenter")
    def test_settingsWindow(self, mSettingsPresenter, mRundexSettings):
        mRundexSettings.SETTINGS = {"a1": ["param1", "param2"], "a2": ["param3"]}
        self.model.getAcquisitionMode.return_value = "a1"
        p1 = mock.Mock()
        p1.getName.return_value = "param1"
        p2 = mock.Mock()
        p2.getName.return_value = "param2"
        p3 = mock.Mock()
        p3.getName.return_value = "param3"
        p4 = mock.Mock()
        p4.getName.return_value = "param4"
        self.model.getParameters.return_value = [p1, p2, p3, p4]
        self.presenter.settingsWindow()
        mSettingsPresenter.assert_called_once_with(self.view, [p1, p2])

    @mock.patch("mantidqtinterfaces.drill.presenter.DrillPresenter.DrillExportPresenter")
    def test_onShowExportDialog(self, mExportPresenter):
        dialog = mock.Mock()
        exportModel = mock.Mock()
        self.model.getExportModel.return_value = exportModel
        self.presenter.onShowExportDialog(dialog)
        mExportPresenter.assert_called_once_with(dialog, exportModel)

    @mock.patch("mantidqtinterfaces.drill.presenter.DrillPresenter.DrillContextMenuPresenter")
    def test_onShowContextMenu(self, mContextMenuPresenter):
        menu = mock.Mock()
        self.presenter.onShowContextMenu(menu)
        mContextMenuPresenter.assert_called_once_with(self.view, self.model, menu)

    def test_onClose(self):
        self.view.isWindowModified.return_value = True
        self.presenter._saveDataQuestion = mock.Mock()
        self.presenter.onClose()
        self.presenter._saveDataQuestion.assert_called_once()

    def test_onNew(self):
        self.view.isWindowModified.return_value = True
        self.presenter._saveDataQuestion = mock.Mock()
        self.presenter._syncViewHeader = mock.Mock()
        self.presenter._saveDataQuestion.return_value = False
        self.presenter.onNew()
        self.model.clear.assert_not_called()
        self.model.resetIOFile.assert_not_called()
        self.presenter._syncViewHeader.assert_not_called()
        self.view.isWindowModified.return_value = False
        self.presenter.onNew()
        self.model.clear.assert_called_once()
        self.model.resetIOFile.assert_called_once()
        self.presenter._syncViewHeader.assert_called_once()

    @mock.patch("mantidqtinterfaces.drill.presenter.DrillPresenter.QMessageBox")
    def test_saveDataQuestion(self, mQMessageBox):
        mQMessageBox.Yes = "yes"
        mQMessageBox.question.return_value = "yes"
        self.view.isHidden.return_value = True
        self.presenter._saveDataQuestion()
        mQMessageBox.assert_not_called()
        self.view.isHidden.return_value = False
        self.presenter.onSaveAs = mock.Mock()
        self.presenter._saveDataQuestion()
        mQMessageBox.question.assert_called_once()
        self.presenter.onSaveAs.assert_called_once()

    def test_syncViewHeader(self):
        self.view.reset_mock()
        self.model.getAvailableAcquisitionModes.return_value = ["a1", "a2", "a3"]
        self.model.getInstrument.return_value = "i1"
        self.model.getAcquisitionMode.return_value = "a1"
        self.model.getCycleAndExperiment.return_value = "c1", "exp1"
        self.presenter._syncViewHeader()
        self.view.setInstrument.assert_called_once_with("i1")
        self.view.set_available_modes.assert_called_once_with(["a1", "a2", "a3"])
        self.view.set_acquisition_mode.assert_called_once_with("a1")
        self.view.setCycleAndExperiment.assert_called_once_with("c1", "exp1")

    @mock.patch("mantidqtinterfaces.drill.presenter.DrillPresenter.RundexSettings")
    def test_resetTable(self, mSettings):
        self.view.reset_mock()
        mSettings.COLUMNS = {"a1": ["c1", "c2"]}
        self.model.getAcquisitionMode.return_value = "a2"
        p1 = mock.Mock()
        p1.getName.return_value = "c1"
        p1.getDocumentation.return_value = "doc1"
        p2 = mock.Mock()
        p2.getName.return_value = "c2"
        p2.getDocumentation.return_value = "doc2"
        self.model.getParameters.return_value = [p1, p2]
        r = self.presenter._resetTable()
        self.assertFalse(r)
        self.view.set_table.assert_called_once_with([], [])
        self.model.getAcquisitionMode.return_value = "a1"
        self.view.reset_mock()
        r = self.presenter._resetTable()
        self.assertTrue(r)
        self.view.set_table.assert_called_once_with(["c1", "c2"], ["doc1", "doc2"])


if __name__ == "__main__":
    unittest.main()
