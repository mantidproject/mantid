# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import mock

from Interface.ui.drill.presenter.DrillPresenter import DrillPresenter


class DrillPresenterTest(unittest.TestCase):

    def setUp(self):
        patch = mock.patch(
                'Interface.ui.drill.presenter.DrillPresenter.SettingsDialog')
        self.mSettings = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch(
                'Interface.ui.drill.presenter.DrillPresenter.DrillModel')
        self.mModel = patch.start()
        self.addCleanup(patch.stop)

        self.view = mock.Mock()
        self.presenter = DrillPresenter(self.view)
        self.model = self.mModel.return_value

    def test_process(self):
        self.presenter.process(["test", "test"])
        self.model.process.assert_called_once_with(["test", "test"])
        self.view.set_disabled.assert_called_once_with(True)
        self.view.set_progress.assert_called_once_with(0, 100)

        e = Exception()
        e.elements = mock.Mock()
        self.model.process.side_effect = e
        self.presenter.process(["test", "test"])
        self.view.processing_error.assert_called_once_with(e.elements)

    def test_stopProcessing(self):
        self.presenter.stopProcessing()
        self.model.stopProcess.assert_called_once()
        self.view.set_disabled.assert_called_once_with(False)
        self.view.set_progress.assert_called_once_with(0, 100)

    def test_processingDone(self):
        self.presenter.processingDone()
        self.view.set_disabled.assert_called_once_with(False)
        self.view.set_progress.assert_called_once_with(0, 100)

    def test_instrumentChanged(self):
        self.presenter.instrumentChanged("test")
        self.model.setInstrument.assert_called_once_with("test")

    def test_acquisitionModeChanged(self):
        self.presenter.acquisitionModeChanged("test")
        self.model.setAcquisitionMode.assert_called_once_with("test")

    def test_rundexLoaded(self):
        self.presenter.rundexLoaded("test")
        self.model.importRundexData.assert_called_once_with("test")

    def test_settingsWindow(self):
        self.model.getSettingsTypes.return_value = ({}, {}, {})
        self.presenter.settingsWindow()
        self.mSettings.assert_called_once()
        self.mSettings.return_value.initWidgets.assert_called_once_with(
                {}, {}, {})
        self.mSettings.return_value.setSettings.assert_called_once()
        self.model.getSettings.assert_called_once()

    def test_updateViewFromModel(self):
        self.view.reset_mock()
        self.model.reset_mock()
        self.presenter.updateViewFromModel()

        self.model.getAvailableAcquisitionModes.assert_called_once()
        self.model.getAcquisitionMode.assert_called_once()
        self.model.get_columns.assert_called_once()
        self.model.get_rows_contents.assert_called_once()

        self.view.set_available_modes.assert_called_once()
        self.view.set_acquisition_mode.assert_called_once()
        self.view.set_table.assert_called_once()
        self.view.fill_table.assert_called_once()


if __name__ == "__main__":
    unittest.main()
