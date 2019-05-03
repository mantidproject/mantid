# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.list_selector.list_selector_view import ListSelectorView
import unittest
from Muon.GUI.Common.mock_widget import mockQapp


class TestListSelectorView(unittest.TestCase):
    def setUp(self):
        self.qapp = mockQapp()
        self.view = ListSelectorView()

    def test_add_items_adds_correct_number_of_items_to_table_view(self):
        item_list = [('property_one', True, True), ('property_two', False, False)]

        self.view.addItems(item_list)

        self.assertEqual(self.view.item_table_widget.rowCount(), 2)
        self.assertEqual(self.view.item_table_widget.item(0, 0).text(), item_list[0][0])
        self.assertEqual(self.view.item_table_widget.item(1, 0).text(), item_list[1][0])
        self.assertEqual(self.view.item_table_widget.cellWidget(1, 1).isChecked(), False)
        self.assertEqual(self.view.item_table_widget.cellWidget(0, 1).isChecked(), True)
        self.assertEqual(self.view.item_table_widget.cellWidget(0, 1).isEnabled(), True)
        self.assertEqual(self.view.item_table_widget.cellWidget(1, 1).isEnabled(), False)

    def test_that_clear_removes_all_items(self):
        item_list = [('property_one', True, True), ('property_two', False, False)]

        self.view.addItems(item_list)
        self.view.clearItems()

        self.assertEqual(self.view.item_table_widget.rowCount(), 0)

if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
