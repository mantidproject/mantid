# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import sys
from Muon.GUI.Common.muon_load_data import MuonLoadData

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


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

    def test_that_can_add_data_to_struct(self):
        self.assertEqual(self.muon_load_data.num_items(), 5)
        self.assertEqual(self.muon_load_data.get_parameter('run')[2], 3)
        self.assertEqual(self.muon_load_data.get_parameter('workspace')[1], self.workspace)
        self.assertEqual(self.muon_load_data.get_parameter('filename')[0], 'path to file')

    def test_that_matches_returns_true_for_all_entries_with_one_match(self):
        match_list = self.muon_load_data._matches(run=1, workspace=self.workspace, filename='matching path')

        self.assertEqual(match_list, [True, True, True, False, False])

    def test_that_matches_with_no_params_matches_none(self):
        match_list = self.muon_load_data._matches()

        self.assertEqual(match_list, [False, False, False, False, False])

    def test_that_matches_with_unused_parameters_match_none(self):
        match_list = self.muon_load_data._matches(new_info='new info')

        self.assertEqual(match_list, [False, False, False, False, False])

    def test_that_matches_correctly_with_only_one_parameter_given(self):
        match_list = self.muon_load_data._matches(filename='path to file')

        self.assertEqual(match_list, [True, True, False, False, True])

    def test_that_get_data_returns_correct_dict(self):
        data_dict = self.muon_load_data.get_data(run=2)

        self.assertEqual(data_dict, {'workspace': self.workspace, 'filename': 'path to file', 'run': 2})

    def test_that_get_latest_data_returns_correct_dict(self):
        data_dict = self.muon_load_data.get_latest_data()

        self.assertEqual(data_dict, {'workspace': self.workspace_last, 'filename': 'path to file', 'run': 4})


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
