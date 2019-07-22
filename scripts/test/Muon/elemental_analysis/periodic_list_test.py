from __future__ import print_function, absolute_import


import unittest
from qtpy import QtWidgets

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table as periodic_table
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table import PeriodicList


class PeriodicListTest(GuiTest):
    def setUp(self):
        self.plist = PeriodicList()

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtWidgets.QTreeWidgetItem')
    def test_that_fill_widget_uses_defaultTableItems_when_argument_is_none(self, mock_items):
        self.plist._fill_widget(None)

        exp_num_calls = len(periodic_table._defaultTableItems)
        self.assertEqual(mock_items.call_count, exp_num_calls)
        self.assertEqual(len(self.plist.tree_items), exp_num_calls)

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtWidgets.QTreeWidgetItem')
    def test_that_fill_widget_uses_custom_items_when_argument_is_not_none(self, mock_items):
        exp_num_calls = 3
        self.plist._fill_widget(periodic_table._defaultTableItems[:exp_num_calls])

        self.assertEqual(mock_items.call_count, exp_num_calls)
        self.assertEqual(len(self.plist.tree_items), exp_num_calls)

    def test_that_tree_items_is_filled_by_QTreeWidgetItems(self):
        self.plist._fill_widget(None)

        assert all([isinstance(item, QtWidgets.QTreeWidgetItem) for item in self.plist.tree_items])

    def test_all_tree_items_have_correct_name(self):
        self.plist._fill_widget(None)

        assert all([self.plist.tree_items[i].text(0) == str(el.Z)
                    for i, el in enumerate(periodic_table._defaultTableItems)])
        assert all([self.plist.tree_items[i].text(1) == el.symbol
                    for i, el in enumerate(periodic_table._defaultTableItems)])

    def test_that_selectionChanged_emits_correct_signal(self):
        self.plist.sigSelectionChanged = mock.Mock()
        self.plist.getSelection = mock.Mock(return_value='my-return')
        self.plist._selectionChanged(mock.Mock(), mock.Mock())

        self.plist.sigSelectionChanged.emit.assert_called_with('my-return')

    def test_that_getSelection_returns_correct_number_of_elements(self):
        item1 = mock.Mock()
        item1.isSelected.return_value = True
        item2 = mock.Mock()
        item2.isSelected.return_value = True
        item3 = mock.Mock()
        item3.isSelected.return_value = False
        item4 = mock.Mock()
        item4.isSelected.return_value = True
        self.plist.tree_items = [item1, item2, item3, item4]
        ret = self.plist.getSelection()

        self.assertEqual(len(ret), 3)
        self.assertEqual(ret, [periodic_table._defaultTableItems[0],
                               periodic_table._defaultTableItems[1],
                               periodic_table._defaultTableItems[3]])

    def test_that_setSelectedElements_throws_when_given_no_elements(self):
        with self.assertRaises(IndexError):
            self.plist.setSelectedElements([])

    def test_that_setSelectedElements_calls_setSelected_correctly_when_given_elements(self):
        item1 = mock.Mock()
        item2 = mock.Mock()
        item3 = mock.Mock()
        item4 = mock.Mock()
        self.plist.tree_items = [item1, item2, item3, item4]
        self.plist.setSelectedElements(['H', 'Li'])

        assert all([item.setSelected.call_count == 1 for item in self.plist.tree_items])
        item1.setSelected.assert_called_with(True)
        item3.setSelected.assert_called_with(True)
        item2.setSelected.assert_called_with(False)
        item4.setSelected.assert_called_with(False)


if __name__ == '__main__':
    unittest.main()























