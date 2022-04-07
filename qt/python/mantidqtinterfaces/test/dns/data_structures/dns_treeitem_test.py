# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqtinterfaces.dns.data_structures.dns_treeitem import DNSTreeItem


class DNSTreeItemTest(unittest.TestCase):
    # pylint: disable=protected-access
    @classmethod
    def setUpClass(cls):
        cls.data = [
            'number', 'det_rot', 'sample_rot', 'field', 'temperature',
            'sample', 'time', 'tof_channels', 'tof_channel_width', 'filepath'
        ]
        cls.item = DNSTreeItem(cls.data, parent=None)

    def test___init__(self):
        self.assertIsInstance(self.item, DNSTreeItem)
        self.assertIsInstance(self.item, object)
        self.assertIsNone(self.item.parent_item)
        self.assertIsInstance(self.item.children_items, list)
        self.assertIsInstance(self.item, object)
        self.assertEqual(self.item._checkstate, 0)

    def test_clearChilds(self):
        self.item.children_items = [1, 2, 3]
        self.item.clearChilds()
        self.assertEqual(self.item.children_items, [])

    def test_appendChild(self):
        self.item.children_items = [1, 2, 3]
        testv = self.item.appendChild(4)
        self.assertEqual(self.item.children_items, [1, 2, 3, 4])
        self.assertEqual(testv, 4)

    def test_child(self):
        self.item.children_items = [1, 2, 3]
        testv = self.item.child(1)
        self.assertEqual(testv, 2)

    def test_removeChild(self):
        self.item.children_items = [1, 2, 3]
        self.item.removeChild(1)
        self.assertEqual(self.item.children_items, [1, 3])

    def test_childCount(self):
        self.item.children_items = [1, 2, 3]
        testv = self.item.childCount()
        self.assertEqual(testv, 3)

    def test_get_children_items(self):
        self.item.children_items = [1, 2, 3]
        testv = self.item.get_children_items()
        self.assertEqual(testv, [1, 2, 3])

    def test_columnCount(self):
        testv = self.item.columnCount()
        self.assertEqual(testv, 10)

    def test_data(self):
        testv = self.item.data()
        self.assertEqual(testv, [
            'number', 'det_rot', 'sample_rot', 'field', 'temperature',
            'sample', 'time', 'tof_channels', 'tof_channel_width', 'filepath'
        ])
        testv = self.item.data(100)
        self.assertIsNone(testv)
        testv = self.item.data(2)
        self.assertEqual(testv, 'sample_rot')

    def test_get_sample(self):
        self.item.children_items = []
        testv = self.item.get_sample()
        self.assertEqual(testv, 'sample')
        child = DNSTreeItem([1, 2, 3, 4, 5, 6])
        self.item.children_items = [child]
        testv = self.item.get_sample()
        self.assertEqual(testv, 6)

    def test_get_sample_type(self):
        child = DNSTreeItem([1, 2, 3, 4, 5, '123'])
        self.item.children_items = [child]
        testv = self.item.get_sample_type()
        self.assertEqual(testv, 'sample')
        child = DNSTreeItem([1, 2, 3, 4, 5, 'vana'])
        self.item.children_items = [child]
        testv = self.item.get_sample_type()
        self.assertEqual(testv, 'vanadium')

    def test_is_type(self):
        child = DNSTreeItem([1, 2, 3, 4, 5, 'vana'])
        self.item.children_items = [child]
        testv = self.item.is_type('vanadium')
        self.assertTrue(testv)
        testv = self.item.is_type('nicr')
        self.assertFalse(testv)

    def test_hasChildren(self):
        self.item.children_items = [1]
        self.assertTrue(self.item.hasChildren())
        self.item.children_items = []
        self.assertFalse(self.item.hasChildren())

    def test_isChecked(self):
        self.item._checkstate = 2
        self.assertEqual(self.item.isChecked(), 2)

    def test_parent(self):
        self.assertIsNone(self.item.parent())

    def test_row(self):
        self.assertEqual(self.item.row(), 0)
        child = DNSTreeItem([1, 2, 3, 4, 5, 6], parent=self.item)
        child2 = DNSTreeItem([1, 2, 3, 4, 5, 6], parent=self.item)
        self.item.children_items = [child, child2]
        testv = child2.row()
        self.assertEqual(testv, 1)

    def test_setChecked(self):
        self.item.setChecked()
        self.assertEqual(self.item._checkstate, 2)
        self.item.setChecked(0)
        self.assertEqual(self.item._checkstate, 0)

    def test_setData(self):
        self.item.setData('x', 0)
        self.assertEqual(self.item.item_data[0], 'x')
        with self.assertRaises(IndexError):
            self.item.setData('x', 100)


if __name__ == '__main__':
    unittest.main()
