import Muon.GUI.Common.run_string_utils as utils
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

    def test_run_list_to_string_throws_if_more_than_100_runs(self):
        run_list = [i for i in range(150)]
        with self.assertRaises(IndexError) as context:
            utils.run_list_to_string(run_list)
            self.assertTrue("Too many runs (150) must be <100" in context.exception)


class RunStringUtilsStringToListTest(unittest.TestCase):

    # Validation of the run_string

    def test_validate_run_string_returns_true_for_valid_strings(self):
        valid_strings = ["", "1", "1,2,3,4,5", "5,4,3,2,1", "1-10", "1,2,3,4,5-10"]
        for valid_string in valid_strings:
            self.assertEqual(utils.validate_run_string(valid_string), True)

    def test_validate_run_string_returns_false_for_delimiter_typos(self):
        invalid_strings = [",", ",,,", ",1", "1,", ",1,2,3", "1,2,3,"]
        for invalid_string in invalid_strings:
            self.assertEqual(utils.validate_run_string(invalid_string), False)

    def test_validate_run_string_returns_false_for_range_separator_typos(self):
        invalid_strings = ["-", "---", "-1", "1-", "-1,2,3", "1,2,3-", "1,-4", "1-,4"]
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
        run_strings = ["-1,2,3", "1,2,3-"]
        for run_string in run_strings:
            with self.assertRaises(IndexError) as context:
                utils.run_string_to_list(run_string)
                self.assertTrue(run_string + " is not a valid run string" in context.exception)

    def test_run_string_to_list_throws_for_incorrectly_placed_delimiter(self):
        run_strings = [",1,2,3", "1,2,3,"]
        for run_string in run_strings:
            with self.assertRaises(IndexError) as context:
                utils.run_string_to_list(run_string)
                self.assertTrue(run_string + " is not a valid run string" in context.exception)

    def test_run_string_to_list_handles_non_consecutive_runs(self):
        run_lists = [[1, 3, 4, 5], [0, 1, 2, 3, 8, 9, 199, 200]]
        run_strings = ["1,3-5", "0-3,8-9,199-200"]
        for i in range(len(run_lists)):
            self.assertEqual(utils.run_string_to_list(run_strings[i]), run_lists[i])

    def test_run_string_to_list_orders_non_consecutive_runs(self):
        run_string = "1-3,48-50"
        run_list = [1, 2, 3, 48, 49, 50]
        self.assertEqual(utils.run_string_to_list(run_string), run_list)


class RunStringUtilsIncrementAndDecrementRunLists(unittest.TestCase):

    def test_cannot_increment_or_decrement_by_zero_or_negative_numbers(self):
        run_list = [1, 2, 3, 4, 5]
        self.assertEqual(utils.increment_run_list(run_list, 0), run_list)
        self.assertEqual(utils.increment_run_list(run_list, -1), run_list)
        self.assertEqual(utils.decrement_run_list(run_list, 0), run_list)
        self.assertEqual(utils.decrement_run_list(run_list, -1), run_list)

    def test_can_correctly_increment_run_list_by_one(self):
        run_list = [1, 2, 3, 4, 5]
        increment = 1
        incremented_run_list = [1, 2, 3, 4, 5, 6]
        self.assertEqual(utils.increment_run_list(run_list, increment), incremented_run_list)

    def test_can_correctly_decrement_run_list_by_one(self):
        run_list = [2, 3, 4, 5]
        decrement = 1
        decremented_run_list = [2, 3, 4, 5, 1]
        self.assertEqual(utils.decrement_run_list(run_list, decrement), decremented_run_list)

    def test_can_correctly_increment_run_list_by_five(self):
        run_list = [1, 2, 3, 4, 5]
        increment = 5
        incremented_run_list = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        self.assertEqual(utils.increment_run_list(run_list, increment), incremented_run_list)

    def test_can_correctly_decrement_run_list_by_five(self):
        run_list = [5, 6, 7, 8, 9, 10]
        decrement = 5
        decremented_run_list = [5, 6, 7, 8, 9, 10, 4, 3, 2, 1, 0]
        self.assertEqual(utils.decrement_run_list(run_list, decrement), decremented_run_list)

    def test_cannot_decrement_list_below_zero(self):
        run_list = [0, 1, 2]
        increment = 1
        self.assertEqual(utils.decrement_run_list(run_list, increment), run_list)


class RunStringUtilsIncrementAndDecrementRunStrings(unittest.TestCase):

    def test_cannot_increment_or_decrement_run_string_by_zero_or_negative_numbers(self):
        run_string = "1-5"
        self.assertEqual(utils.increment_run_string(run_string, 0), run_string)
        self.assertEqual(utils.increment_run_string(run_string, -1), run_string)
        self.assertEqual(utils.decrement_run_string(run_string, 0), run_string)
        self.assertEqual(utils.decrement_run_string(run_string, -1), run_string)

    def test_can_correctly_increment_run_string_by_one(self):
        run_string = "1-5"
        increment = 1
        incremented_run_string = "1-6"
        self.assertEqual(utils.increment_run_string(run_string, increment), incremented_run_string)

    def test_can_correctly_decrement_run_string_by_one(self):
        run_string = "2-5"
        increment = 1
        decremented_run_string = "1-5"
        self.assertEqual(utils.decrement_run_string(run_string, increment), decremented_run_string)

    def test_can_correctly_increment_run_string_by_five(self):
        run_string = "1-5"
        increment = 5
        incremented_run_string = "1-10"
        self.assertEqual(utils.increment_run_string(run_string, increment), incremented_run_string)

    def test_can_correctly_decrement_run_string_by_five(self):
        run_string = "5-10"
        increment = 5
        decremented_run_string = "0-10"
        self.assertEqual(utils.decrement_run_string(run_string, increment), decremented_run_string)

    def test_cannot_decrement_list_below_zero(self):
        run_string = "0-2"
        increment = 1
        decremented_run_string = utils.decrement_run_string(run_string, increment)

        self.assertEqual(decremented_run_string, run_string)


if __name__ == '__main__':
    unittest.main()
