import sys
import six

import unittest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

from Muon.GUI.MuonAnalysis.loadfile.load_file_model import BrowseFileWidgetModel
from Muon.GUI.Common.muon_load_data import MuonLoadData


class IteratorWithException:
    """Wraps a simple iterable (i.e. list) so that it throws a ValueError on a particular index."""

    def __init__(self, iterable, throw_on_index):
        self.max = len(iterable)
        self.iterable = iter(iterable)

        self.throw_indices = [index for index in throw_on_index if index < self.max]

    def __iter__(self):
        self.n = 0
        return self

    def __next__(self):

        if self.n in self.throw_indices:
            next(self.iterable)
            self.n += 1
            raise ValueError()
        elif self.n == self.max:
            raise StopIteration()
        else:
            self.n += 1
            return next(self.iterable)

    # python 3 compatibility
    next = __next__


class LoadFileWidgetModelTest(unittest.TestCase):

    def setUp(self):
        self.data = MuonLoadData()
        self.model = BrowseFileWidgetModel(self.data)

    def mock_load_function(self, files_to_load, load_return_values):
        self.model.load_workspace_from_filename = mock.Mock(side_effect=load_return_values)
        self.model.loadData(files_to_load)

    def assert_model_empty(self):
        self.assertEqual(self.model.loaded_workspaces, [])
        self.assertEqual(self.model.loaded_filenames, [])
        self.assertEqual(self.model.loaded_runs, [])

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_model_initialized_with_empty_lists_of_loaded_data(self):
        self.assert_model_empty()

    def test_executing_load_without_filenames_does_nothing(self):
        self.model.execute()
        self.assert_model_empty()

    def test_execute_successfully_loads_given_files(self):
        files = ['EMU00019489.nxs', 'EMU00019490.nxs', 'EMU00019491.nxs']
        load_return_vals = [([1 + i], 19489 + i) for i in range(3)]
        self.mock_load_function(files, load_return_vals)

        self.model.execute()

        six.assertCountEqual(self, self.model.loaded_workspaces, [[1], [2], [3]])
        six.assertCountEqual(self, self.model.loaded_filenames, files)
        six.assertCountEqual(self, self.model.loaded_runs, [19489, 19490, 19491])

    def test_model_is_cleared_correctly(self):
        files = [r'EMU00019489.nxs', r'EMU00019490.nxs', r'EMU00019491.nxs']
        load_return_vals = [([1, 2, 3], 19489 + i) for i in range(3)]
        self.mock_load_function(files, load_return_vals)

        self.model.execute()
        self.model.clear()

        self.assert_model_empty()

    def test_execute_throws_if_one_file_does_not_load_correctly_but_still_loads_other_files(self):
        files = [r'EMU00019489.nxs', r'EMU00019490.nxs', r'EMU00019491.nxs']
        load_return_vals = [([1, 2, 3], 19489 + i) for i in range(3)]

        # Mock load to throw on a particular index
        self.model.load_workspace_from_filename = mock.Mock()
        self.model.load_workspace_from_filename.side_effect = iter(IteratorWithException(load_return_vals, [1]))
        self.model.loadData(files)
        with self.assertRaises(ValueError):
            self.model.execute()

        six.assertCountEqual(self, self.model.loaded_filenames, [files[i] for i in [0, 2]])
        six.assertCountEqual(self, self.model.loaded_runs, [19489, 19491])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
