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


class ThreadingManagerCancelWorkerManagerTest(unittest.TestCase):
    class ManagerRunner:
        QT_APP = QtWidgets.QApplication([])

        def __init__(self, thread):
            self._thread = thread
            self._thread.cancelled.connect(self.finished)
            QtCore.QTimer.singleShot(1000, self._thread.cancel)
            self.QT_APP.exec_()

        def finished(self):
            self.QT_APP.processEvents()
            self.QT_APP.exit(0)

    def cancel_test_function(lst):
        print("cancel_test_function ", lst)
        # add a delay to allow for cancellation of threads during evaluation
        time.sleep(1)
        return 2 * lst

    def test_that_worker_manager_can_be_safely_cancelled_while_running(self):
        # 4 executions per thread
        num_threads = 2
        manager = WorkerManager(fn=self.cancel_test_function, num_threads=num_threads, verbose=True,
                                lst=[1, 2, 3, 4, 5, 6, 7, 8])
        manager_cancelled = mock.Mock()
        manager.cancelled.connect(manager_cancelled)

        # Called function will wait 1 second on each arg (total of 5 seconds)
        manager.start()
        # Will call the cancel slot after 1 second
        self.ManagerRunner(manager)

        self.assertEqual(manager_cancelled.call_count, 1)

    def test_that_threads_and_workers_of_a_cancelled_worker_manager_are_cleared(self):
        # 4 executions per thread
        num_threads = 2
        manager = WorkerManager(self.cancel_test_function, num_threads=num_threads, verbose=True,
                                lst=[1, 2, 3, 4, 5, 6, 7, 8])
        manager_cancelled = mock.Mock()
        manager.cancelled.connect(manager_cancelled)

        # Called function will wait 1 second on each arg (total of 5 seconds)
        manager.start()
        # Will call the cancel slot after 1 second
        self.ManagerRunner(manager)

        self.assertEqual(manager._threads, [])
        self.assertEqual(manager._workers, [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
