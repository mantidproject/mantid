# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantidqtinterfaces.Muon.GUI.Common.utilities.run_string_utils as utils
import unittest


class RunStringUtilsListToStringTest(unittest.TestCase):
    def test_run_list_to_string_for_consecutive_runs(self):
        run_list = [1, 2, 3, 4, 5]
        run_string = utils.run_list_to_string(run_list)
        self.assertEqual(run_string, "1-5")

    def test_run_list_to_string_orders_run_numbers(self):
        run_list = [5, 3, 2, 4, 1]
        run_string = utils.run_list_to_string(run_list)
        self.assertEqual(run_string, "1-5")

    def test_run_list_to_string_removes_negative_runs(self):
        run_list = [-1, 1, 2, -2, 3, 4, 5, -3]
        run_string = utils.run_list_to_string(run_list)
        self.assertEqual(run_string, "1-5")

    def test_run_list_to_string_removes_duplicated_runs(self):
        run_list = [1, 1, 2, 2, 3, 4, 5, 3]
        run_string = utils.run_list_to_string(run_list)
        self.assertEqual(run_string, "1-5")

    def test_run_list_to_string_handles_non_consecutive_runs(self):
        run_lists = [[1, 3, 4, 5], [0, 1, 2, 3, 8, 9, 199, 200]]
        run_strings = ["1,3-5", "0-3,8-9,199-200"]
        for i in range(len(run_lists)):
            self.assertEqual(utils.run_list_to_string(run_lists[i]), run_strings[i])

    def test_run_list_to_string_orders_non_consecutive_runs(self):
        run_list = [50, 49, 48, 3, 2, 1]
        run_string = utils.run_list_to_string(run_list)
        self.assertEqual(run_string, "1-3,48-50")

    def test_run_list_to_string_doesnt_throw_if_more_than_100_runs(self):
        run_list = [i for i in range(150)]
        utils.run_list_to_string(run_list)


class RunStringUtilsStringToListTest(unittest.TestCase):
    # Validation of the run_string

    def test_validate_run_string_returns_true_for_valid_strings(self):
        valid_strings = ["", "1", "1,2,3,4,5", "5,4,3,2,1", "1-10", "1,2,3,4,5-10"]
        for valid_string in valid_strings:
            self.assertEqual(utils.validate_run_string(valid_string), True)

    def test_validate_run_string_returns_false_for_delimiter_typos(self):
        invalid_strings = [",,,", ",1", ",1,2,3"]
        for invalid_string in invalid_strings:
            self.assertEqual(utils.validate_run_string(invalid_string), False)

    def test_validate_run_string_returns_false_for_range_separator_typos(self):
        invalid_strings = ["---", "-1", "-1,2,3", "1,-4", "1-,4"]
        for invalid_string in invalid_strings:
            self.assertEqual(utils.validate_run_string(invalid_string), False)

    def test_validate_run_string_returns_false_for_non_run_strings(self):
        invalid_strings = ["1-5a", "abc", "a-z", "a,b", "!!", "\\", "@##"]
        for invalid_string in invalid_strings:
            self.assertEqual(utils.validate_run_string(invalid_string), False)

    def test_run_string_to_list_correct_for_consecutive_runs(self):
        run_strings = ["1-5", "1,2,3,4,5"]
        for run_string in run_strings:
            run_list = utils.run_string_to_list(run_string)
            self.assertEqual(run_list, [1, 2, 3, 4, 5])

    def test_run_string_to_list_sorts_list_ascending(self):
        run_strings = ["5,4,3,2,1", "4,5,1-3"]
        for run_string in run_strings:
            run_list = utils.run_string_to_list(run_string)
            self.assertEqual(run_list, [1, 2, 3, 4, 5])

    def test_run_string_to_list_removes_duplicates(self):
        run_strings = ["5,5,5,5,4,3,2,1,1", "1,2,4,5,1-3,1-5,1-4"]
        for run_string in run_strings:
            run_list = utils.run_string_to_list(run_string)
            self.assertEqual(run_list, [1, 2, 3, 4, 5])

    def test_run_string_to_list_throws_for_incorrectly_placed_range_separator(self):
        run_strings = ["-1,2,3"]
        for run_string in run_strings:
            with self.assertRaises(IndexError) as error:
                utils.run_string_to_list(run_string)
            self.assertTrue(run_string + " is not a valid run string" in str(error.exception))

    def test_run_string_to_list_throws_for_incorrectly_placed_delimiter(self):
        run_strings = [",1,2,3"]
        for run_string in run_strings:
            with self.assertRaises(IndexError) as context:
                utils.run_string_to_list(run_string)
            self.assertTrue(run_string + " is not a valid run string" in str(context.exception))

    def test_run_string_to_list_handles_non_consecutive_runs(self):
        run_lists = [[1, 3, 4, 5], [0, 1, 2, 3, 8, 9, 199, 200]]
        run_strings = ["1,3-5", "0-3,8-9,199-200"]
        for i in range(len(run_lists)):
            self.assertEqual(utils.run_string_to_list(run_strings[i]), run_lists[i])

    def test_run_string_to_list_orders_non_consecutive_runs(self):
        run_string = "1-3,48-50"
        run_list = [1, 2, 3, 48, 49, 50]
        self.assertEqual(utils.run_string_to_list(run_string), run_list)

    def test_run_string_to_list_allows_large_number_runs(self):
        run_string = "1-1001"
        expected_list = list(range(1, 1002))
        self.assertEqual(utils.run_string_to_list(run_string), expected_list)

    def test_run_string_to_list_allows_trailing_comma_in_short_string(self):
        run_string = "1,2,3,10-15,"
        expected_list = [1, 2, 3, 10, 11, 12, 13, 14, 15]
        self.assertEqual(utils.run_string_to_list(run_string), expected_list)

    def test_run_string_to_list_allows_trailing_dash_in_short_string(self):
        run_string = "1,2,3,10-15-"
        expected_list = [1, 2, 3, 10, 11, 12, 13, 14, 15]
        self.assertEqual(utils.run_string_to_list(run_string), expected_list)

    def test_run_string_to_list_allows_trailing_comma_in_long_string(self):
        run_string = "1,2,3,10-10010,"
        expected_list = [1, 2, 3] + list(range(10, 10011))
        self.assertEqual(utils.run_string_to_list(run_string), expected_list)

    def test_run_string_to_list_allows_trailing_dash_in_long_string(self):
        run_string = "1,2,3,10-10010-"
        expected_list = [1, 2, 3] + list(range(10, 10011))
        self.assertEqual(utils.run_string_to_list(run_string), expected_list)

    def test_run_string_to_list_allows_trailing_dash_and_space_in_long_string(self):
        run_string = "1,2,3,10-10010- "
        expected_list = [1, 2, 3] + list(range(10, 10011))
        self.assertEqual(utils.run_string_to_list(run_string), expected_list)

    def test_run_string_allows_incomplete_upper_range(self):
        run_string = "62260-66"
        run_list = [62260, 62261, 62262, 62263, 62264, 62265, 62266]
        self.assertEqual(utils.run_string_to_list(run_string), run_list)

    def test_run_string_allows_trailing_comma(self):
        run_string = "5,4,3,2,1,"
        run_list = utils.run_string_to_list(run_string)
        self.assertEqual(run_list, [1, 2, 3, 4, 5])

    def test_run_string_allows_trailing_comma_and_space(self):
        run_string = "5,4,3,2,1, "
        run_list = utils.run_string_to_list(run_string)
        self.assertEqual(run_list, [1, 2, 3, 4, 5])

    def test_run_string_removes_whitespace(self):
        run_string = "1 -3,48- 50"
        run_list = [1, 2, 3, 48, 49, 50]
        self.assertEqual(utils.run_string_to_list(run_string), run_list)

    def test_run_string_allows_range_over_decade(self):
        run_string = "62268-2"
        run_list = [62268, 62269, 62270, 62271, 62272]
        self.assertEqual(utils.run_string_to_list(run_string), run_list)


if __name__ == "__main__":
    unittest.main()
