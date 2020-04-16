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

from Interface.ui.drill.main.view import DrillView
from Interface.ui.drill.main.model import DrillModel
from Interface.ui.drill.main.presenter import DrillPresenter

app = QApplication(sys.argv)

class DrillTest(unittest.TestCase):
    def setUp(self):
        self.view = DrillView()
        self.model = DrillModel()
        self.presenter = DrillPresenter(self.model, self.view)

    def select_cell(self, row, column, modifier):
        # find the middle of the cell
        y = self.view.table.rowViewportPosition(row) \
            + self.view.table.rowHeight(row) / 2
        x = self.view.table.columnViewportPosition(column) \
            + self.view.table.columnWidth(column) / 2

        QTest.mouseClick(self.view.table.viewport(),
                         Qt.LeftButton, modifier, QPoint(x, y))

    def select_row(self, row, modifier):
        # find the middle of the row header
        vertical_header = self.view.table.verticalHeader()
        x = 0 + vertical_header.length() / 2
        y = vertical_header.sectionPosition(row) \
            + vertical_header.sectionSize(row) / 2

        QTest.mouseClick(vertical_header.viewport(),
                         Qt.LeftButton, modifier, QPoint(x, y))

    def edit_cell(self, row, column):
        y = self.view.table.rowViewportPosition(row) + 1
        x = self.view.table.columnViewportPosition(column) + 1
        QTest.mouseClick(self.view.table.viewport(),
                         Qt.LeftButton, Qt.NoModifier, QPoint(x, y))
        QTest.mouseDClick(self.view.table.viewport(),
                          Qt.LeftButton, Qt.NoModifier, QPoint(x, y))

    ###########################################################################
    # test view - model connections                                           #
    ###########################################################################

    def test_sans_instrument(self):
        self.presenter.on_instrument_changed("D22")
        self.assertEqual(self.model.technique, "SANS")
        self.assertEqual(self.view.table.columnCount(),
                         len(self.model.columns))

    def test_add_rows(self):
        self.presenter.on_instrument_changed("D22")
        self.assertEqual(self.view.table.rowCount(), 1)
        self.assertEqual(len(self.model.samples), 1)
        QTest.mouseClick(self.view.addrow, Qt.LeftButton)
        QTest.mouseClick(self.view.addrow, Qt.LeftButton)
        self.assertEqual(self.view.table.rowCount(), 3)
        self.assertEqual(len(self.model.samples), 3)

    def test_del_rows(self):
        self.presenter.on_instrument_changed("D22")
        QTest.mouseClick(self.view.addrow, Qt.LeftButton)
        QTest.mouseClick(self.view.addrow, Qt.LeftButton)
        QTest.mouseClick(self.view.addrow, Qt.LeftButton)
        QTest.mouseClick(self.view.addrow, Qt.LeftButton)
        self.assertEqual(self.view.table.rowCount(), 5)
        self.assertEqual(len(self.model.samples), 5)

        # click in the first row
        self.select_row(0, Qt.NoModifier)

        # ctrl-click on the second row
        self.select_row(1, Qt.ControlModifier)

        # delete the first two rows
        QTest.mouseClick(self.view.deleterow, Qt.LeftButton)
        self.assertEqual(self.view.table.rowCount(), 3)
        self.assertEqual(len(self.model.samples), 3)

        # click in the first row
        self.select_row(0, Qt.NoModifier)

        # shift-click on the third row
        self.select_row(2, Qt.ShiftModifier)

        # delete the first three rows
        QTest.mouseClick(self.view.deleterow, Qt.LeftButton)
        self.assertEqual(self.view.table.rowCount(), 0)
        self.assertEqual(len(self.model.samples), 0)

    def test_cell_modification(self):
        self.presenter.on_instrument_changed("D22")

        # edit the first cell
        self.edit_cell(0, 0)
        QTest.keyClick(self.view.table.viewport().focusWidget(),
                       Qt.Key_Enter)
        QTest.keyClick(self.view.table.viewport().focusWidget(),
                       Qt.Key_T)
        QTest.keyClick(self.view.table.viewport().focusWidget(),
                       Qt.Key_E)
        QTest.keyClick(self.view.table.viewport().focusWidget(),
                       Qt.Key_S)
        QTest.keyClick(self.view.table.viewport().focusWidget(),
                       Qt.Key_T)

        # remove focus to validate the input
        self.select_cell(0, 1, Qt.NoModifier)

        self.assertEqual(self.model.samples[0]["SampleRuns"], "test")

    def test_erase_row(self):
        self.presenter.on_instrument_changed("D22")

        # add contents (last column has a special treatment !)
        for i in range(self.view.table.columnCount() - 1):
            item = QTableWidgetItem("test")
            self.view.table.setItem(0, i, item)
        item = QTableWidgetItem("test=test")
        self.view.table.setItem(0, self.view.table.columnCount() - 1, item)
        self.assertEqual(len(self.model.samples[0]),
                         self.view.table.columnCount())

        # erase row
        self.select_row(0, Qt.NoModifier)
        QTest.mouseClick(self.view.erase, Qt.LeftButton)
        self.assertEqual(len(self.model.samples[0]), 0)

    def test_cut_copy_paste(self):
        self.presenter.on_instrument_changed("D22")

        # add contents
        for i in range(self.view.table.columnCount() - 1):
            item = QTableWidgetItem(str(i))
            self.view.table.setItem(0, i, item)
        item = QTableWidgetItem("test=test")
        self.view.table.setItem(0, self.view.table.columnCount() - 1, item)
        reference = self.model.samples[0]

        # cut - paste
        self.select_row(0, Qt.NoModifier)
        QTest.mouseClick(self.view.cut, Qt.LeftButton)
        self.assertEqual(len(self.model.samples), 0)
        QTest.mouseClick(self.view.paste, Qt.LeftButton)
        self.assertEqual(len(self.model.samples), 1)
        self.assertEqual(self.model.samples[0], reference)

        # copy - paste
        self.select_row(0, Qt.NoModifier)
        QTest.mouseClick(self.view.copy, Qt.LeftButton)
        QTest.mouseClick(self.view.paste, Qt.LeftButton)
        self.assertEqual(len(self.model.samples), 2)
        self.assertEqual(self.model.samples[0], self.model.samples[1])
        self.assertEqual(self.model.samples[1], reference)

    ###########################################################################
    # test model specific functions                                           #
    ###########################################################################

    def test_convolute(self):
        self.presenter.on_instrument_changed("D22")

        # add contents
        reference = dict()
        for i in range(self.view.table.columnCount() - 1):
            item = QTableWidgetItem(str(i))
            self.view.table.setItem(0, i, item)
            reference[self.model.columns[i]] = str(i)
        item = QTableWidgetItem("test=test")
        self.view.table.setItem(0, self.view.table.columnCount() - 1, item)
        reference["CustomOptions"] = {"test" : "test"}

        self.assertEqual(reference, self.model.samples[0])

        convolution = self.model.convolute(self.model.samples[0])
        reference.update(reference["CustomOptions"])
        del reference["CustomOptions"]
        self.assertEqual(reference, convolution)

if __name__ == "__main__":
    unittest.main()

