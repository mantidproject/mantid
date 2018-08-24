from __future__ import (absolute_import, division, print_function)

import sys
import time
import unittest

try:
    from unittest import mock
except ImportError:
    import mock

from qtpy import QtWidgets, QtCore
from Muon.GUI.Common.threading_manager import Worker, WorkerManager, split_kwarg_list, split_list_into_n_parts


def test_function(lst):
    return 2 * lst


def test_function_raises_on_n(lst, raise_on=0):
    if lst == raise_on:
        raise ValueError
    else:
        return 2 * lst


class ThreadingManagerFreeFunctionTest(unittest.TestCase):

    def test_split_list_into_n_parts_splits_list(self):
        lst = [1, 2, 3, 4, 5, 6, 7]
        split_list = split_list_into_n_parts(lst, 2)
        self.assertEqual(len(split_list), 2)
        self.assertEqual(split_list[0], [1, 3, 5, 7])
        self.assertEqual(split_list[1], [2, 4, 6])

    def test_that_splitting_list_into_more_parts_than_list_entries_creates_empty_lists(self):
        lst = [1]
        split_list = split_list_into_n_parts(lst, 4)
        self.assertEqual(len(split_list), 4)
        self.assertEqual(split_list[0], [1])
        self.assertEqual(split_list[1], [])

    def test_that_cannot_split_empty_list(self):
        lst = []
        split_list = split_list_into_n_parts(lst, 4)
        self.assertEqual(len(split_list), 4)
        for sublist in split_list:
            self.assertEqual(sublist, [])

    def test_split_kwarg_list_with_list_of_several_kwargs(self):
        kwarg_list = {"arg1": [1, 2, 3, 4, 5], "arg2": ["a", "b", "c", "d", "e"]}
        split_kwargs = split_kwarg_list(kwarg_list, 2)

        self.assertEqual(len(split_kwargs), 2)
        self.assertEqual(split_kwargs[0], {"arg1": [1, 3, 5], "arg2": ["a", "c", "e"]})
        self.assertEqual(split_kwargs[1], {"arg1": [2, 4], "arg2": ["b", "d"]})


class ThreadingManagerWorkerManagerTest(unittest.TestCase):
    class Runner:
        QT_APP = QtWidgets.QApplication([])

        def __init__(self, thread):
            self._thread = thread
            self._thread.finished.connect(self.finished)
            self._thread.start()
            self.QT_APP.exec_()

        def finished(self):
            self.QT_APP.processEvents()
            self.QT_APP.exit(0)

    def setUp(self):
        pass

    def test_manager_with_four_threads_starts_and_finishes(self):
        # 2 executions per thread
        manager_finished = mock.Mock()
        manager = WorkerManager(test_function, 4,
                                callback_on_threads_complete=manager_finished,
                                lst=[1, 2, 3, 4, 5, 6, 7, 8])

        self.Runner(manager)

        self.assertEqual(manager_finished.call_count, 1)

    def test_manager_with_four_threads_produces_expected_results(self):
        # 2 executions per thread
        num_threads = 4
        manager = WorkerManager(test_function, num_threads,
                                lst=[1, 2, 3, 4, 5, 6, 7, 8])

        self.Runner(manager)

        self.assertEqual(len(manager.results['results']), 8)
        for item in [1, 2, 3, 4, 5, 6, 7, 8]:
            index = [i for i, arg in enumerate(manager.results['lst']) if arg == item][0]
            self.assertEqual(manager.results['results'][index], 2 * item)

    #
    def test_manager_passes_exceptions_from_evaluation_to_callback(self):
        manager_finished = mock.Mock()
        manager_exception = mock.Mock()
        test_function = lambda lst: test_function_raises_on_n(lst, raise_on=1)
        manager = WorkerManager(fn=test_function, num_threads=4,
                                callback_on_threads_complete=manager_finished,
                                callback_on_thread_exception=manager_exception,
                                lst=[1, 2, 3, 4, 5, 6, 7, 8])
        self.Runner(manager)

        self.assertIn(1, manager_exception.call_args[1]['inputs']['lst'])

        self.assertEqual(manager_finished.call_count, 1)
        self.assertEqual(manager_exception.call_count, 1)

    def test_manager_stores_failed_evaluations(self):
        manager_finished = mock.Mock()
        manager_exception = mock.Mock()
        test_function = lambda lst: test_function_raises_on_n(lst, raise_on=1)
        manager = WorkerManager(fn=test_function, num_threads=4, verbose=True,
                                callback_on_threads_complete=manager_finished,
                                callback_on_thread_exception=manager_exception,
                                lst=[1, 2, 3, 4, 5, 6, 7, 8])
        self.Runner(manager)

        self.assertEqual(manager._failed_results['lst'], [1, 5])

    def test_that_once_started_manager_cannot_be_started_again_while_threads_are_running(self):
        num_threads = 4
        manager = WorkerManager(test_function, num_threads, lst=[1, 2, 3, 4])

        manager.is_running = mock.Mock(return_value=True)

        self.assertRaises(RuntimeError, manager.start)

    def test_that_manager_progress_is_ascending_and_goes_from_0_to_100(self):
        num_threads = 4
        manager_progress = mock.Mock()
        manager = WorkerManager(test_function, num_threads,
                                callback_on_progress_update=manager_progress,
                                lst=[1, 2, 3, 4])

        self.Runner(manager)

        progress_call_args = [i[0][0] for i in manager_progress.call_args_list]
        self.assertEqual(progress_call_args[0], 0.0)
        self.assertEqual(progress_call_args[-1], 100.0)
        self.assertEqual(sorted(progress_call_args), progress_call_args)


if __name__ == "__main__":
    unittest.main(verbosity=2)
