# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import unittest

from qtpy.QtCore import Qt

from mantid.kernel import V3D
from mantidqt.widgets.workspacedisplay.table.tableworkspace_item import create_table_item


class TableWorkspaceItemTest(unittest.TestCase):
    def test_initialise_editable_int(self):
        """
        Test that the widget is correctly initialised and the type is correctly kept in the .data call
        """
        mock_data = 12
        w = create_table_item(mock_data, editable=True)
        self.assertEqual(mock_data, w.orig_data)
        self.assertEqual(mock_data, w.data(Qt.DisplayRole))

    def test_initialise_editable_bool(self):
        """
        Test that the widget is correctly initialised and the type is correctly kept in the .data call
        """
        mock_data = True
        w = create_table_item(mock_data, editable=True)
        self.assertEqual(mock_data, w.orig_data)
        self.assertEqual(mock_data, w.data(Qt.DisplayRole))

    def test_initialise_readonly(self):
        """
        Test that the widget converts everything to string if read only
        :return:
        """
        mock_data = 123
        w = create_table_item(mock_data, editable=False)
        self.assertEqual(str(mock_data), w.data(Qt.DisplayRole))

        mock_data = 1.3333333
        w = create_table_item(mock_data, editable=False)
        self.assertEqual(str(mock_data), w.data(Qt.DisplayRole))

        mock_data = V3D(1, 2, 3)
        w = create_table_item(mock_data, editable=False)
        self.assertEqual(str(mock_data), w.data(Qt.DisplayRole))

        mock_data = True
        w = create_table_item(mock_data, editable=False)
        self.assertEqual(str(mock_data), w.data(Qt.DisplayRole))

        mock_data = "apples"
        w = create_table_item(mock_data, editable=False)
        self.assertEqual(str(mock_data), w.data(Qt.DisplayRole))

    def test_initialise_editable_with_v3d(self):
        mock_data = V3D(1, 2, 3)
        w = create_table_item(mock_data, True)
        self.assertEqual(str(mock_data), w.data(Qt.DisplayRole))
        # the original data of the V3D is stored as a string too
        self.assertEqual(str(mock_data), w.orig_data)

    def test_initialise_editable_with_float(self):
        mock_data = 42.00
        w = create_table_item(mock_data, True)
        self.assertEqual(mock_data, w.data(Qt.DisplayRole))
        self.assertEqual(mock_data, w.orig_data)

    def test_reset_sets_item_data_back_to_original_for_editable_item(self):
        mock_data = 42
        w = create_table_item(mock_data, True)

        w.setData(3, Qt.DisplayRole)
        w.reset()

        self.assertEqual(mock_data, w.data(Qt.DisplayRole))

    def test_sync_updates_orignal_data_from_model_for_editable_item(self):
        w = create_table_item(400, editable=True)
        new_data = 5
        w.setData(new_data, Qt.DisplayRole)

        w.sync()

        self.assertEqual(new_data, w.orig_data)


if __name__ == '__main__':
    unittest.main()
