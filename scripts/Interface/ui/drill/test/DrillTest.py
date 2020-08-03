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
from qtpy.QtTest import QTest
from qtpy.QtCore import Qt, QPoint

from mantid.kernel import config

from Interface.ui.drill.view.DrillView import DrillView
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

    def setCellContents(self, row, column, contents):
        """
        Fill a cell.

        Args:
            row (int): row index
            column (int): column index
            contents (str): cell contents
        """
        self.view.table.setCellContents(row, column, contents)

    def fillTable(self, nrows):
        """
        Fill the table with data. This methods creates rows and fills all the
        cells with unique text. It then returns a list of all the cell contents.

        Args:
            nrows (int): number of rows that the table will contain

        Returns:
            list(dict(str: str)): list of row contents
        """
        data = list()
        text = "test"
        for n in range(nrows):
            if n >= self.view.table.rowCount():
                self.selectRow(n-1, Qt.NoModifier)
                self.view.add_row_after()
            for c in range(self.view.table.columnCount()):
                self.setCellContents(n, c, text + str(n) + str(c))
            data.append(dict())
            data[-1].update(self.model.samples[n])
            n += 1
        self.view.table.selectionModel().clear()
        return data

    ###########################################################################
    # tests                                                                   #
    ###########################################################################

    def setUp(self):
        self.facility = config['default.facility']
        config['default.facility'] = "ILL"
        # avoid popup messages
        patch = mock.patch('Interface.ui.drill.view.DrillView.QMessageBox')
        self.mMessageBox = patch.start()
        self.addCleanup(patch.stop)
        # mock the controller
        patch = mock.patch('Interface.ui.drill.model.DrillModel.ParameterController')
        self.mController = patch.start()
        self.addCleanup(patch.stop)

        self.view = DrillView()
        self.presenter = self.view._presenter
        self.model = self.presenter.model

    def tearDown(self):
        config['default.facility'] = self.facility

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

    def test_changeCycleAndExperiment(self):
        # only 1 value is set
        self.view.cycleNumber.setText("test1")
        self.view.cycleNumber.editingFinished.emit()
        self.assertIsNone(self.model.cycleNumber)
        self.assertIsNone(self.model.experimentId)
        self.view.cycleNumber.setText("")
        self.view.experimentId.setText("test2")
        self.view.experimentId.editingFinished.emit()
        self.assertIsNone(self.model.cycleNumber)
        self.assertIsNone(self.model.experimentId)

        # both cycle and exp set
        self.view.cycleNumber.setText("test1")
        self.view.experimentId.setText("test2")
        self.assertEqual(self.model.cycleNumber, "test1")
        self.assertEqual(self.model.experimentId, "test2")
        self.view.cycleNumber.setText("test2")
        self.view.experimentId.setText("test1")
        self.assertEqual(self.model.cycleNumber, "test2")
        self.assertEqual(self.model.experimentId, "test1")

    @mock.patch('Interface.ui.drill.view.DrillView.manageuserdirectories')
    def test_userDirectories(self, mDirectoriesManager):
        QTest.mouseClick(self.view.datadirs, Qt.LeftButton)
        mDirectoriesManager.ManageUserDirectories.assert_called_once()
        mDirectoriesManager.reset_mock()
        self.view.actionManageDirectories.trigger()
        mDirectoriesManager.ManageUserDirectories.assert_called_once()

    def test_settingsWindow(self):
        self.view.showSettings = mock.Mock()
        self.view.setup_header()
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
                        False,
                        False,
                        False
                        ]
                    },
                'GlobalSettings': RundexSettings.SETTINGS['SANS'],
                'Samples': []
                }
        self.assertDictEqual(json, mJson.dump.call_args[0][0])

    def test_addRow(self):
        data = self.fillTable(2)

        # 1 row at the end
        QTest.mouseClick(self.view.addrow, Qt.LeftButton)
        data.append(dict())
        self.assertEqual(self.model.samples, data)

        # 2 rows at the end
        self.view.nrows.setValue(2)
        QTest.mouseClick(self.view.addrow, Qt.LeftButton)
        data.append(dict())
        data.append(dict())
        self.assertEqual(self.model.samples, data)

        # 1 row after the first
        self.view.nrows.setValue(1)
        self.selectRow(0, Qt.NoModifier)
        QTest.mouseClick(self.view.addrow, Qt.LeftButton)
        data.insert(1, dict())
        self.assertEqual(self.model.samples, data)

    def test_deleteRows(self):
        data = self.fillTable(10)

        # no selection
        QTest.mouseClick(self.view.deleterow, Qt.LeftButton)
        self.assertEqual(self.model.samples, data)

        # 1 row
        self.selectRow(0, Qt.NoModifier)
        QTest.mouseClick(self.view.deleterow, Qt.LeftButton)
        del data[0]
        self.assertEqual(self.model.samples, data)

        # several rows
        self.selectRow(0, Qt.NoModifier)
        self.selectRow(2, Qt.ControlModifier)
        QTest.mouseClick(self.view.deleterow, Qt.LeftButton)
        del data[2]
        del data[0]
        self.assertEqual(self.model.samples, data)
        self.selectRow(0, Qt.NoModifier)
        self.selectRow(2, Qt.ShiftModifier)
        QTest.mouseClick(self.view.deleterow, Qt.LeftButton)
        del data[2]
        del data[1]
        del data[0]
        self.assertEqual(self.model.samples, data)

    def test_cut(self):
        data = self.fillTable(10)

        # no selection
        QTest.mouseClick(self.view.cut, Qt.LeftButton)
        self.assertEqual(self.model.samples, data)

        # 1 row
        self.selectRow(0, Qt.NoModifier)
        QTest.mouseClick(self.view.cut, Qt.LeftButton)
        data[0] = {}
        self.assertEqual(self.model.samples, data)

        # several rows
        self.selectRow(0, Qt.NoModifier)
        self.selectRow(2, Qt.ControlModifier)
        QTest.mouseClick(self.view.cut, Qt.LeftButton)
        data[0] = {}
        data[2] = {}
        self.assertEqual(self.model.samples, data)
        self.selectRow(3, Qt.NoModifier)
        self.selectRow(5, Qt.ShiftModifier)
        QTest.mouseClick(self.view.cut, Qt.LeftButton)
        data[3] = {}
        data[4] = {}
        data[5] = {}
        self.assertEqual(self.model.samples, data)

        # cells
        self.selectCell(6, 0, Qt.NoModifier)
        self.selectCell(6, 1, Qt.ControlModifier)
        QTest.mouseClick(self.view.cut, Qt.LeftButton)
        del data[6][self.view.columns[0]]
        del data[6][self.view.columns[1]]
        self.assertEqual(self.model.samples, data)

    def test_copy(self):
        data = self.fillTable(10)

        # no selection
        QTest.mouseClick(self.view.copy, Qt.LeftButton)
        self.assertEqual(self.model.samples, data)

        # row(s)
        self.selectRow(0, Qt.NoModifier)
        QTest.mouseClick(self.view.copy, Qt.LeftButton)
        self.assertEqual(self.model.samples, data)
        self.selectRow(0, Qt.NoModifier)
        self.selectRow(5, Qt.ControlModifier)
        QTest.mouseClick(self.view.copy, Qt.LeftButton)
        self.assertEqual(self.model.samples, data)

        # cells
        self.selectCell(0, 0, Qt.NoModifier)
        QTest.mouseClick(self.view.copy, Qt.LeftButton)
        self.assertEqual(self.model.samples, data)
        self.selectCell(0, 0, Qt.NoModifier)
        self.selectCell(5, 0, Qt.ShiftModifier)
        QTest.mouseClick(self.view.copy, Qt.LeftButton)
        self.assertEqual(self.model.samples, data)

    def test_paste(self):
        data = self.fillTable(8)

        # no selection
        QTest.mouseClick(self.view.paste, Qt.LeftButton)
        self.assertEqual(self.model.samples, data)

        # copy paste
        self.selectRow(0, Qt.NoModifier)
        QTest.mouseClick(self.view.copy, Qt.LeftButton)
        self.selectRow(1, Qt.NoModifier)
        QTest.mouseClick(self.view.paste, Qt.LeftButton)
        self.assertDictEqual(self.model.samples[1], data[0])
        self.selectCell(2, 0, Qt.NoModifier)
        QTest.mouseClick(self.view.copy, Qt.LeftButton)
        self.selectRow(3, Qt.NoModifier)
        QTest.mouseClick(self.view.paste, Qt.LeftButton)
        for (k, v) in self.model.samples[3].items():
            self.assertEqual(v, data[2][self.view.columns[0]])

        # cut paste
        self.selectRow(4, Qt.NoModifier)
        QTest.mouseClick(self.view.cut, Qt.LeftButton)
        self.selectRow(5, Qt.NoModifier)
        QTest.mouseClick(self.view.paste, Qt.LeftButton)
        self.assertDictEqual(self.model.samples[1], data[0])
        self.selectCell(6, 0, Qt.NoModifier)
        QTest.mouseClick(self.view.cut, Qt.LeftButton)
        self.selectRow(7, Qt.NoModifier)
        QTest.mouseClick(self.view.paste, Qt.LeftButton)
        for (k, v) in self.model.samples[7].items():
            self.assertEqual(v, data[6][self.view.columns[0]])

    def test_erase(self):
        data = self.fillTable(5)

        # no selection
        QTest.mouseClick(self.view.erase, Qt.LeftButton)
        self.assertEqual(self.model.samples, data)

        # 1 row
        self.selectRow(0, Qt.NoModifier)
        QTest.mouseClick(self.view.erase, Qt.LeftButton)
        data[0] = {}
        self.assertEqual(self.model.samples, data)

        # several rows
        self.selectRow(0, Qt.NoModifier)
        self.selectRow(2, Qt.ControlModifier)
        QTest.mouseClick(self.view.erase, Qt.LeftButton)
        data[2] = {}
        self.assertEqual(self.model.samples, data)
        self.selectRow(1, Qt.NoModifier)
        self.selectRow(4, Qt.ShiftModifier)
        QTest.mouseClick(self.view.erase, Qt.LeftButton)
        data = [{}] * 5
        self.assertEqual(self.model.samples, data)

    def test_increment(self):
        self.view.nrows.setValue(9)
        QTest.mouseClick(self.view.addrow, Qt.LeftButton)
        self.assertEqual(self.model.samples, [{}] * 10)

        # 0 increment
        self.view.increment.setValue(0)
        self.setCellContents(0, 0, "0")
        self.selectColumn(0, Qt.NoModifier)
        QTest.mouseClick(self.view.fill, Qt.LeftButton)
        column = self.model.columns[0]
        for i in range(10):
            self.assertEqual(self.model.samples[i][column], "0")

        # increment
        self.view.increment.setValue(7)
        self.setCellContents(0, 1, "0")
        self.selectColumn(1, Qt.NoModifier)
        QTest.mouseClick(self.view.fill, Qt.LeftButton)
        column = self.model.columns[1]
        value = 0
        for i in range(10):
            self.assertEqual(self.model.samples[i][column], str(value))
            value += 7


if __name__ == "__main__":
    unittest.main()
