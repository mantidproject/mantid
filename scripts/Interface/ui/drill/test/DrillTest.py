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
        # select a supported instrument
        self.presenter.on_instrument_changed("D22")

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

    def select_column(self, column, modifier):
        # find the middle of the column header
        horizontal_header = self.view.table.horizontalHeader()
        x = horizontal_header.sectionPosition(column) \
            + horizontal_header.sectionSize(column) / 2
        y = 0 + horizontal_header.length() / 2

        QTest.mouseClick(horizontal_header.viewport(),
                         Qt.LeftButton, modifier, QPoint(x, y))

    def edit_cell(self, row, column):
        y = self.view.table.rowViewportPosition(row) + 1
        x = self.view.table.columnViewportPosition(column) + 1
        QTest.mouseClick(self.view.table.viewport(),
                         Qt.LeftButton, Qt.NoModifier, QPoint(x, y))
        QTest.mouseDClick(self.view.table.viewport(),
                          Qt.LeftButton, Qt.NoModifier, QPoint(x, y))

    ###########################################################################
    # test view functions                                                     #
    ###########################################################################

    def test_view_init(self):
        self.assertEqual(self.view.table.rowCount(), 1)

    def test_view_add_row(self):
        n_row = self.view.table.rowCount()
        # add in position 0
        self.view.add_row(0)
        self.assertEqual(self.view.table.rowCount(), n_row + 1)
        # add after the end
        self.view.add_row(n_row + 10)
        self.assertEqual(self.view.table.rowCount(), n_row + 1)
        # add before the first
        self.view.add_row(-1)
        self.assertEqual(self.view.table.rowCount(), n_row + 1)

    def test_view_del_row(self):
        n_row = self.view.table.rowCount()
        self.view.add_row(0)
        # delete an existing row
        row = 0
        self.view.del_row(row)
        self.assertEqual(self.view.table.rowCount(), n_row)
        # delete a non existing row
        row = 20
        self.view.del_row(row)
        self.assertEqual(self.view.table.rowCount(), n_row)
        row = -1
        self.view.del_row(row)
        self.assertEqual(self.view.table.rowCount(), n_row)

    def test_view_erase_row(self):
        self.view.add_row(0)
        n_columns = self.view.table.columnCount()
        # set row contents
        row = 0
        test_str = "test"
        for c in range(n_columns):
            cell = QTableWidgetItem(test_str)
            self.view.table.setItem(row, c, cell)
            self.assertEqual(self.view.table.item(row, c).text(), test_str)
        # erase an existing row
        row = 0
        self.view.erase_row(row)
        for c in range(n_columns):
            self.assertIsNone(self.view.table.item(row, c))
        # erase a non existing row
        row = 20
        self.view.erase_row(row)
        for c in range(n_columns):
            self.assertIsNone(self.view.table.item(row, c))

    def test_view_get_selected_rows(self):
        # no selection
        self.assertEqual(self.view.get_selected_rows(), [])
        # select two rows
        self.view.add_row(0)
        self.view.add_row(0)
        self.select_row(0, Qt.NoModifier)
        self.select_row(1, Qt.ControlModifier)
        self.assertEqual(self.view.get_selected_rows(), [0, 1])

    def test_view_get_selected_cells(self):
        # no selection
        self.assertEqual(self.view.get_selected_cells(), [])
        # select two cells
        self.view.add_row(0)
        self.select_cell(0, 0, Qt.NoModifier)
        self.select_cell(0, 1, Qt.ControlModifier)
        self.assertEqual(self.view.get_selected_cells(), [(0, 0), (0, 1)])

    def test_view_get_last_selected_row(self):
        # no selection
        self.assertEqual(self.view.get_last_selected_row(), -1)
        # select the first two rows
        self.view.add_row(0)
        self.view.add_row(0)
        self.select_row(0, Qt.NoModifier)
        self.select_row(1, Qt.ControlModifier)
        self.assertEqual(self.view.get_last_selected_row(), 1)

    def test_view_get_all_rows(self):
        # empty table
        self.view.del_row(0)
        self.assertEqual(self.view.get_all_rows(), [])
        # add a row
        self.view.add_row(0)
        self.assertEqual(self.view.get_all_rows(), [0])
        # a second
        self.view.add_row(0)
        self.assertEqual(self.view.get_all_rows(), [0, 1])

    def test_view_get_cell_contents(self):
        test_str = "test"
        cell = QTableWidgetItem(test_str)
        self.view.add_row(0)
        self.view.table.setItem(0, 0, cell)
        self.assertEqual(self.view.get_cell_contents(0, 0), test_str)
        # empty cell
        self.assertEqual(self.view.get_cell_contents(0, 1), "")
        # non existing cell
        self.assertEqual(self.view.get_cell_contents(20, 20), "")
        self.assertEqual(self.view.get_cell_contents(-1, -1), "")

    def test_view_set_cell_contents(self):
        test_str = "test"
        self.view.add_row(0)
        # existing cell
        row = 0
        column = 0
        self.view.set_cell_contents(row, column, test_str)
        self.assertEqual(self.view.table.item(row, column).text(), test_str)
        # non existing cell
        row = 20
        column = 20
        self.view.set_cell_contents(row, column, test_str)
        self.assertIsNone(self.view.table.item(row, column))
        row = -1
        column = -1
        self.view.set_cell_contents(row, column, test_str)
        self.assertIsNone(self.view.table.item(row, column))

    def test_view_get_row_contents(self):
        self.view.add_row(0)
        n_columns = self.view.table.columnCount()
        test_row_contents = list()
        for c in range(n_columns):
            test_str = "test_" + str(c)
            test_row_contents.append(test_str)
            cell = QTableWidgetItem(test_str)
            self.view.table.setItem(0, c, cell)
        #existing row
        row = 0
        self.assertEqual(self.view.get_row_contents(row), test_row_contents)
        # non existing row
        row = 20
        self.assertEqual(self.view.get_row_contents(row), [""] * n_columns)
        row = -1
        self.assertEqual(self.view.get_row_contents(row), [""] * n_columns)

    def test_view_set_row_contents(self):
        n_columns = self.view.table.columnCount()
        test_row_contents = list()
        for c in range(n_columns):
            test_str = "test_" + str(c)
            test_row_contents.append(test_str)
        self.view.add_row(0)
        # existing row
        row = 0
        self.view.set_row_contents(row, test_row_contents)
        for c in range(n_columns):
            self.assertEqual(self.view.table.item(row, c).text(),
                             test_row_contents[c])
        # non existing row
        row = 20
        self.view.set_row_contents(row, test_row_contents)
        for c in range(n_columns):
            self.assertIsNone(self.view.table.item(row, c))
        row = -1
        self.view.set_row_contents(row, test_row_contents)
        for c in range(n_columns):
            self.assertIsNone(self.view.table.item(row, c))

    ###########################################################################
    # test view interface and model connections                               #
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
        # add through the icon
        QTest.mouseClick(self.view.addrow, Qt.LeftButton)
        # add through the menu
        self.view.actionAddRow.trigger()
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

        # delete the first three rows with the menu
        self.view.actionDelRow.trigger()
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

        self.view.add_row(0)
        # add contents (last column has a special treatment !)
        for i in range(self.view.table.columnCount() - 1):
            item = QTableWidgetItem("test")
            self.view.table.setItem(0, i, item)
            item = QTableWidgetItem("test")
            self.view.table.setItem(1, i, item)
        item = QTableWidgetItem("test=test")
        self.view.table.setItem(0, self.view.table.columnCount() - 1, item)
        item = QTableWidgetItem("test=test")
        self.view.table.setItem(1, self.view.table.columnCount() - 1, item)
        self.assertEqual(len(self.model.samples[0]),
                         self.view.table.columnCount())
        self.assertEqual(len(self.model.samples[1]),
                        self.view.table.columnCount())

        # erase row with icon
        self.select_row(0, Qt.NoModifier)
        QTest.mouseClick(self.view.erase, Qt.LeftButton)
        self.assertEqual(len(self.model.samples[0]), 0)

        #erase row with menu
        self.select_row(1, Qt.NoModifier)
        self.view.actionErase.trigger()
        self.assertEqual(len(self.model.samples[1]), 0)

    def test_cut_copy_paste(self):
        self.presenter.on_instrument_changed("D22")

        # add contents
        for i in range(self.view.table.columnCount() - 1):
            item = QTableWidgetItem(str(i))
            self.view.table.setItem(0, i, item)
        item = QTableWidgetItem("test=test")
        self.view.table.setItem(0, self.view.table.columnCount() - 1, item)
        reference = self.model.samples[0]

        # cut - paste with icon
        self.select_row(0, Qt.NoModifier)
        QTest.mouseClick(self.view.cut, Qt.LeftButton)
        self.assertEqual(len(self.model.samples), 0)
        QTest.mouseClick(self.view.paste, Qt.LeftButton)
        self.assertEqual(len(self.model.samples), 1)
        self.assertEqual(self.model.samples[0], reference)

        # cut - paste with menu
        self.select_row(0, Qt.NoModifier)
        self.view.actionCutRow.trigger()
        self.assertEqual(len(self.model.samples), 0)
        self.view.actionPasteRow.trigger()
        self.assertEqual(len(self.model.samples), 1)
        self.assertEqual(self.model.samples[0], reference)

        # copy - paste with icon
        self.select_row(0, Qt.NoModifier)
        QTest.mouseClick(self.view.copy, Qt.LeftButton)
        QTest.mouseClick(self.view.paste, Qt.LeftButton)
        self.assertEqual(len(self.model.samples), 2)
        self.assertEqual(self.model.samples[0], self.model.samples[1])
        self.assertEqual(self.model.samples[1], reference)

        # copy - paste with menu
        self.view.del_row(1)
        self.select_row(0, Qt.NoModifier)
        self.view.actionCopyRow.trigger()
        self.view.actionPasteRow.trigger()
        self.assertEqual(len(self.model.samples), 2)
        self.assertEqual(self.model.samples[0], self.model.samples[1])
        self.assertEqual(self.model.samples[1], reference)

    def test_copy_fill(self):
        self.view.add_row(0)
        self.view.add_row(0)
        n_rows = self.view.table.rowCount()
        self.assertEqual(n_rows, 3)
        # one cell filling
        row = 0
        column = 0
        test_str = "test"
        cell = QTableWidgetItem(test_str)
        self.view.table.setItem(row, column, cell)
        self.assertEqual(self.view.table.item(row, column).text(), test_str)
        for r in range(1, n_rows):
            self.assertIsNone(self.view.table.item(r, column))
        # no selection
        QTest.mouseClick(self.view.fill, Qt.LeftButton)
        self.assertEqual(self.view.table.item(row, column).text(), test_str)
        for r in range(1, n_rows):
            self.assertIsNone(self.view.table.item(r, column))
        # individual selections
        self.select_cell(row, column, Qt.NoModifier)
        self.select_cell(row + 1, column, Qt.ControlModifier)
        QTest.mouseClick(self.view.fill, Qt.LeftButton)
        self.assertEqual(self.view.table.item(row, column).text(), test_str)
        self.assertEqual(self.view.table.item(row + 1, column).text(), test_str)
        for r in range(2, n_rows):
            self.assertIsNone(self.view.table.item(r, column))
        # whole column selection
        self.select_column(column, Qt.NoModifier)
        QTest.mouseClick(self.view.fill, Qt.LeftButton)
        for r in range(n_rows):
            self.assertEqual(self.view.table.item(r, column).text(), test_str)

    def test_increment_fill(self):
        self.view.add_row(0)
        self.view.add_row(0)
        n_rows = self.view.table.rowCount()
        self.assertEqual(n_rows, 3)
        # one cell filling
        row = 0
        column = 0
        test_str = "10,20,30"
        cell = QTableWidgetItem(test_str)
        self.view.table.setItem(row, column, cell)
        self.assertEqual(self.view.table.item(row, column).text(), test_str)
        for r in range(1, n_rows):
            self.assertIsNone(self.view.table.item(r, column))
        # 0 increment fill
        self.select_column(column, Qt.NoModifier)
        self.view.increment.setValue(0)
        QTest.mouseClick(self.view.fill, Qt.LeftButton)
        for r in range(n_rows):
            self.assertEqual(self.view.table.item(r, column).text(), test_str)
        # increment
        increment_value = 99
        column = 1
        cell = QTableWidgetItem(test_str)
        self.view.table.setItem(row, column, cell)
        self.assertEqual(self.view.table.item(row, column).text(), test_str)
        for r in range(1, n_rows):
            self.assertIsNone(self.view.table.item(r, column))
        self.select_column(column, Qt.NoModifier)
        self.view.increment.setValue(increment_value)
        QTest.mouseClick(self.view.fill, Qt.LeftButton)
        i = 0
        for r in range(n_rows):
            test_str_increment = ','.join([str(int(n) + i * increment_value)
                                           for n in test_str.split(',')])
            self.assertEqual(self.view.table.item(r, column).text(),
                             test_str_increment)
            i += 1
        # increment a text
        column = 2
        test_str = "a=b,test,54"
        cell = QTableWidgetItem(test_str)
        self.view.table.setItem(row, column, cell)
        self.assertEqual(self.view.table.item(row, column).text(), test_str)
        for r in range(1, n_rows):
            self.assertIsNone(self.view.table.item(r, column))
        self.select_column(column, Qt.NoModifier)
        self.view.increment.setValue(increment_value)
        QTest.mouseClick(self.view.fill, Qt.LeftButton)
        for r in range(n_rows):
            self.assertEqual(self.view.table.item(r, column).text(), test_str)
        # increment a valid numor string
        increment_value = 1
        column = 3
        test_str = "1000,2000+3000,5000:8000,15000-16000"
        cell = QTableWidgetItem(test_str)
        self.view.table.setItem(row, column, cell)
        self.assertEqual(self.view.table.item(row, column).text(), test_str)
        self.select_column(column, Qt.NoModifier)
        self.view.increment.setValue(increment_value)
        QTest.mouseClick(self.view.fill, Qt.LeftButton)
        self.assertEqual(self.view.table.item(1, column).text(),
                         "1001,2001+3001,8001:11001,16001-17001")
        self.assertEqual(self.view.table.item(2, column).text(),
                         "1002,2002+3002,11002:14002,17002-18002")

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

