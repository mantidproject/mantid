import sys
import six
from qtpy import QtWidgets, QtCore

from Muon.GUI.MuonAnalysis.loadfile.load_file_model_multithreading import BrowseFileWidgetModel
from Muon.GUI.Common.muon_load_data import MuonLoadData

import unittest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class IteratorWithException(object):
    """Wraps a simple iterable (i.e. list) so that it throws a ValueError on a particular index."""

    def __init__(self, iterable, throw_on_index):
        self.max = len(iterable)
        self.iterable = iter(iterable)

        self.throw_indices = [index for index in throw_on_index if index <= self.max]

        self.lock = QtCore.QMutex()

    def __iter__(self):
        self.n = 0
        return self

    def __next__(self):
        with QtCore.QMutexLocker(self.lock):
            if self.n == self.max:
                raise StopIteration()
            if self.n in self.throw_indices:
                next(self.iterable)
                self.n += 1
                raise ValueError(str(self.n) )
            else:
                self.n += 1
                return next(self.iterable)


    # Python 3 compatibility
    next = __next__


class LoadFileWidgetModelTest(unittest.TestCase):
    class Runner:
        QT_APP = QtWidgets.QApplication([])

        def __init__(self, thread):
            if thread:
                self._thread = thread
                self._thread.finished.connect(self.finished)
                self.QT_APP.exec_()

        def finished(self):
            self.QT_APP.processEvents()
            self.QT_APP.exit(0)

    def test_model_initialized_with_empty_lists_of_loaded_data(self):
        model = BrowseFileWidgetModel(MuonLoadData())
        self.assertEqual(model.loaded_workspaces, [])
        self.assertEqual(model.loaded_filenames, [])
        self.assertEqual(model.loaded_runs, [])

    def test_executing_load_without_filenames_does_nothing(self):
        model = BrowseFileWidgetModel(MuonLoadData())
        model.load_with_multithreading(filenames=[])
        self.assertEqual(model.loaded_workspaces, [])
        self.assertEqual(model.loaded_filenames, [])
        self.assertEqual(model.loaded_runs, [])

    def test_execute_successfully_loads_valid_files(self):
        # Mock the load algorithm
        files = [r'EMU00019489.nxs', r'EMU00019490.nxs', r'EMU00019491.nxs']
        load_return_vals = [(files[i], [1, 2, 3], 19489 + i) for i in range(3)]

        model = BrowseFileWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock(side_effect = iter(load_return_vals))

        model.load_with_multithreading(filenames=files)
        self.Runner(model.thread_manager)
        model.add_thread_data()

        self.assertEqual(len(model.loaded_workspaces), len(model.loaded_runs))
        self.assertEqual(len(model.loaded_workspaces), len(model.loaded_filenames))
        self.assertEqual(len(model.loaded_workspaces), 3)
        six.assertCountEqual(self, model.loaded_filenames, files)
        six.assertCountEqual(self, model.loaded_runs, [19489, 19490, 19491])

    def test_model_is_cleared_correctly(self):
        files = [r'EMU00019489.nxs', r'EMU00019490.nxs', r'EMU00019491.nxs']
        load_return_vals = [(files[i], [1, 2, 3], 19489 + i) for i in range(3)]

        model = BrowseFileWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock()
        model.load_workspace_from_filename.side_effect = load_return_vals

        model.load_with_multithreading(filenames=files)
        self.Runner(model.thread_manager)
        model.add_thread_data()

        model.clear()
        self.assertEqual(model.loaded_workspaces, [])
        self.assertEqual(model.loaded_filenames, [])
        self.assertEqual(model.loaded_runs, [])

    def test_execute_throws_if_one_file_does_not_load_correctly_but_still_loads_other_files(self):
        files = [r'EMU00019489.nxs', r'EMU00019490.nxs', r'EMU00019491.nxs']
        load_return_vals = [(files[i], [1, 2, 3], 19489 + i) for i in range(3)]

        model = BrowseFileWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock()
        iterator = IteratorWithException(load_return_vals, [1])
        model.load_workspace_from_filename.side_effect = iter(iterator)

        model.load_with_multithreading(filenames=files)
        self.Runner(model.thread_manager)
        model.add_thread_data()

        self.assertEqual(len(model.loaded_workspaces), 2)
        six.assertCountEqual(self, model.loaded_filenames, [files[0], files[2]])
        six.assertCountEqual(self, model.loaded_runs, [19489, 19491])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
