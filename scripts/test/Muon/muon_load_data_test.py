from __future__ import (absolute_import, division, print_function)

import unittest

from Muon.GUI.Common.muon_load_data import MuonLoadData


class MuonLoadDataTest(unittest.TestCase):

    def setUp(self):
        self.data = MuonLoadData()

    def assert_empty(self, load_data):
        self.assertEqual(load_data.get_parameter("run"), [])
        self.assertEqual(load_data.get_parameter("workspace"), [])
        self.assertEqual(load_data.get_parameter("filename"), [])

        self.assertEqual(load_data.num_items(), 0)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_load_data_initialized_as_empty(self):
        self.assert_empty(self.data)

    def test_data_can_be_added_correctly_via_keyword_args(self):
        data = MuonLoadData()
        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")

        self.assertEqual(data.num_items(), 1)
        self.assertEqual(data.contains_n(run=1234), 1)
        self.assertEqual(data.contains_n(workspace=[1]), 1)
        self.assertEqual(data.contains_n(filename="C:\\dir1\\file1.nxs"), 1)

    def test_that_clear_empties_the_data_of_all_entries(self):
        data = MuonLoadData()
        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1236, workspace=[3], filename="C:\\dir1\\file3.nxs")

        data.clear()

        self.assert_empty(data)

    def test_that_adding_then_removing_single_item_leaves_data_empty(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.remove_data(run=1234)
        self.assert_empty(data)

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.remove_data(workspace=[1])
        self.assert_empty(data)

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.remove_data(filename="C:\\dir1\\file1.nxs")
        self.assert_empty(data)

    def test_that_remove_method_can_remove_several_entries_at_once(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1234, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1234, workspace=[3], filename="C:\\dir1\\file3.nxs")

        data.remove_data(run=1234)

        self.assert_empty(data)

    def test_that_remove_method_applies_OR_to_multiple_keyword_arguments(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1236, workspace=[3], filename="C:\\dir1\\file3.nxs")

        data.remove_data(run=1234, workspace=[2], filename="C:\\dir1\\file3.nxs")

        self.assert_empty(data)

    def test_that_contains_is_true_when_data_contains_entry_which_matches_to_a_single_keyword_and_false_otherwise(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1234, workspace=[3], filename="C:\\dir1\\file3.nxs")

        self.assertTrue(data.contains(run=1234))
        self.assertTrue(data.contains(filename="C:\\dir1\\file1.nxs"))

        self.assertFalse(data.contains(run=9999))
        self.assertFalse(data.contains(filename="C:\\dir1\\file4.nxs"))

    def test_that_contains_is_true_when_data_contains_entries_which_match_to_at_least_one_of_multiple_keywords(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1234, workspace=[3], filename="C:\\dir1\\file3.nxs")

        # values from keywords correspond to different entries
        self.assertTrue(data.contains(run=1234, filename="C:\\dir1\\file2.nxs"))
        self.assertTrue(data.contains(run=1234, workspace=[3]))

    def test_counting_entries_with_keyword_argument_gives_correct_count(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1234, workspace=[3], filename="C:\\dir1\\file3.nxs")

        self.assertEqual(data.contains_n(run=1234), 2)
        self.assertEqual(data.contains_n(filename="C:\\dir1\\file1.nxs"), 1)
        self.assertEqual(data.contains_n(run=9999), 0)

    def test_counting_entries_applies_OR_behaviour_to_keyword_arguments(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1234, workspace=[3], filename="C:\\dir1\\file3.nxs")
        data.add_data(run=1236, workspace=[4], filename="C:\\dir1\\file4.nxs")

        self.assertEqual(data.contains_n(run=1234, workspace=[1]), 2)
        self.assertEqual(data.contains_n(run=1234, workspace=[2]), 3)
        self.assertEqual(data.contains_n(run=1234, workspace=[2], filename="C:\\dir1\\file4.nxs"), 4)

    def test_iterator_behaviour_of_data(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1236, workspace=[3], filename="C:\\dir1\\file3.nxs")

        check = [{"run": 1234, "workspace": [1], "filename": "C:\\dir1\\file1.nxs"},
                 {"run": 1235, "workspace": [2], "filename": "C:\\dir1\\file2.nxs"},
                 {"run": 1236, "workspace": [3], "filename": "C:\\dir1\\file3.nxs"}]

        for data_item in iter(data):
            self.assertEqual(data_item, check.pop(0))

    def test_that_remove_current_data_removes_the_most_recently_added_data(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1236, workspace=[3], filename="C:\\dir1\\file3.nxs")

        data.remove_current_data()

        check = [{"run": 1234, "workspace": [1], "filename": "C:\\dir1\\file1.nxs"},
                 {"run": 1235, "workspace": [2], "filename": "C:\\dir1\\file2.nxs"}]

        for data_item in iter(data):
            self.assertEqual(data_item, check.pop(0))

    def test_that_remove_last_added_data_removes_the_previous_data_item_before_the_most_recent(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1236, workspace=[3], filename="C:\\dir1\\file3.nxs")

        data.remove_last_added_data()

        check = [{"run": 1234, "workspace": [1], "filename": "C:\\dir1\\file1.nxs"},
                 {"run": 1236, "workspace": [3], "filename": "C:\\dir1\\file3.nxs"}]

        for data_item in iter(data):
            self.assertEqual(data_item, check.pop(0))


if __name__ == "__main__":
    unittest.main(verbosity=2)
