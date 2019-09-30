# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, print_function)

import unittest

from qtpy.QtWidgets import QApplication

from mantid.simpleapi import CreateEmptyTableWorkspace
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.workspacedisplay.table.presenter import TableWorkspaceDisplay
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder


@start_qapplication
class TableWorkspaceDisplayViewQtTest(unittest.TestCase, QtWidgetFinder):

    def test_window_deleted_correctly(self):
        ws = CreateEmptyTableWorkspace()

        p = TableWorkspaceDisplay(ws)
        self.assert_widget_created()
        p.close(ws.name())

        QApplication.processEvents()

        self.assertEqual(None, p.ads_observer)
        self.assert_widget_not_present("work")
        self.assert_no_toplevel_widgets()

    def test_window_force_deleted_correctly(self):
        ws = CreateEmptyTableWorkspace()

        p = TableWorkspaceDisplay(ws)
        self.assert_widget_created()
        p.force_close()

        QApplication.processEvents()

        self.assertEqual(None, p.ads_observer)
        self.assert_widget_not_present("work")
        self.assert_no_toplevel_widgets()


@start_qapplication
class TableWorkspaceDisplayViewTest(unittest.TestCase):

    def test_gui_updated_when_row_added_from_dictionary(self):
        ws = CreateEmptyTableWorkspace()
        ws.addColumn("double", "test_col")

        presenter = TableWorkspaceDisplay(ws)
        current_rows = presenter.view.rowCount()
        ws.addRow({'test_col': 1.0})

        self.assertEqual(current_rows + 1, presenter.view.rowCount())

    def test_gui_updated_when_row_added_from_sequence(self):
        ws = CreateEmptyTableWorkspace()
        ws.addColumn("double", "l")

        presenter = TableWorkspaceDisplay(ws)
        current_rows = presenter.view.rowCount()
        ws.addRow([1.0])

        self.assertEqual(current_rows + 1, presenter.view.rowCount())

    def test_gui_updated_when_column_removed(self):
        ws = CreateEmptyTableWorkspace()
        ws.addColumn("double", "test_col")

        presenter = TableWorkspaceDisplay(ws)
        ws.removeColumn('test_col')

        self.assertEqual(0, presenter.view.columnCount())


if __name__ == '__main__':
    unittest.main()
