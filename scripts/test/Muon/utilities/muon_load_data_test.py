# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat import mock
from Muon.GUI.Common.muon_load_data import MuonLoadData


class MuonLoadDataTest(unittest.TestCase):

    def setUp(self):
        self.muon_load_data = MuonLoadData()
        self.workspace = mock.MagicMock()
        self.workspace_last = mock.MagicMock()
        self.muon_load_data.add_data(run=1, workspace=mock.MagicMock(), filename='path to file')
        self.muon_load_data.add_data(run=2, workspace=self.workspace, filename='path to file')
        self.muon_load_data.add_data(run=3, workspace=mock.MagicMock(), filename='matching path')
        self.muon_load_data.add_data()
        self.muon_load_data.add_data(run=4, workspace=self.workspace_last, filename='path to file')

    def assert_empty(self, load_data):
        self.assertEqual(load_data.get_parameter("run"), [])
        self.assertEqual(load_data.get_parameter("workspace"), [])
        self.assertEqual(load_data.get_parameter("filename"), [])

        self.assertEqual(load_data.num_items(), 0)

    def assert_unchanged(self, load_data):
        self.assertEqual(load_data.get_parameter("run"), [1234, 1235, 1236])
        self.assertEqual(load_data.get_parameter("workspace"), [[1], [2], [3]])
        self.assertEqual(load_data.get_parameter("filename"), ["C:\\dir1\\file1.nxs", "C:\\dir1\\file2.nxs", "C:\\dir1\\file3.nxs"])

        self.assertEqual(load_data.num_items(), 3)

    def populate_loaded_data(self):
        data = MuonLoadData()
        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs", instrument='EMU')
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs", instrument='EMU')
        data.add_data(run=1236, workspace=[3], filename="C:\\dir1\\file3.nxs", instrument='EMU')
        return data

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_load_data_initialized_as_empty(self):
        data = MuonLoadData()
        self.assert_empty(data)

    def test_data_can_be_added_correctly_via_keyword_args(self):
        data = MuonLoadData()
        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")

        self.assertEqual(data.num_items(), 1)
        self.assertEqual(data.contains_n(run=1234), 1)
        self.assertEqual(data.contains_n(workspace=[1]), 1)
        self.assertEqual(data.contains_n(filename="C:\\dir1\\file1.nxs"), 1)

    def test_that_clear_empties_the_data_of_all_entries(self):
        data = self.populate_loaded_data()
        self.assertEqual(data.num_items(), 3)

        data.clear()

        self.assert_empty(data)

    def test_that_adding_then_removing_single_item_leaves_data_empty(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        self.assertEqual(data.num_items(), 1)
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
        self.assertEqual(data.num_items(), 3)

        data.remove_data(run=1234)

        self.assert_empty(data)

    def test_that_remove_method_applies_AND_to_multiple_keyword_arguments(self):
        data = self.populate_loaded_data()
        self.assertEqual(data.num_items(), 3)

        data.remove_data(run=1234, workspace=[2], filename="C:\\dir1\\file3.nxs")

        self.assert_unchanged(data)

    def test_that_contains_is_true_when_data_contains_entry_which_matches_to_a_single_keyword_and_false_otherwise(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1234, workspace=[3], filename="C:\\dir1\\file3.nxs")

        self.assertTrue(data.contains(run=1234))
        self.assertTrue(data.contains(filename="C:\\dir1\\file1.nxs"))

        self.assertFalse(data.contains(run=9999))
        self.assertFalse(data.contains(filename="C:\\dir1\\file4.nxs"))

    def test_that_contains_is_false_when_data_contains_entries_which_match_to_only_one_of_multiple_keywords(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1234, workspace=[3], filename="C:\\dir1\\file3.nxs")

        # values from keywords correspond to different entries
        self.assertFalse(data.contains(run=1234, filename="C:\\dir1\\file2.nxs"))
        self.assertTrue(data.contains(run=1234, workspace=[3]))

    def test_counting_entries_with_keyword_argument_gives_correct_count(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1234, workspace=[3], filename="C:\\dir1\\file3.nxs")

        self.assertEqual(data.contains_n(run=1234), 2)
        self.assertEqual(data.contains_n(filename="C:\\dir1\\file1.nxs"), 1)
        self.assertEqual(data.contains_n(run=9999), 0)

    def test_counting_entries_applies_AND_behaviour_to_keyword_arguments(self):
        data = MuonLoadData()

        data.add_data(run=1234, workspace=[1], filename="C:\\dir1\\file1.nxs")
        data.add_data(run=1235, workspace=[2], filename="C:\\dir1\\file2.nxs")
        data.add_data(run=1234, workspace=[3], filename="C:\\dir1\\file3.nxs")
        data.add_data(run=1236, workspace=[4], filename="C:\\dir1\\file4.nxs")

        self.assertEqual(data.contains_n(run=1234, workspace=[1]), 1)
        self.assertEqual(data.contains_n(run=1234, workspace=[2]), 0)
        self.assertEqual(data.contains_n(run=1234, workspace=[2], filename="C:\\dir1\\file4.nxs"), 0)

    def test_iterator_behaviour_of_data(self):
        data = self.populate_loaded_data()

        check = [{"run": 1234, "workspace": [1], "filename": "C:\\dir1\\file1.nxs", 'instrument': 'EMU'},
                 {"run": 1235, "workspace": [2], "filename": "C:\\dir1\\file2.nxs", 'instrument': 'EMU'},
                 {"run": 1236, "workspace": [3], "filename": "C:\\dir1\\file3.nxs", 'instrument': 'EMU'}]

        for data_item in iter(data):
            self.assertEqual(data_item, check.pop(0))

    def test_that_remove_current_data_removes_the_most_recently_added_data(self):
        data = self.populate_loaded_data()

        data.remove_current_data()

        check = [{"run": 1234, "workspace": [1], "filename": "C:\\dir1\\file1.nxs", 'instrument': 'EMU'},
                 {"run": 1235, "workspace": [2], "filename": "C:\\dir1\\file2.nxs", 'instrument': 'EMU'}]

        for data_item in iter(data):
            self.assertEqual(data_item, check.pop(0))

    def test_that_remove_last_added_data_removes_the_previous_data_item_before_the_most_recent(self):
        data = self.populate_loaded_data()

        data.remove_last_added_data()

        check = [{"run": 1234, "workspace": [1], "filename": "C:\\dir1\\file1.nxs", 'instrument': 'EMU'},
                 {"run": 1236, "workspace": [3], "filename": "C:\\dir1\\file3.nxs", 'instrument': 'EMU'}]

        for data_item in iter(data):
            self.assertEqual(data_item, check.pop(0))

    def test_that_can_add_data_to_struct(self):
        self.assertEqual(self.muon_load_data.num_items(), 5)
        self.assertEqual(self.muon_load_data.get_parameter('run')[2], 3)
        self.assertEqual(self.muon_load_data.get_parameter('workspace')[1], self.workspace)
        self.assertEqual(self.muon_load_data.get_parameter('filename')[0], 'path to file')

    def test_that_matches_returns_false_for_all_entries_with_only_one_match(self):
        match_list = self.muon_load_data._matches(run=1, workspace=self.workspace, filename='matching path')

        self.assertEqual(match_list, [False, False, False, False, False])

    def test_that_matches_with_no_params_matches_all(self):
        match_list = self.muon_load_data._matches()

        self.assertEqual(match_list, [True, True, True, True, True])

    def test_that_matches_with_unused_parameters_match_none(self):
        match_list = self.muon_load_data._matches(new_info='new info')

        self.assertEqual(match_list, [False, False, False, False, False])

    def test_that_matches_correctly_with_only_one_parameter_given(self):
        match_list = self.muon_load_data._matches(filename='path to file')

        self.assertEqual(match_list, [True, True, False, False, True])

    def test_that_get_data_returns_correct_dict(self):
        data_dict = self.muon_load_data.get_data(run=2)

        self.assertEqual(data_dict, {'workspace': self.workspace, 'filename': 'path to file', 'run': 2, 'instrument': ''})

    def test_that_get_latest_data_returns_correct_dict(self):
        data_dict = self.muon_load_data.get_latest_data()

        self.assertEqual(data_dict, {'workspace': self.workspace_last, 'filename': 'path to file', 'run': 4, 'instrument': ''})


if __name__ == "__main__":
    unittest.main(verbosity=2)
