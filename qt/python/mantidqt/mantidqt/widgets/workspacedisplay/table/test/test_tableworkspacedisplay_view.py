# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest

from qtpy.QtWidgets import QApplication

from mantid.simpleapi import CreateEmptyTableWorkspace
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.workspacedisplay.table.presenter import TableWorkspaceDisplay
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.workspacedisplay.table.table_model import BATCH_SIZE


@start_qapplication
class TableWorkspaceDisplayViewQtTest(unittest.TestCase, QtWidgetFinder):

    def test_window_deleted_correctly(self):
        ws = CreateEmptyTableWorkspace()

        p = TableWorkspaceDisplay(ws)
        self.assert_widget_created()
        p.close(ws.name())

        QApplication.sendPostedEvents()

        self.assertEqual(None, p.ads_observer)
        self.assert_widget_not_present("work")
        self.assert_no_toplevel_widgets()

    def test_window_force_deleted_correctly(self):
        ws = CreateEmptyTableWorkspace()

        p = TableWorkspaceDisplay(ws)
        self.assert_widget_created()
        p.force_close()

        QApplication.sendPostedEvents()

        self.assertEqual(None, p.ads_observer)
        self.assert_widget_not_present("work")
        self.assert_no_toplevel_widgets()


@start_qapplication
class TableWorkspaceDisplayViewTest(unittest.TestCase):

    def test_gui_updated_when_row_added_from_dictionary_standard(self):
        ws = CreateEmptyTableWorkspace()
        ws.addColumn("double", "test_col")

        presenter = TableWorkspaceDisplay(ws, batch=False)
        presenter.model.block_model_replace = False
        current_rows = presenter.view.rowCount()
        ws.addRow({'test_col': 1.0})

        self.assertEqual(current_rows + 1, presenter.view.model().rowCount())
        presenter.close(ws.name())

    def test_gui_updated_when_row_added_from_sequence_standard(self):
        ws = CreateEmptyTableWorkspace()
        ws.addColumn("double", "l")

        presenter = TableWorkspaceDisplay(ws, batch=False)
        presenter.model.block_model_replace = False
        current_rows = presenter.view.rowCount()
        ws.addRow([1.0])

        self.assertEqual(current_rows + 1, presenter.view.model().rowCount())
        presenter.close(ws.name())

    def test_gui_updated_when_column_removed_batch(self):
        ws = CreateEmptyTableWorkspace()
        ws.addColumn("double", "test_col")

        presenter = TableWorkspaceDisplay(ws, batch=True)
        presenter.model.block_model_replace = False
        ws.removeColumn('test_col')

        self.assertEqual(0, presenter.view.columnCount())
        presenter.close(ws.name())

    def test_gui_updated_when_row_added_from_dictionary_batch(self):
        ws = CreateEmptyTableWorkspace()
        ws.addColumn("double", "test_col")

        presenter = TableWorkspaceDisplay(ws, batch=True)
        presenter.model.block_model_replace = False
        current_rows = presenter.view.rowCount()
        ws.addRow({'test_col': 1.0})

        self.assertEqual(current_rows + 1, presenter.view.model().max_rows())
        presenter.close(ws.name())

    def test_correct_number_of_rows_fetched_initially_batch(self):
        ws = CreateEmptyTableWorkspace()
        ws.addColumn("double", "l")
        list(map(ws.addRow, ([i] for i in range(5 * BATCH_SIZE))))
        presenter = TableWorkspaceDisplay(ws, batch=True)
        # fetch more starting at index 0,0
        index = presenter.view.model().index(0, 0)
        presenter.view.model().fetchMore(index)
        self.assertEqual(5 * BATCH_SIZE, presenter.view.model().max_rows())
        self.assertEqual(BATCH_SIZE, presenter.view.model().rowCount())
        presenter.close(ws.name())

    def test_scrolling_updates_number_of_rows_fetched_batch(self):
        ws = CreateEmptyTableWorkspace()
        ws.addColumn("double", "l")
        list(map(ws.addRow, ([i] for i in range(5 * BATCH_SIZE))))
        presenter = TableWorkspaceDisplay(ws, batch=True)
        # fetch more starting at index 0,0
        index = presenter.view.model().index(0, 0)
        presenter.view.model().fetchMore(index)
        self.assertEqual(BATCH_SIZE, presenter.view.model().rowCount())
        # scrolling should update our batch size to 2*BATCH_SIZE
        presenter.view.scrollToBottom()
        self.assertEqual(2 * BATCH_SIZE, presenter.view.model().rowCount())
        presenter.close(ws.name())


if __name__ == '__main__':
    unittest.main()
