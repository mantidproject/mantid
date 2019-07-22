# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import print_function, absolute_import

import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest

import Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table as periodic_table
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table import PeriodicTableItem, PeriodicCombo


class PeriodicComboTest(GuiTest):
    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtWidgets.QComboBox.insertItem')
    def test_that_init_defaults_to_all_elements_in_periodic_table(self, mock_insert_item):
        PeriodicCombo()
        self.assertEqual(mock_insert_item.call_count, 109)
        mock_insert_item.assert_called_with(108, 'Mt (109) - meitnerium')

        PeriodicCombo(detailed=False)
        self.assertEqual(mock_insert_item.call_count, 109+109)
        mock_insert_item.assert_called_with(108, 'Mt (109)')

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtWidgets.QComboBox.insertItem')
    def test_that_init_accepts_new_list_elements(self, mock_insert_item):
        new_elements = [PeriodicTableItem("H", 1, 1, 1, "hydrogen", 1.00800, "diatomic nonmetal"),
                        PeriodicTableItem("He", 2, 18, 1, "helium", 4.0030, "noble gas"),
                        PeriodicTableItem("Li", 3, 1, 2, "lithium", 6.94000, "alkali metal")]
        PeriodicCombo(elements=new_elements, detailed=True)
        self.assertEqual(mock_insert_item.call_count, 3)
        mock_insert_item.assert_called_with(2, 'Li (3) - lithium')

        PeriodicCombo(elements=new_elements, detailed=False)
        self.assertEqual(mock_insert_item.call_count, 3+3)
        mock_insert_item.assert_called_with(2, 'Li (3)')

    def test_that_selection_changed_signal_sent(self):
        combo = PeriodicCombo()
        combo.sigSelectionChanged = mock.Mock()
        combo._selectionChanged(1)

        combo.sigSelectionChanged.emit.assert_called_with(periodic_table._defaultTableItems[1])

    def test_that_selected_item_is_correct(self):
        combo = PeriodicCombo()
        combo.currentIndex = mock.Mock(return_value=1)

        self.assertEqual(combo.getSelection(), periodic_table._defaultTableItems[1])

    def test_that_setSelection_sets_correct_selection_if_given_symbol(self):
        combo = PeriodicCombo()
        combo.setCurrentIndex = mock.Mock()
        combo.setSelection('H')

        combo.setCurrentIndex.assert_called_with(0)

    def test_that_setSelection_sets_correct_selection_if_given_PeriodicTableItem(self):
        combo = PeriodicCombo()
        combo.setCurrentIndex = mock.Mock()
        combo.setSelection(PeriodicTableItem("H", 1, 1, 1, "hydrogen", 1.00800, "diatomic nonmetal"))

        combo.setCurrentIndex.assert_called_with(0)

    def test_that_setSelection_throws_if_given_bad_symbol(self):
        combo = PeriodicCombo()
        with self.assertRaises(ValueError):
            combo.setSelection('I-dont-exist')


if __name__ == '__main__':
    unittest.main()
