# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from SANS.sans.common.enums import RowState
from SANS.sans.common.RowEntries import RowEntries
from mantidqtinterfaces.sans_isis.gui_logic.models.basic_hint_strategy import BasicHintStrategy
from mantidqtinterfaces.sans_isis.gui_logic.models.table_model import TableModel


class TableModelTest(unittest.TestCase):
    def test_user_file_can_be_set(self):
        self._do_test_file_setting(self._user_file_wrapper, "user_file")

    def test_batch_file_can_be_set(self):
        self._do_test_file_setting(self._batch_file_wrapper, "batch_file")

    def test_that_can_get_table_index_model_for_valid_index(self):
        table_model = TableModel()
        table_index_model = RowEntries()
        table_model.replace_table_entry(0, table_index_model)
        returned_model = table_model.get_row(0)
        self.assertEqual(table_index_model, returned_model)

    def test_get_multiple_rows(self):
        row_1, row_2 = RowEntries(), RowEntries()
        instance = TableModel()
        instance.add_multiple_table_entries([row_1, row_2])
        self.assertEqual([row_1, row_2], instance.get_multiple_rows([0, 1]))

    def test_that_can_set_the_options_column_model(self):
        table_index_model = RowEntries()
        table_index_model.options.set_user_options("WavelengthMin=1, WavelengthMax=3, NotRegister2=1")

        options_dict = table_index_model.options.get_options_dict()
        self.assertEqual(len(options_dict), 2)
        self.assertEqual(options_dict["WavelengthMin"], 1.0)
        self.assertEqual(options_dict["WavelengthMax"], 3.0)

    def test_that_raises_for_missing_equal(self):
        row = RowEntries()
        row.options.set_user_options("WavelengthMin=1, WavelengthMax=3, NotRegister2")
        with self.assertRaises(ValueError):
            row.options.get_options_dict()

    def test_that_parse_string_returns_correctly(self):
        string_to_parse = "EventSlices=1-6,5-9,4:5:89 , WavelengthMax=78 , WavelengthMin=9"
        entry = RowEntries()
        entry.options.set_user_options(string_to_parse)
        expected_dict = {"EventSlices": "1-6,5-9,4:5:89", "WavelengthMax": 78.0, "WavelengthMin": 9.0}

        parsed_dict = entry.options.get_options_dict()

        self.assertEqual(expected_dict, parsed_dict)

    def test_get_number_of_rows_returns_number_of_entries(self):
        table_model = TableModel()
        table_index_model = RowEntries()
        table_model.replace_table_entry(0, table_index_model)
        table_index_model = RowEntries()
        table_model.replace_table_entry(1, table_index_model)

        number_of_rows = table_model.get_number_of_rows()

        self.assertEqual(number_of_rows, 2)

    def test_copy_paste_single_appends_to_end(self):
        table_model = TableModel()
        table_model.append_table_entry(RowEntries(sample_transmission="1"))
        table_model.append_table_entry(RowEntries(sample_transmission="2"))

        table_model.copy_rows([0])
        table_model.paste_rows([])
        self.assertEqual(3, table_model.get_number_of_rows())
        self.assertEqual(table_model.get_row(0), table_model.get_row(2))

    def test_copy_paste_overwrites_partial(self):
        table_model = TableModel()
        num_rows = 6
        for entry_i in range(num_rows):
            table_model.append_table_entry(RowEntries(sample_transmission=entry_i))

        # When a partial range of rows is selected that doesn't match the copied length
        # overwrite to match the behaviour of other spreadsheet tools
        copied_rows = [0, 1, 2]
        table_model.copy_rows(copied_rows)
        table_model.paste_rows([3, 4])

        self.assertEqual(num_rows, table_model.get_number_of_rows())
        for i in copied_rows:
            # We pasted to offset 3
            self.assertEqual(table_model.get_row(i), table_model.get_row(i + 3))

    def test_copy_paste_append_mixed_selection(self):
        table_model = TableModel()
        for entry_i in range(4):
            table_model.append_table_entry(RowEntries(sample_transmission=entry_i))

        copied_rows = [0, 1, 3]
        table_model.copy_rows(copied_rows)
        table_model.paste_rows([])

        # 4 rows + 3 pasted
        self.assertEqual(7, table_model.get_number_of_rows())
        for dest_row, original_row in enumerate(copied_rows):
            self.assertEqual(table_model.get_row(original_row), table_model.get_row(dest_row + 4))

    def test_copy_paste_append_mixed_selection_onto_mixed_selection(self):
        table_model = TableModel()
        num_elements = 5
        for entry_i in range(num_elements):
            table_model.append_table_entry(RowEntries(sample_transmission=entry_i))

        copied_indices = [0, 1, 3, 4]
        copied_rows = table_model.get_multiple_rows(copied_indices)

        table_model.copy_rows(copied_indices)
        table_model.paste_rows([0, 2, 3])

        self.assertEqual(num_elements, table_model.get_number_of_rows())
        for paste_index, original_items in enumerate(copied_rows):
            self.assertEqual(original_items, table_model.get_row(paste_index))

    def test_copy_paste_beyond_table_range(self):
        table_model = TableModel()
        num_elements = 4

        for entry_i in range(num_elements):
            table_model.append_table_entry(RowEntries(sample_transmission=entry_i))

        copied_indices = [0, 1, 2]
        table_model.copy_rows(copied_indices)
        paste_location = 3
        table_model.paste_rows([paste_location])

        # We override row 4, and should have + rows now too
        self.assertEqual(num_elements + 2, table_model.get_number_of_rows())
        for paste_index in copied_indices:
            self.assertEqual(table_model.get_row(paste_index), table_model.get_row(paste_index + paste_location))

    def test_copy_paste_overwrite(self):
        for paste_location in [0, 1]:
            table_model = TableModel()
            for entry_i in range(3):
                table_model.append_table_entry(RowEntries(sample_transmission=entry_i))

            table_model.copy_rows([2])
            table_model.paste_rows([paste_location])
            self.assertEqual(table_model.get_row(2), table_model.get_row(paste_location))

    def test_copy_paste_sorts_order(self):
        # Qt can give is the order a user selected rows in, ensure it's always in the natural order
        table_model = TableModel()
        num_rows = 5
        for entry_i in range(num_rows):
            table_model.append_table_entry(RowEntries(sample_transmission=entry_i))

        table_model.copy_rows([4, 0, 2])
        table_model.paste_rows([])

        self.assertEqual(table_model.get_row(0), table_model.get_row(0 + num_rows))
        self.assertEqual(table_model.get_row(2), table_model.get_row(1 + num_rows))
        self.assertEqual(table_model.get_row(4), table_model.get_row(2 + num_rows))

    def test_cut_paste_sorts_order(self):
        # Qt can give is the order a user selected rows in, ensure it's always in the natural order
        table_model = TableModel()
        for entry_i in range(5):
            table_model.append_table_entry(RowEntries(sample_transmission=entry_i))

        input_rows = [4, 0, 2]
        expected_rows = table_model.get_multiple_rows(sorted(input_rows))

        table_model.cut_rows(input_rows)
        table_model.paste_rows([])

        for i, row in enumerate(expected_rows):
            # 5 original rows - 3 cut rows = 2 rows remaining
            self.assertEqual(row, table_model.get_row(i + 2))

    def test_copy_paste_modify_overwrite(self):
        # Check we aren't doing a shallow copy, which cause a user to
        # see two rows get modified at once
        table_model = TableModel()
        table_model.append_table_entry(RowEntries(sample_transmission="1"))
        table_model.append_table_entry(RowEntries(sample_transmission="2"))
        table_model.copy_rows([0])
        table_model.paste_rows([1])

        row_1 = table_model.get_row(1)
        self.assertEqual(table_model.get_row(0), row_1)
        row_1.sample_transmission = 2
        self.assertNotEqual(table_model.get_row(0), row_1)

    def test_copy_paste_modify_append(self):
        table_model = TableModel()
        table_model.append_table_entry(RowEntries(sample_transmission="1"))
        table_model.copy_rows([0])
        table_model.paste_rows([])

        row_1 = table_model.get_row(1)
        self.assertEqual(table_model.get_row(0), row_1)
        row_1.sample_transmission = 2
        self.assertNotEqual(table_model.get_row(0), row_1)

    def test_when_table_is_cleared_is_left_with_one_empty_row(self):
        table_model = TableModel()
        table_model.append_table_entry(RowEntries())
        table_model.append_table_entry(RowEntries())

        self.assertEqual(2, table_model.get_number_of_rows())

        table_model.clear_table_entries()

        self.assertEqual(table_model.get_number_of_rows(), 0)
        self.assertTrue(table_model.get_row(0).is_empty())

    def test_when_last_row_is_removed_table_is_left_with_one_empty_row(self):
        table_model = TableModel()
        table_model.append_table_entry(RowEntries())
        table_model.append_table_entry(RowEntries())

        self.assertEqual(2, table_model.get_number_of_rows())

        table_model.remove_table_entries([0, 1])

        self.assertEqual(table_model.get_number_of_rows(), 0)
        self.assertTrue(table_model.get_row(0).is_empty())

    def test_that_OptionsColumnModel_get_hint_strategy(self):
        hint_strategy = TableModel.get_options_hint_strategy()
        expected_hint_strategy = BasicHintStrategy(
            {
                "WavelengthMin": "The min value of the wavelength when converting from TOF.",
                "WavelengthMax": "The max value of the wavelength when converting from TOF.",
                "PhiMin": "The min angle of the detector to accept." " Anti-clockwise from horizontal.",
                "PhiMax": "The max angle of the detector to accept." " Anti-clockwise from horizontal.",
                "UseMirror": "True or False. Whether or not to accept phi angle" " in opposing quadrant",
                "MergeScale": "The scale applied to the HAB when merging",
                "MergeShift": "The shift applied to the HAB when merging",
                "EventSlices": "The event slices to reduce."
                " The format is the same as for the event slices"
                " box in settings, however if a comma separated list is given "
                "it must be enclosed in quotes",
            }
        )

        self.assertEqual(expected_hint_strategy, hint_strategy)

    def test_that_set_row_to_error_sets_row_to_error_and_tool_tip(self):
        table_model = TableModel()
        table_index_model = RowEntries()

        table_model.replace_table_entry(0, table_index_model)

        row = 0
        tool_tip = "There was an error"

        table_model.set_row_to_error(row, tool_tip)
        row_entry = table_model.get_row(0)

        self.assertTrue(RowState.ERROR, row_entry.state)
        self.assertEqual(tool_tip, row_entry.tool_tip)

    def test_that_get_non_empty_rows_returns_non_empty_rows(self):
        table_model = TableModel()
        empty_row = RowEntries()

        table_model.append_table_entry(empty_row)
        table_model.append_table_entry(RowEntries(sample_scatter=123))
        table_model.append_table_entry(empty_row)
        table_model.append_table_entry(RowEntries(sample_direct=345))
        table_model.append_table_entry(empty_row)

        self.assertEqual(2, len(table_model.get_non_empty_rows()))

        for i in table_model.get_non_empty_rows():
            self.assertFalse(i.is_empty())

        self.assertEqual(5, table_model.get_number_of_rows())

    def test_default_obj_correctly_tracked(self):
        # We need to make sure there is always 1 object in table model, but not to get it mixed with user input
        obj = TableModel()
        self.assertEqual(0, obj.get_number_of_rows())

        obj.clear_table_entries()
        self.assertEqual(0, obj.get_number_of_rows())

        obj.append_table_entry(RowEntries())
        self.assertEqual(1, obj.get_number_of_rows())
        obj.append_table_entry(RowEntries())
        self.assertEqual(2, obj.get_number_of_rows())

        obj.clear_table_entries()
        self.assertEqual(0, obj.get_number_of_rows())
        obj.append_table_entry(RowEntries())
        self.assertEqual(1, obj.get_number_of_rows())

    def test_get_num_rows(self):
        obj = TableModel()
        self.assertEqual(0, obj.get_number_of_rows())

        obj.append_table_entry(RowEntries())
        self.assertEqual(1, obj.get_number_of_rows())

        obj.remove_table_entries([0])
        self.assertEqual(0, obj.get_number_of_rows())

    def test_inserting_row_at_pos(self):
        model = TableModel()

        expected_order = [RowEntries() for _ in range(3)]
        model.replace_table_entry(row_index=0, row_entry=expected_order[0])
        model.replace_table_entry(row_index=1, row_entry=expected_order[2])

        self.assertTrue(2, model.get_number_of_rows())

        model.insert_row_at(1, expected_order[1])
        self.assertTrue(3, model.get_number_of_rows())
        self.assertEqual(expected_order, model.get_all_rows())

    def test_insert_row_at_multiple_beyond(self):
        model = TableModel()

        expected = [RowEntries(sample_scatter=i) for i in range(2)]
        model.replace_table_entry(row_index=0, row_entry=expected[0])

        self.assertTrue(1, model.get_number_of_rows())

        model.insert_row_at(4, expected[1])

        # Remember that the number of rows is counted from 1, but we use 0 based indexing
        self.assertTrue(5, model.get_number_of_rows())
        self.assertEqual(expected, model.get_multiple_rows([0, 4]))
        self.assertTrue(all(i.is_empty() for i in model.get_multiple_rows([1, 2, 3])))

    def _do_test_file_setting(self, func, prop):
        # Test that can set to empty string
        table_model = TableModel()
        try:
            setattr(table_model, prop, "")
            has_raised = False
        except:
            has_raised = True
        self.assertFalse(has_raised)

        # Test that can be set to valid value
        setattr(table_model, prop, __file__)
        self.assertEqual(getattr(table_model, prop), __file__)

    @staticmethod
    def _batch_file_wrapper(value):
        table_model = TableModel()
        table_model.batch_file = value

    @staticmethod
    def _user_file_wrapper(value):
        table_model = TableModel()
        table_model.user_file = value


if __name__ == "__main__":
    unittest.main()
