import sys

from Muon.GUI.MuonAnalysis.loadrun.load_run_model_multithreading import LoadRunWidgetModel
from Muon.GUI.Common.muon_load_data import MuonLoadData

import unittest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class IteratorWithException:
    """Wraps a simple iterable (i.e. list) so that it throws a ValueError on a particular index."""

    def __init__(self, iterable, throw_on_index):
        self.max = len(iterable)
        self.iterable = iter(iterable)

        self.throw_indices = [index for index in throw_on_index if index < self.max]

    def __iter__(self):
        self.n = 0
        return self

    def next(self):

        if self.n in self.throw_indices:
            next(self.iterable)
            self.n += 1
            raise ValueError()
        elif self.n == self.max:
            raise StopIteration()
        else:
            self.n += 1
            return next(self.iterable)


class LoadRunWidgetModelTest(unittest.TestCase):

    def test_model_initialized_with_empty_lists_of_loaded_data(self):
        model = LoadRunWidgetModel(MuonLoadData())
        self.assertEqual(model.loaded_workspaces, [])
        self.assertEqual(model.loaded_filenames, [])
        self.assertEqual(model.loaded_runs, [])

    def test_executing_load_without_filenames_does_nothing(self):
        model = LoadRunWidgetModel(MuonLoadData())
        model.execute()
        self.assertEqual(model.loaded_workspaces, [])
        self.assertEqual(model.loaded_filenames, [])
        self.assertEqual(model.loaded_runs, [])

    def test_execute_successfully_loads_valid_files(self):
        # Mock the load algorithm
        files = [r'EMU00019489.nxs', r'EMU00019490.nxs', r'EMU00019491.nxs']
        load_return_vals = [([1, 2, 3], filename, 19489 + i) for i, filename in enumerate(files)]

        model = LoadRunWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock()
        model.load_workspace_from_filename.side_effect = load_return_vals

        model.loadData(files)
        model.execute()
        self.assertEqual(len(model.loaded_workspaces), len(model.loaded_runs))
        self.assertEqual(len(model.loaded_workspaces), len(model.loaded_filenames))
        self.assertEqual(len(model.loaded_workspaces), 3)
        self.assertEqual(model.loaded_filenames[0], files[0])
        self.assertEqual(model.loaded_filenames[1], files[1])
        self.assertEqual(model.loaded_filenames[2], files[2])
        self.assertEqual(model.loaded_runs[0], 19489)
        self.assertEqual(model.loaded_runs[1], 19490)
        self.assertEqual(model.loaded_runs[2], 19491)

    def test_model_is_cleared_correctly(self):
        files = [r'EMU00019489.nxs', r'EMU00019490.nxs', r'EMU00019491.nxs']
        load_return_vals = [([1, 2, 3], filename, 19489 + i) for i, filename in enumerate(files)]

        model = LoadRunWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock()
        model.load_workspace_from_filename.side_effect = load_return_vals

        model.loadData(files)
        model.execute()
        model.clear_loaded_data()
        self.assertEqual(model.loaded_workspaces, [])
        self.assertEqual(model.loaded_filenames, [])
        self.assertEqual(model.loaded_runs, [])

    def test_execute_throws_if_one_file_does_not_load_correctly_but_still_loads_other_files(self):
        files = [r'EMU00019489.nxs', r'EMU00019490.nxs', r'EMU00019491.nxs']
        load_return_vals = [([1, 2, 3], filename, 19489 + i) for i, filename in enumerate(files)]

        model = LoadRunWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock()

        model.load_workspace_from_filename.side_effect = iter(IteratorWithException(load_return_vals, [1]))

        model.loadData(files)
        with self.assertRaises(ValueError):
            model.execute()
        self.assertEqual(len(model.loaded_workspaces), 2)
        self.assertEqual(model.loaded_filenames[0], files[0])
        self.assertEqual(model.loaded_filenames[1], files[2])
        self.assertEqual(model.loaded_runs[0], 19489)
        self.assertEqual(model.loaded_runs[1], 19491)

if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)