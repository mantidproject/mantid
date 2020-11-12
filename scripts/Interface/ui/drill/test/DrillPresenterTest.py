# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from qtpy.QtWidgets import QMessageBox

from Interface.ui.drill.presenter.DrillPresenter import DrillPresenter


class DrillPresenterTest(unittest.TestCase):

    def setUp(self):
        patch = mock.patch('Interface.ui.drill.presenter.DrillPresenter'
                           '.DrillSettingsDialog')
        self.mSettings = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch(
                'Interface.ui.drill.presenter.DrillPresenter.DrillModel')
        self.mModel = patch.start()
        self.mModel.return_value.getColumnHeaderData.return_value = [], []
        self.mModel.return_value.getCycleAndExperiment.return_value = "", ""
        self.addCleanup(patch.stop)

        self.view = mock.Mock()
        self.presenter = DrillPresenter(self.view)
        self.model = self.mModel.return_value

    def test_onDataChanged(self):
        self.view.getCellContents.return_value = "test"
        self.presenter.onDataChanged(1, 2)
        self.view.getCellContents.assert_called_once_with(1, 2)
        self.view.unsetRowBackground.assert_called_once_with(1)
        self.view.setWindowModified.assert_called_once_with(True)

    def test_process(self):
        self.presenter.process(["test", "test"])
        self.model.process.assert_called_once_with(["test", "test"])
        self.view.set_disabled.assert_called_once_with(True)
        self.view.set_progress.assert_called_once_with(0, 100)

    def test_stopProcessing(self):
        self.presenter.stopProcessing()
        self.model.stopProcess.assert_called_once()
        self.view.set_disabled.assert_called_once_with(False)
        self.view.set_progress.assert_called_once_with(0, 100)

    def test_onProcessBegin(self):
        self.presenter.onProcessBegin(0)
        self.view.setRowProcessing.assert_called_once_with(0)

    def test_onProcessError(self):
        self.presenter.onProcessError(0)
        self.view.setRowError.assert_called_once_with(0)

    def test_onProcessSuccess(self):
        self.presenter.onProcessSuccess(0)
        self.view.setRowDone.assert_called_once_with(0)

    @mock.patch("Interface.ui.drill.presenter.DrillPresenter.QMessageBox")
    def test_onProcessingDone(self, mQmessageBox):
        self.presenter.onProcessingDone()
        self.view.set_disabled.assert_called_once_with(False)
        self.view.set_progress.assert_called_once_with(0, 100)
        mQmessageBox.assert_not_called()
        self.presenter._processError = {0, 1, 3}
        self.presenter.onProcessingDone()
        mQmessageBox.assert_called_once()

    def test_instrumentChanged(self):
        self.view.isWindowModified.return_value = True
        self.presenter._saveDataQuestion = mock.Mock()
        self.presenter.instrumentChanged("test")
        self.presenter._saveDataQuestion.assert_called_once()
        self.model.setInstrument.assert_called_once_with("test")

    def test_acquisitionModeChanged(self):
        self.view.isWindowModified.return_value = True
        self.presenter._saveDataQuestion = mock.Mock()
        self.presenter.acquisitionModeChanged("test")
        self.presenter._saveDataQuestion.assert_called_once()
        self.model.setAcquisitionMode.assert_called_once_with("test")

    @mock.patch("Interface.ui.drill.presenter.DrillPresenter.QFileDialog")
    def test_onLoad(self, mDialog):
        mDialog.getOpenFileName.return_value = ("test", "test")
        self.presenter.onLoad()
        self.model.setIOFile.assert_called_once_with("test")
        self.view.setWindowModified.assert_called_once_with(False)

    def test_onSave(self):
        self.presenter.onSaveAs = mock.Mock()
        self.model.getIOFile.return_value = 1
        self.presenter.onSave()
        self.model.exportRundexData.assert_called_once()
        self.view.setWindowModified.assert_called_once_with(False)
        self.presenter.onSaveAs.assert_not_called()
        self.model.getIOFile.return_value = None
        self.presenter.onSave()
        self.presenter.onSaveAs.assert_called_once()

    @mock.patch("Interface.ui.drill.presenter.DrillPresenter.QFileDialog")
    def test_onSaveAs(self, mDialog):
        mDialog.getSaveFileName.return_value = ("test", "test")
        self.presenter.onSaveAs()
        self.model.setIOFile.assert_called_once_with("test")
        self.model.exportRundexData.assert_called_once()
        self.view.setWindowModified.assert_called_once_with(False)

    def test_settingsWindow(self):
        self.model.getSettingsTypes.return_value = ({}, {}, {})
        self.presenter.settingsWindow()
        self.mSettings.assert_called_once()
        self.mSettings.return_value.initWidgets.assert_called_once_with(
                {}, {}, {})
        self.mSettings.return_value.setSettings.assert_called_once()
        self.model.getSettings.assert_called_once()

    def test_onClose(self):
        self.view.isWindowModified.return_value = True
        self.presenter._saveDataQuestion = mock.Mock()
        self.presenter.onClose()
        self.presenter._saveDataQuestion.assert_called_once()

    @mock.patch("Interface.ui.drill.presenter.DrillPresenter.QMessageBox")
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

    def test_updateViewFromModel(self):
        self.view.reset_mock()
        self.model.reset_mock()
        self.presenter.updateViewFromModel()

        self.model.getAvailableAcquisitionModes.assert_called_once()
        self.model.getAcquisitionMode.assert_called_once()
        self.model.getColumnHeaderData.assert_called_once()
        self.model.getRowsContents.assert_called_once()

        self.view.set_available_modes.assert_called_once()
        self.view.set_acquisition_mode.assert_called_once()
        self.view.set_table.assert_called_once()
        self.view.fill_table.assert_called_once()


if __name__ == "__main__":
    unittest.main()
