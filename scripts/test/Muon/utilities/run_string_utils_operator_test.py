# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import Muon.GUI.Common.utilities.run_string_utils as utils
import unittest


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
        increment = 1
        decremented_run_list = [2, 3, 4, 5, 1]
        self.assertEqual(utils.decrement_run_list(run_list, increment), decremented_run_list)

    def test_can_correctly_increment_run_list_by_five(self):
        run_list = [1, 2, 3, 4, 5]
        increment = 5
        incremented_run_list = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        self.assertEqual(utils.increment_run_list(run_list, increment), incremented_run_list)

    def test_can_correctly_decrement_run_list_by_five(self):
        run_list = [5, 6, 7, 8, 9, 10]
        increment = 5
        decremented_run_list = [5, 6, 7, 8, 9, 10, 4, 3, 2, 1, 0]
        self.assertEqual(utils.decrement_run_list(run_list, increment), decremented_run_list)

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
