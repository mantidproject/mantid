# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_treeitem import DNSTreeItem
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_treemodel import DNSTreeModel
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import (
    get_dataset, get_first_scan_command)
from qtpy.QtCore import QAbstractItemModel, QModelIndex, Qt


class DNSTreeModelTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods

    def setUp(self):
        self.data = get_dataset()
        self.model = DNSTreeModel(data=self.data)
        self.first_scan_command = get_first_scan_command()

    def test___init__(self):
        self.assertIsInstance(self.model, QAbstractItemModel)
        self.assertIsInstance(self.model, DNSTreeModel)
        self.assertIsInstance(self.model.rootItem, DNSTreeItem)

    def test_index(self):
        test_v = self.model.index(0, 1, QModelIndex())
        self.assertIsInstance(test_v, QModelIndex)
        self.assertTrue(test_v.isValid())
        test_v = self.model.index(100, 1, self.model._scan_index_from_row(0))
        self.assertIsInstance(test_v, QModelIndex)
        self.assertFalse(test_v.isValid())
        test_v = self.model.index(0, 1, self.model._scan_index_from_row(0))
        self.assertIsInstance(test_v, QModelIndex)
        self.assertTrue(test_v.isValid())

    def test_parent(self):
        test_v = self.model.parent(QModelIndex())
        self.assertIsInstance(test_v, QModelIndex)
        self.assertFalse(test_v.isValid())
        index = self.model._scan_index_from_row(0)
        test = self.model.parent(index)
        self.assertFalse(test.isValid())

    def test_headerData(self):
        test_v = self.model.headerData(1, None, None)
        self.assertIsNone(test_v)
        test_v = self.model.headerData(1, Qt.Horizontal, Qt.DisplayRole)
        self.assertEqual(test_v, 'det_rot')

    def test_columnCount(self):
        parent = self.model._scan_index_from_row(1).parent()
        self.assertEqual(self.model.columnCount(parent), 10)
        parent = self.model._scan_index_from_row(1)
        self.assertEqual(self.model.columnCount(parent), 10)

    def test_scan_command_from_row(self):
        test_v = self.model._scan_command_from_row(0)
        self.assertEqual(test_v, self.first_scan_command)

    def test_scan_expected_points_from_row(self):
        test_v = self.model._scan_expected_points_from_row(0)
        self.assertEqual(test_v, 340)
        self.model.add_number_of_children()
        test_v = self.model._scan_expected_points_from_row(0)
        self.assertEqual(test_v, 340)

    def test_number_of_scans(self):
        self.assertEqual(self.model.number_of_scans(), 2)

    def test_scan_range(self):
        self.assertEqual(self.model._scan_range(), range(0, 2))

    def test_get_sample_type(self):
        self.assertEqual(self.model._get_sample_type('123_vanadium'), 'vana')
        self.assertEqual(self.model._get_sample_type('123_hui'), '123_hui')

    def test_get_scan_rows(self):
        testv = self.model._get_scan_rows()
        self.assertEqual(testv, [0, 1])

    def test_is_scan_complete(self):
        self.assertFalse(self.model._is_scan_complete(0))
        self.assertTrue(self.model._is_scan_complete(1))

    def test_text_in_scan(self):
        self.assertTrue(self.model.text_in_scan(0, 'scan'))
        self.assertFalse(self.model.text_in_scan(0, 'hui'))

    def test_get_last_row(self):
        self.assertEqual(self.model._get_last_row(), 1)

    def test_data(self):
        index = self.model._scan_index_from_row(0)
        role = Qt.CheckStateRole
        test_v = self.model.data(index, role)
        self.assertEqual(test_v, Qt.Unchecked)
        self.model.set_checked_from_index(index)
        test_v = self.model.data(index, role)
        self.assertEqual(test_v, Qt.Checked)
        role = Qt.DisplayRole
        test_v = self.model.data(index, role)
        self.assertEqual(test_v, self.first_scan_command)
        role = None
        self.assertIsNone(self.model.data(index, role))

    def test_flags(self):
        index = QModelIndex()
        self.assertEqual(self.model.flags(index), Qt.NoItemFlags)
        index = self.model._scan_index_from_row(0)
        test_v = self.model.flags(index)
        self.assertEqual(test_v, (Qt.ItemIsEnabled
                                  | Qt.ItemIsSelectable
                                  | Qt.ItemIsUserCheckable))

    def test_scan_index_from_row(self):
        index = self.model._scan_index_from_row(0)
        self.assertIsInstance(index, QModelIndex)
        self.assertTrue(index.isValid())

    def test_index_from_scan(self):
        scan = self.model.scan_from_row(0)
        index = self.model._index_from_scan(scan)
        self.assertIsInstance(index, QModelIndex)
        self.assertTrue(index.isValid())

    def test_item_from_index(self):
        test_v = self.model._item_from_index(self.model._scan_index_from_row(0))
        self.assertIsInstance(test_v, DNSTreeItem)

    def test_get_filename_from_index(self):
        index = self.model._scan_index_from_row(0)
        test_v = self.model.get_filename_from_index(index)
        self.assertEqual(test_v, '')
        index = self.model._index_from_row(0, index)
        test_v = self.model.get_filename_from_index(index)
        self.assertEqual(test_v, 'service_787463.d_dat')

    def test_index_from_row(self):
        index = self.model._scan_index_from_row(0)
        index = self.model._index_from_row(0, index)
        self.assertIsInstance(index, QModelIndex)
        self.assertTrue(index.isValid())

    def test_scan_from_row(self):
        item = self.model.scan_from_row(0)
        self.assertIsInstance(item, DNSTreeItem)
        self.assertEqual(item.parent(), self.model.rootItem)

    def test_rowCount(self):
        index = self.model._scan_index_from_row(0)
        test_v = self.model.rowCount(parent=QModelIndex())
        self.assertEqual(test_v, 2)  # scans
        test_v = self.model.rowCount(parent=index)
        self.assertEqual(test_v, 1)  # datafiles in first scan

    def test_get_or_create_parent_item(self):
        index = self.model._scan_index_from_row(0)
        test_v = self.model._get_or_create_parent_item(parent=None)
        self.assertEqual(self.model.rootItem, test_v)
        test_v = self.model._get_or_create_parent_item(parent=index)
        self.assertNotEqual(self.model.rootItem, test_v)

    def test_is_scan_tof(self):
        self.assertFalse(self.model.is_scan_tof(0))
        self.assertTrue(self.model.is_scan_tof(1))

    def test_get_checked(self):
        test_v = self.model.get_checked()
        self.assertEqual(test_v, [])
        self.model.set_checked_scan(1, 2)
        test_v = self.model.get_checked()
        self.assertEqual(test_v, [788058])
        test_v = self.model.get_checked(True)
        self.assertIsInstance(test_v[0], dict)
        self.model.set_checked_scan(1, 0)

    def test_check_scans_by_indexes(self):
        index = self.model._scan_index_from_row(0)
        self.model.check_scans_by_indexes([])
        self.assertEqual(self.model.get_checked(), [])
        self.model.check_scans_by_indexes([index])
        self.assertEqual(self.model.get_checked(), [787463])

    def test_check_scans_by_rows(self):
        self.model.check_scans_by_rows([])
        self.assertEqual(self.model.get_checked(), [])
        self.model.check_scans_by_rows([0])
        self.assertEqual(self.model.get_checked(), [787463])

    def test_get_complete_scan_rows(self):
        test_v = self.model.get_complete_scan_rows([0, 1])
        self.assertEqual(test_v, [1])

    def test_setData(self):
        index = self.model._scan_index_from_row(0)
        test_v = self.model.setData(index, 0)
        self.assertFalse(test_v)
        test_v = self.model.setData(index, 0, role=Qt.CheckStateRole)
        self.assertTrue(test_v)
        index = self.model._index_from_row(1, index)
        test_v = self.model.setData(index, 0)
        self.assertTrue(test_v)

    def test_set_checked_scan(self):
        self.model.set_checked_scan(0, 2)
        self.assertEqual(self.model.get_checked(), [787463])
        self.model.set_checked_scan(0, 0)
        self.assertEqual(self.model.get_checked(), [])

    def test_set_checked_from_index(self):
        index = self.model._scan_index_from_row(0)
        self.model.set_checked_from_index(index, 2)
        self.assertEqual(self.model.get_checked(), [787463])
        self.model.set_checked_from_index(index, 0)
        self.assertEqual(self.model.get_checked(), [])

    def test_item_checked(self):
        index = self.model._scan_index_from_row(0)
        # has children
        self.model.set_checked_from_index(index)
        item = self.model._item_from_index(index)
        child_index = self.model.index(0, 0, index)
        item.child(0).setChecked(0)
        self.model._item_checked(index)
        self.assertTrue(item.child(0).isChecked())
        # no children check
        item.setChecked(0)
        item.child(0).setChecked(2)
        self.model._item_checked(child_index)
        self.assertTrue(item.isChecked())
        item.child(0).setChecked(0)
        self.model._item_checked(child_index)
        self.assertFalse(item.isChecked())

    def test_uncheck_all_scans(self):
        index = self.model._scan_index_from_row(0)
        self.model.set_checked_from_index(index)
        self.assertNotEqual(self.model.get_checked(), [])
        self.model.uncheck_all_scans()
        self.assertEqual(self.model.get_checked(), [])

    def test_clear_scans(self):
        self.assertEqual(self.model.rowCount(), 2)
        self.model.clear_scans()
        self.assertEqual(self.model.rowCount(), 0)

    def test_new_scan_check(self):
        self.model._last_scan_number = self.data[0].scan_number
        self.model._last_tof = self.data[0].tof_channels
        self.model._last_tof_time = self.data[0].channel_width
        self.model._last_sample = self.data[0].sample
        self.assertFalse(self.model._new_scan_check(self.data[0]))
        self.assertTrue(self.model._new_scan_check(self.data[1]))

    def test_get_scan_text(self):
        test_v = self.model._get_scan_text(self.data[0])
        self.assertEqual(test_v[0], self.first_scan_command)

    def test_get_data_from_dns_file(self):
        test_v = self.model._get_data_from_dns_file(self.data[0])[0]
        self.assertEqual(test_v, '787463')

    def test_check_child_if_scan_is_checked(self):
        index = self.model._scan_index_from_row(0)
        scan = self.model._item_from_index(index)
        child = scan.child(0)
        scan.setChecked(0)
        self.model._check_child_if_scan_is_checked(scan, child)
        self.assertFalse(child.isChecked())
        scan.setChecked(2)
        self.model._check_child_if_scan_is_checked(scan, child)
        self.assertTrue(child.isChecked())

    def test_check_fn_range(self):
        index = self.model._scan_index_from_row(0)
        scan = self.model._item_from_index(index)
        self.assertFalse(scan.isChecked())
        self.model.check_fn_range(787463, 787463)
        self.assertTrue(scan.isChecked())

    def test_setup_model_data(self):
        self.assertEqual(self.model._last_scan_number, '14933')
        self.assertEqual(self.model._last_tof_time, 1.6)
        self.assertEqual(self.model._last_tof, 1000)
        self.assertEqual(self.model._last_sample, '4p1K_map')
        self.assertEqual(self.model.rowCount(), 2)

    def test_add_number_of_children(self):
        index = self.model._scan_index_from_row(0)
        scan = self.model._item_from_index(index)
        test_v = self.model.add_number_of_children()
        self.assertEqual(test_v, 2)
        postfix = scan.get_tree_item_data(0).split('#')[1].split('/')[0]
        self.assertEqual(postfix, '1')

    def test_get_txt(self):
        test_v = self.model.get_txt()
        self.assertEqual(test_v[0][0:10], '787463 ; -')

    def test_get_file_number_dict(self):
        test_v = self.model.get_file_number_dict()
        self.assertIsInstance(test_v, dict)
        self.assertIsInstance(test_v[788058], QModelIndex)
        self.assertTrue(test_v[788058].isValid())


if __name__ == '__main__':
    unittest.main()
