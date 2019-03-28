# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from Muon.GUI.Common.load_run_widget.load_run_model import LoadRunWidgetModel
from Muon.GUI.Common.muon_data_context import MuonDataContext
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.utilities.muon_test_helpers import IteratorWithException


class LoadRunWidgetModelTest(unittest.TestCase):
    def setUp(self):
        self.context = MuonDataContext()
        self.context.instrument = 'EMU'
        self.model = LoadRunWidgetModel(MuonLoadData(), self.context)

    def test_model_initialized_with_empty_lists_of_loaded_data(self):
        self.assertEqual(self.model.loaded_workspaces, [])
        self.assertEqual(self.model.loaded_filenames, [])
        self.assertEqual(self.model.loaded_runs, [])

    def test_executing_load_without_filenames_does_nothing(self):
        self.model.execute()
        self.assertEqual(self.model.loaded_workspaces, [])
        self.assertEqual(self.model.loaded_filenames, [])
        self.assertEqual(self.model.loaded_runs, [])

    @mock.patch('Muon.GUI.Common.load_run_widget.load_run_model.load_utils')
    def test_execute_successfully_loads_valid_files(self, load_utils_mock):
        # Mock the load algorithm
        files = [r'EMU00019489.nxs', r'EMU00019490.nxs', r'EMU00019491.nxs']
        load_return_vals = [([1, 2, 3], 19489 + i, filename) for i, filename in enumerate(files)]

        load_utils_mock.load_workspace_from_filename = mock.MagicMock()
        load_utils_mock.load_workspace_from_filename.side_effect = load_return_vals

        self.model.loadData(files)
        self.model.execute()
        self.assertEqual(len(self.model.loaded_workspaces), len(self.model.loaded_runs))
        self.assertEqual(len(self.model.loaded_workspaces), len(self.model.loaded_filenames))
        self.assertEqual(len(self.model.loaded_workspaces), 3)
        self.assertEqual(self.model.loaded_runs[0], [19489])
        self.assertEqual(self.model.loaded_runs[1], [19490])
        self.assertEqual(self.model.loaded_runs[2], [19491])

    @mock.patch('Muon.GUI.Common.load_run_widget.load_run_model.load_utils')
    def test_model_is_cleared_correctly(self, load_utils_mock):
        files = [r'EMU00019489.nxs', r'EMU00019490.nxs', r'EMU00019491.nxs']
        load_return_vals = [([1, 2, 3], filename, 19489 + i) for i, filename in enumerate(files)]

        load_utils_mock.load_workspace_from_filename = mock.Mock()
        load_utils_mock.load_workspace_from_filename.side_effect = load_return_vals

        self.model.loadData(files)
        self.model.execute()
        self.assertEqual(len(self.model.loaded_runs), 3)
        self.model.clear_loaded_data()
        self.assertEqual(self.model.loaded_workspaces, [])
        self.assertEqual(self.model.loaded_filenames, [])
        self.assertEqual(self.model.loaded_runs, [])

    @mock.patch('Muon.GUI.Common.load_run_widget.load_run_model.load_utils')
    def test_execute_throws_if_one_file_does_not_load_correctly_but_still_loads_other_files(self, load_utils_mock):
        files = [r'EMU00019489.nxs', r'EMU00019490.nxs', r'EMU00019491.nxs']
        load_return_vals = [([1, 2, 3], 19489 + i, filename) for i, filename in enumerate(files)]

        load_utils_mock.load_workspace_from_filename = mock.MagicMock()

        load_utils_mock.load_workspace_from_filename.side_effect = iter(IteratorWithException(load_return_vals, [1]))

        self.model.loadData(files)
        with self.assertRaises(ValueError):
            self.model.execute()
        self.assertEqual(len(self.model.loaded_workspaces), 2)
        self.assertEqual(self.model.loaded_filenames[0], files[0])
        self.assertEqual(self.model.loaded_filenames[1], files[2])
        self.assertEqual(self.model.loaded_runs[0], [19489])
        self.assertEqual(self.model.loaded_runs[1], [19491])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
