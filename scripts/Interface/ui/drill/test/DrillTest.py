# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import mock
import sys

from qtpy.QtWidgets import QApplication
from qtpy.QtTest import QTest
from qtpy.QtCore import Qt, QPoint

from Interface.ui.drill.view.DrillView import DrillView
from Interface.ui.drill.model.DrillModel import DrillModel
from Interface.ui.drill.presenter.DrillPresenter import DrillPresenter
from Interface.ui.drill.model.specifications import RundexSettings

app = QApplication(sys.argv)


class DrillTest(unittest.TestCase):

    ###########################################################################
    # helper methods                                                          #
    ###########################################################################

    def selectCell(self, row, column, modifier):
        """
        Select a cell by clicking on it.

        Args:
            row (int): row index
            column (int): column index
            modifier (Qt::KeyboardModifier): modifier key
        """
        # find the middle of the cell
        y = self.view.table.rowViewportPosition(row) \
            + self.view.table.rowHeight(row) / 2
        x = self.view.table.columnViewportPosition(column) \
            + self.view.table.columnWidth(column) / 2

        QTest.mouseClick(self.view.table.viewport(),
                         Qt.LeftButton, modifier, QPoint(x, y))

    def selectRow(self, row, modifier):
        """
        Select a full row by clicking on its header.

        Args:
            row (int): row index
            modifier (Qt::KeyboardModifier): modifier key
        """
        # find the middle of the row header
        vertical_header = self.view.table.verticalHeader()
        x = 0 + vertical_header.length() / 2
        y = vertical_header.sectionPosition(row) \
            + vertical_header.sectionSize(row) / 2

        QTest.mouseClick(vertical_header.viewport(),
                         Qt.LeftButton, modifier, QPoint(x, y))

    def selectColumn(self, column, modifier):
        """
        Select a full column by clicking on its header.

        Args:
            column (int): column index
            modifier (Qt::KeyboardModifier): modifier key
        """
        # find the middle of the column header
        horizontal_header = self.view.table.horizontalHeader()
        x = horizontal_header.sectionPosition(column) \
            + horizontal_header.sectionSize(column) / 2
        y = 0 + horizontal_header.length() / 2

        QTest.mouseClick(horizontal_header.viewport(),
                         Qt.LeftButton, modifier, QPoint(x, y))

    def editCell(self, row, column):
        """
        Enter in cell editing mode (cell double click).

        Args:
            row (int): row index
            column (int): column index
        """
        y = self.view.table.rowViewportPosition(row) + 1
        x = self.view.table.columnViewportPosition(column) + 1
        QTest.mouseClick(self.view.table.viewport(),
                         Qt.LeftButton, Qt.NoModifier, QPoint(x, y))
        QTest.mouseDClick(self.view.table.viewport(),
                          Qt.LeftButton, Qt.NoModifier, QPoint(x, y))

    ###########################################################################
    # tests                                                                   #
    ###########################################################################

    def setUp(self):
        self.view = DrillView()
        self.model = DrillModel()
        self.presenter = DrillPresenter(self.model, self.view)

    def test_changeInstrument(self):
        for i in range(self.view.instrumentselector.count()):
            self.view.instrumentselector.setCurrentIndex(i)
            mInstrument = self.model.instrument
            mAcquisitionMode = self.model.acquisitionMode
            mColumns = self.model.columns
            mAlgorithm = self.model.algorithm
            mSettings = self.model.settings
            self.assertEqual(mInstrument,
                             self.view.instrumentselector.currentText())
            self.assertEqual(mAcquisitionMode,
                             RundexSettings.ACQUISITION_MODES[mInstrument][0])
            self.assertEqual(mColumns,
                             RundexSettings.COLUMNS[mAcquisitionMode])
            self.assertEqual(mAlgorithm,
                             RundexSettings.ALGORITHM[mAcquisitionMode])
            self.assertDictEqual(mSettings,
                                 RundexSettings.SETTINGS[mAcquisitionMode])

    def test_changeAcquisitionMode(self):
        # D17 has two acquisition modes
        self.view.instrumentselector.setCurrentText("D17")

        for i in range(self.view.modeSelector.count()):
            self.view.modeSelector.setCurrentIndex(i)
            mInstrument = self.model.instrument
            mAcquisitionMode = self.model.acquisitionMode
            mColumns = self.model.columns
            mAlgorithm = self.model.algorithm
            mSettings = self.model.settings
            self.assertEqual(mInstrument,
                             self.view.instrumentselector.currentText())
            self.assertEqual(mAcquisitionMode,
                             RundexSettings.ACQUISITION_MODES[mInstrument][i])
            self.assertEqual(mAlgorithm,
                             RundexSettings.ALGORITHM[mAcquisitionMode])
            self.assertEqual(mColumns,
                             RundexSettings.COLUMNS[mAcquisitionMode])
            self.assertDictEqual(mSettings,
                                 RundexSettings.SETTINGS[mAcquisitionMode])

    @mock.patch('Interface.ui.drill.view.DrillView.manageuserdirectories')
    def test_userDirectories(self, mDirectoriesManager):
        QTest.mouseClick(self.view.datadirs, Qt.LeftButton)
        mDirectoriesManager.ManageUserDirectories.assert_called_once()
        mDirectoriesManager.reset_mock()
        self.view.actionManageDirectories.trigger()
        mDirectoriesManager.ManageUserDirectories.assert_called_once()

    def test_settingsWindow(self):
        self.view.showSettings = mock.Mock()
        QTest.mouseClick(self.view.settings, Qt.LeftButton)
        self.view.showSettings.emit.assert_called_once()
        self.view.showSettings.reset_mock()
        self.view.actionSettings.trigger()
        self.view.showSettings.emit.assert_called_once()

    @mock.patch('Interface.ui.drill.view.DrillView.QFileDialog')
    @mock.patch('Interface.ui.drill.model.DrillModel.json')
    @mock.patch('Interface.ui.drill.model.DrillModel.open')
    def test_loadRundex(self, mOpen, mJson, mFileDialog):
        mFileDialog.getOpenFileName.return_value = ["test", "test"]
        mJson.load.return_value = {
                'Instrument': 'D11',
                'AcquisitionMode': 'SANS',
                'GlobalSettings': {},
                'Samples': [],
                }
        QTest.mouseClick(self.view.load, Qt.LeftButton)
        self.assertEqual(self.model.instrument, "D11")
        self.assertEqual(self.view.instrumentselector.currentText(), "D11")
        self.assertEqual(self.model.acquisitionMode, "SANS")
        self.assertEqual(self.view.modeSelector.currentText(), "SANS")
        self.assertEqual(self.model.algorithm, RundexSettings.ALGORITHM['SANS'])
        self.assertEqual(self.model.columns, RundexSettings.COLUMNS['SANS'])
        self.assertDictEqual(self.model.settings, RundexSettings.SETTINGS['SANS'])
        self.assertEqual(self.model.samples, [{}])
        self.assertEqual(self.view.table.columnCount(), len(self.model.columns))

    @mock.patch('Interface.ui.drill.view.DrillView.QFileDialog')
    @mock.patch('Interface.ui.drill.model.DrillModel.json')
    @mock.patch('Interface.ui.drill.model.DrillModel.open')
    def test_saveRundex(self, mOpen, mJson, mFileDialog):
        self.model.setInstrument("D11")
        mFileDialog.getSaveFileName.return_value = ["test", "test"]
        QTest.mouseClick(self.view.save, Qt.LeftButton)
        json = {
                'Instrument': 'D11',
                'AcquisitionMode': 'SANS',
                'VisualSettings': {
                    'FoldingState': [
                        False,
                        False,
                        False,
                        False,
                        False,
                        False,
                        False,
                        False,
                        False,
                        False,
                        False,
                        False,
                        False
                        ]
                    },
                'GlobalSettings': RundexSettings.SETTINGS['SANS'],
                'Samples': []
                }
        self.assertDictEqual(json, mJson.dump.call_args[0][0])


if __name__ == "__main__":
    unittest.main()
