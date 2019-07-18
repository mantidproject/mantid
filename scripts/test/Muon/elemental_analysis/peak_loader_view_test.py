# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

import unittest
import matplotlib
from qtpy.QtGui import QCloseEvent

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest

from Muon.GUI.Common.checkbox import Checkbox
from Muon.GUI.ElementalAnalysis.PeriodicTable.PeakSelector.peak_selector_view import PeakSelectorView


class PeakSelectorViewTest(GuiTest):

    def setUp(self):
        self.element = 'He'
        self.element_data = {
            "Z": 2,
            "A": 4.003,
            "Primary": {
                "K(2->1)": 8.22
            },
            "Secondary": {
                "K(3->1)": 9.74,
                "K(4->1)": 10.28,
                "K(5->1)": 10.48
            }
        }
        self.view = PeakSelectorView(self.element_data, self.element)

    def tearDown(self):
        self.view = None

    def test_init(self):
        self.assertEqual(self.view.windowTitle(), self.element)

    def test_get_checked_returns_primary_only_on_set_up(self):
        new_data = self.view.get_checked()
        self.assertEqual(new_data, self.element_data['Primary'])

    def test_update_new_data_sets_new_data_to_only_the_primary(self):
        self.view.new_data = self.element_data['Secondary']
        self.view.update_new_data(self.element_data)
        self.assertEqual(self.view.new_data.keys(), self.element_data['Primary'].keys())

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.PeakSelector.peak_selector_view.Checkbox')
    def test_setup_checkboxes_sets_check_to_correct_bool(self, mock_Checkbox):
        self.view.list.addWidget = mock.Mock()
        checkbox = self.view._setup_checkbox("{}: {}".format("K(2->1)", 8.22), True)
        checkbox.setChecked.assert_called_with(True)
        checkbox = self.view._setup_checkbox("{}: {}".format("K(2->1)", 8.22), False)
        checkbox.setChecked.assert_called_with(False)

    def test_create_checkbox_list_returns_list_of_checkboxes(self):
        heading = 'Primary'
        checkboxes = self.view._create_checkbox_list(heading, self.element_data[heading])
        self.assertIsInstance(checkboxes, list)
        self.assertIsInstance(checkboxes[0], Checkbox)

    def test_parse_checkbox_name_splits_string(self):
        name = 'type: value'
        peak_type, value = self.view._parse_checkbox_name(name)
        self.assertEqual(peak_type, 'type')
        self.assertEqual(value, 'value')

    def test_parse_checkbox_name_does_not_convert_value_to_float(self):
        name = 'type: 1.0'
        peak_type, value = self.view._parse_checkbox_name(name)
        self.assertEqual(value, '1.0')

    def test_remove_value_from_new_data_removes_existing_key_from_new_data(self):
        self.view.new_data['type'] = 'value'
        checkbox = Checkbox('type: value')
        self.view._remove_value_from_new_data(checkbox)
        self.assertIs('type' in self.view.new_data.keys(), False)

    def test_remove_value_from_new_data_raises_KeyError_is_type_not_in_new_data(self):
        checkbox = Checkbox('type: value')
        self.assertRaises(KeyError, self.view._remove_value_from_new_data, checkbox)

    def test_add_value_to_new_data(self):
        self.new_data = {}
        checkbox = Checkbox('type: value')
        self.view._add_value_to_new_data(checkbox)
        self.assertIs('type' in self.view.new_data.keys(), True)


if __name__ == '__main__':
    unittest.main()
