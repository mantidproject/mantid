# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

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
        self.item.setCheckState(False)
        test_v = self.plm.get_checked_item_names()
        self.assertEqual(test_v, [])
        self.item.setCheckState(True)
        test_v = self.plm.get_checked_item_names()
        self.assertEqual(test_v, ["_magnetic"])

    def test_get_items(self):
        test_v = self.plm.get_items()
        self.assertEqual(test_v, [self.item, self.item2])

    def test_get_names(self):
        test_v = self.plm.get_names()
        self.assertEqual(test_v, ["_magnetic", "def_sf"])

    def test_uncheck_items(self):
        self.item.setCheckState(True)
        self.plm.uncheck_items()
        self.assertFalse(self.item.checkState())
        self.assertFalse(self.item2.checkState())

    def test_set_items(self):
        self.plm.set_items(["_magnetic", "def_sf", "hij"])
        self.assertEqual(self.plm.rowCount(), 3)
        self.assertEqual(self.plm.item(1).text(), "def_sf")

    def test_get_checked_item_numbers(self):
        self.assertEqual(self.plm.get_checked_item_numbers(), [])
        self.item.setCheckState(1)
        self.item2.setCheckState(0)
        self.assertEqual(self.plm.get_checked_item_numbers(), [0])

    def test_down(self):
        self.item.setCheckState(1)
        self.item2.setCheckState(0)
        self.plm.down()
        self.assertFalse(self.item.checkState())
        self.assertTrue(self.item2.checkState())
        self.plm.down()
        self.assertTrue(self.item2.checkState())
        self.item2.setCheckState(0)
        self.plm.down()
        self.assertFalse(self.item2.checkState())
        self.assertTrue(self.item.checkState())
        self.item2.setCheckState(1)
        self.item.setCheckState(1)
        self.plm.down()
        self.assertFalse(self.item2.checkState())
        self.assertTrue(self.item.checkState())

    def test_up(self):
        self.item.setCheckState(0)
        self.item2.setCheckState(1)
        self.plm.up()
        self.assertFalse(self.item2.checkState())
        self.assertTrue(self.item.checkState())
        self.plm.up()
        self.assertFalse(self.item2.checkState())
        self.assertTrue(self.item.checkState())
        self.item.setCheckState(0)
        self.plm.up()
        self.assertFalse(self.item.checkState())
        self.assertTrue(self.item2.checkState())
        self.item2.setCheckState(1)
        self.item.setCheckState(1)
        self.plm.up()
        self.assertFalse(self.item.checkState())
        self.assertTrue(self.item2.checkState())

    def test_check_first(self):
        self.item.setCheckState(0)
        self.item2.setCheckState(0)
        self.plm.check_first()
        self.assertTrue(self.item.checkState())
        self.assertFalse(self.item2.checkState())

    def test_check_separated(self):
        self.item.setCheckState(0)
        self.item2.setCheckState(1)
        self.plm.check_separated()
        self.assertTrue(self.item.checkState())
        self.assertFalse(self.item2.checkState())

    def test_check_raw(self):
        self.item.setCheckState(1)
        self.item2.setCheckState(0)
        self.plm.check_raw()
        self.assertTrue(self.item2.checkState())
        self.assertFalse(self.item.checkState())


if __name__ == "__main__":
    unittest.main()
