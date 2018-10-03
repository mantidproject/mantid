# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from sans.gui_logic.models.table_model import (TableModel, TableIndexModel, OptionsColumnModel)
from sans.gui_logic.models.basic_hint_strategy import BasicHintStrategy


class TableModelTest(unittest.TestCase):
    def test_user_file_can_be_set(self):
        self._do_test_file_setting(self._user_file_wrapper, "user_file")

    def test_batch_file_can_be_set(self):
        self._do_test_file_setting(self._batch_file_wrapper, "batch_file")

    def test_that_raises_if_table_index_does_not_exist(self):
        table_model = TableModel()
        row_entry = [''] * 16
        table_index_model = TableIndexModel(*row_entry)
        table_model.add_table_entry(0, table_index_model)
        self.assertRaises(IndexError, table_model.get_table_entry, 1)

    def test_that_can_get_table_index_model_for_valid_index(self):
        table_model = TableModel()
        row_entry = [''] * 16
        table_index_model = TableIndexModel(*row_entry)
        table_model.add_table_entry(0, table_index_model)
        returned_model = table_model.get_table_entry(0)
        self.assertTrue(returned_model.sample_scatter == '')

    def test_that_can_set_the_options_column_model(self):
        table_index_model = TableIndexModel(0, "", "", "", "", "", "",
                                            "", "", "", "", "", "", "", "",
                                            "WavelengthMin=1, WavelengthMax=3, NotRegister2=1")
        options_column_model = table_index_model.options_column_model
        options = options_column_model.get_options()
        self.assertTrue(len(options) == 2)
        self.assertTrue(options["WavelengthMin"] == 1.)
        self.assertTrue(options["WavelengthMax"] == 3.)

    def test_that_raises_for_missing_equal(self):
        args = [0, "", "", "", "", "", "", "", "", "", "", "", "", "", "",
                "WavelengthMin=1, WavelengthMax=3, NotRegister2"]
        self.assertRaises(ValueError,  TableIndexModel, *args)

    def test_that_querying_nonexistent_row_index_raises_IndexError_exception(self):
        table_model = TableModel()
        args = [0]
        self.assertRaises(IndexError, table_model.get_row_user_file, *args)

    def test_that_can_retrieve_user_file_from_table_index_model(self):
        table_model = TableModel()
        table_index_model = TableIndexModel(2, "", "", "", "", "", "",
                                            "", "", "", "", "", "", "User_file_name")
        table_model.add_table_entry(2, table_index_model)
        user_file = table_model.get_row_user_file(0)
        self.assertEqual(user_file,"User_file_name")

    def test_that_can_retrieve_sample_thickness_from_table_index_model(self):
        sample_thickness = '8.0'
        table_model = TableModel()
        table_index_model = TableIndexModel("", "", "", "", "", "",
                                            "", "", "", "", "", "", sample_thickness=sample_thickness)
        table_model.add_table_entry(2, table_index_model)
        row_entry = table_model.get_table_entry(0)
        self.assertEqual(row_entry.sample_thickness, sample_thickness)

    def test_that_parse_string_returns_correctly(self):
        string_to_parse = 'EventSlices=1-6,5-9,4:5:89 , WavelengthMax=78 , WavelengthMin=9'
        expected_dict = {'EventSlices':'1-6,5-9,4:5:89', 'WavelengthMax':'78', 'WavelengthMin':'9'}

        parsed_dict = OptionsColumnModel._parse_string(string_to_parse)

        self.assertEqual(parsed_dict, expected_dict)

    def test_get_number_of_rows_returns_number_of_entries(self):
        table_model = TableModel()
        table_index_model = TableIndexModel(0, "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(0, table_index_model)
        table_index_model = TableIndexModel(1, "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(1, table_index_model)

        number_of_rows = table_model.get_number_of_rows()

        self.assertEqual(number_of_rows, 2)

    def test_when_table_is_cleared_is_left_with_one_empty_row(self):
        table_model = TableModel()
        table_index_model = TableIndexModel(0, "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(0, table_index_model)
        table_index_model = TableIndexModel(1, "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(1, table_index_model)
        empty_row = table_model.create_empty_row()

        table_model.clear_table_entries()

        self.assertEqual(table_model.get_number_of_rows(), 1)
        self.assertEqual(table_model.get_table_entry(0), empty_row)

    def test_when_last_row_is_removed_table_is_left_with_one_empty_row(self):
        table_model = TableModel()
        table_index_model = TableIndexModel(0, "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(0, table_index_model)
        table_index_model = TableIndexModel(1, "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(1, table_index_model)
        empty_row = table_model.create_empty_row()

        table_model.remove_table_entries([0, 1])

        self.assertEqual(table_model.get_number_of_rows(), 1)
        self.assertEqual(table_model.get_table_entry(0), empty_row)

    def test_that_OptionsColumnModel_get_permissable_properties_returns_correct_properties(self):
        permissable_properties = OptionsColumnModel._get_permissible_properties()

        self.assertEqual(permissable_properties, {"WavelengthMin":float, "WavelengthMax": float, "EventSlices": str})

    def test_that_OptionsColumnModel_get_hint_strategy(self):
        hint_strategy = OptionsColumnModel.get_hint_strategy()
        expected_hint_strategy = BasicHintStrategy({"WavelengthMin": 'The min value of the wavelength when converting from TOF.',
                                  "WavelengthMax": 'The max value of the wavelength when converting from TOF.',
                                  "EventSlices": 'The event slices to reduce.'
                                  ' The format is the same as for the event slices'
                                  ' box in settings, however if a comma separated list is given '
                                  'it must be enclosed in quotes'})

        self.assertEqual(expected_hint_strategy, hint_strategy)

    def _do_test_file_setting(self, func, prop):
        # Test that can set to empty string
        table_model = TableModel()
        try:
            setattr(table_model, prop, "")
            has_raised = False
        except:  # noqa
            has_raised = True
        self.assertFalse(has_raised)

        # Test that can be set to valid value
        setattr(table_model, prop, __file__)
        self.assertTrue(getattr(table_model, prop) == __file__)

    @staticmethod
    def _batch_file_wrapper(value):
        table_model = TableModel()
        table_model.batch_file = value

    @staticmethod
    def _user_file_wrapper(value):
        table_model = TableModel()
        table_model.user_file = value


if __name__ == '__main__':
    unittest.main()
