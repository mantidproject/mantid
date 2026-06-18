# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from qtpy.QtCore import Qt
from qtpy.QtGui import QStandardItem, QStandardItemModel

from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_plot_list import DNSPlotListModel


class DNSPlotListModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.plm = DNSPlotListModel()

    def setUp(self):
        self.plm.clear()
        self.item = QStandardItem("_magnetic")
        self.item2 = QStandardItem("def_sf")
        self.item2.setCheckable(True)
        self.item.setCheckable(True)
        self.plm.appendRow(self.item)
        self.plm.appendRow(self.item2)

    def test___init(self):
        self.assertIsInstance(self.plm, DNSPlotListModel)
        self.assertIsInstance(self.plm, QStandardItemModel)

    def test_get_checked_item_names(self):
        self.item.setCheckState(Qt.CheckState.Unchecked)
        test_v = self.plm.get_checked_item_names()
        self.assertEqual(test_v, [])
        self.item.setCheckState(Qt.CheckState.Checked)
        test_v = self.plm.get_checked_item_names()
        self.assertEqual(test_v, ["_magnetic"])

    def test_get_items(self):
        test_v = self.plm.get_items()
        self.assertEqual(test_v, [self.item, self.item2])

    def test_get_names(self):
        test_v = self.plm.get_names()
        self.assertEqual(test_v, ["_magnetic", "def_sf"])

    def test_uncheck_items(self):
        self.item.setCheckState(Qt.CheckState.Checked)
        self.plm.uncheck_items()
        self.assertEqual(self.item.checkState(), Qt.CheckState.Unchecked)
        self.assertEqual(self.item2.checkState(), Qt.CheckState.Unchecked)

    def test_set_items(self):
        self.plm.set_items(["_magnetic", "def_sf", "hij"])
        self.assertEqual(self.plm.rowCount(), 3)
        self.assertEqual(self.plm.item(1).text(), "def_sf")

    def test_get_checked_item_numbers(self):
        self.assertEqual(self.plm.get_checked_item_numbers(), [])
        self.item.setCheckState(Qt.CheckState.Checked)
        self.item2.setCheckState(Qt.CheckState.Unchecked)
        self.assertEqual(self.plm.get_checked_item_numbers(), [0])

    def test_down(self):
        self.item.setCheckState(Qt.CheckState.Checked)
        self.item2.setCheckState(Qt.CheckState.Unchecked)
        self.plm.down()
        self.assertEqual(self.item.checkState(), Qt.CheckState.Unchecked)
        self.assertNotEqual(self.item2.checkState(), Qt.CheckState.Unchecked)
        self.plm.down()
        self.assertNotEqual(self.item2.checkState(), Qt.CheckState.Unchecked)
        self.item2.setCheckState(Qt.CheckState.Unchecked)
        self.plm.down()
        self.assertEqual(self.item2.checkState(), Qt.CheckState.Unchecked)
        self.assertNotEqual(self.item.checkState(), Qt.CheckState.Unchecked)
        self.item2.setCheckState(Qt.CheckState.Checked)
        self.item.setCheckState(Qt.CheckState.Checked)
        self.plm.down()
        self.assertEqual(self.item2.checkState(), Qt.CheckState.Unchecked)
        self.assertNotEqual(self.item.checkState(), Qt.CheckState.Unchecked)

    def test_up(self):
        self.item.setCheckState(Qt.CheckState.Unchecked)
        self.item2.setCheckState(Qt.CheckState.Checked)
        self.plm.up()
        self.assertEqual(self.item2.checkState(), Qt.CheckState.Unchecked)
        self.assertNotEqual(self.item.checkState(), Qt.CheckState.Unchecked)
        self.plm.up()
        self.assertEqual(self.item2.checkState(), Qt.CheckState.Unchecked)
        self.assertNotEqual(self.item.checkState(), Qt.CheckState.Unchecked)
        self.item.setCheckState(Qt.CheckState.Unchecked)
        self.plm.up()
        self.assertEqual(self.item.checkState(), Qt.CheckState.Unchecked)
        self.assertNotEqual(self.item2.checkState(), Qt.CheckState.Unchecked)
        self.item2.setCheckState(Qt.CheckState.Checked)
        self.item.setCheckState(Qt.CheckState.Checked)
        self.plm.up()
        self.assertEqual(self.item.checkState(), Qt.CheckState.Unchecked)
        self.assertNotEqual(self.item2.checkState(), Qt.CheckState.Unchecked)

    def test_check_first(self):
        self.item.setCheckState(Qt.CheckState.Unchecked)
        self.item2.setCheckState(Qt.CheckState.Unchecked)
        self.plm.check_first()
        self.assertNotEqual(self.item.checkState(), Qt.CheckState.Unchecked)
        self.assertEqual(self.item2.checkState(), Qt.CheckState.Unchecked)

    def test_check_separated(self):
        self.item.setCheckState(Qt.CheckState.Unchecked)
        self.item2.setCheckState(Qt.CheckState.Checked)
        self.plm.check_separated()
        self.assertNotEqual(self.item.checkState(), Qt.CheckState.Unchecked)
        self.assertEqual(self.item2.checkState(), Qt.CheckState.Unchecked)

    def test_check_raw(self):
        self.item.setCheckState(Qt.CheckState.Checked)
        self.item2.setCheckState(Qt.CheckState.Unchecked)
        self.plm.check_raw()
        self.assertNotEqual(self.item2.checkState(), Qt.CheckState.Unchecked)
        self.assertEqual(self.item.checkState(), Qt.CheckState.Unchecked)


if __name__ == "__main__":
    unittest.main()
