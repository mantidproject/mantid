# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)

import unittest

from qtpy.QtCore import Qt

from mantid.kernel import V3D
from mantidqt.widgets.workspacedisplay.table.workbench_table_widget_item import WorkbenchTableWidgetItem


class WorkbenchTableWidgetItemTest(unittest.TestCase):
    def test_initialise_editable_int(self):
        """
        Test that the widget is correctly initialised and the type is correctly kept in the .data call
        """
        mock_data = 12
        w = WorkbenchTableWidgetItem(mock_data, editable=True)
        self.assertEqual(mock_data, w.display_data)
        self.assertEqual(mock_data, w.data(Qt.DisplayRole))

    def test_initialise_editable_bool(self):
        """
        Test that the widget is correctly initialised and the type is correctly kept in the .data call
        """
        mock_data = True
        w = WorkbenchTableWidgetItem(mock_data, editable=True)
        self.assertEqual(mock_data, w.display_data)
        self.assertEqual(mock_data, w.data(Qt.DisplayRole))

    def test_initialise_readonly(self):
        """
        Test that the widget converts everything to string if read only
        :return:
        """
        mock_data = 123
        w = WorkbenchTableWidgetItem(mock_data, editable=False)
        self.assertEqual(str(mock_data), w.data(Qt.DisplayRole))

        mock_data = 1.3333333
        w = WorkbenchTableWidgetItem(mock_data, editable=False)
        self.assertEqual(str(mock_data), w.data(Qt.DisplayRole))

        mock_data = V3D(1, 2, 3)
        w = WorkbenchTableWidgetItem(mock_data, editable=False)
        self.assertEqual(str(mock_data), w.data(Qt.DisplayRole))

        mock_data = True
        w = WorkbenchTableWidgetItem(mock_data, editable=False)
        self.assertEqual(str(mock_data), w.data(Qt.DisplayRole))

        mock_data = "apples"
        w = WorkbenchTableWidgetItem(mock_data, editable=False)
        self.assertEqual(str(mock_data), w.data(Qt.DisplayRole))

    def test_initialise_editable_with_v3d(self):
        mock_data = V3D(1, 2, 3)
        w = WorkbenchTableWidgetItem(mock_data, True)
        self.assertEqual(str(mock_data), w.data(Qt.DisplayRole))
        # the original data of the V3D is stored as a string too
        self.assertEqual(str(mock_data), w.display_data)

    def test_initialise_editable_with_float(self):
        mock_data = 42.00
        w = WorkbenchTableWidgetItem(mock_data, True)
        self.assertEqual(mock_data, w.data(Qt.DisplayRole))
        self.assertEqual(mock_data, w.display_data)

    def test_reset(self):
        w = WorkbenchTableWidgetItem(500, editable=False)

        w.display_data = 4444
        w.reset()

        self.assertEqual(4444, w.data(Qt.DisplayRole))

    def test_update(self):
        w = WorkbenchTableWidgetItem(500, editable=False)
        w.setData(Qt.DisplayRole, 4444)
        w.update()
        self.assertEqual(4444, w.display_data)


if __name__ == '__main__':
    unittest.main()
