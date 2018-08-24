from __future__ import (absolute_import, division, print_function)

import unittest

try:
    from unittest import mock
except ImportError:
    import mock

from qtpy import QtWidgets, QtCore
from Muon.GUI.Common.threading_worker import Worker


def test_function(lst):
    return 2 * lst


def test_function_raises_on_n(lst, raise_on=0):
    if lst == raise_on:
        raise ValueError
    else:
        return 2 * lst


class ThreadingManagerWorkerTest(unittest.TestCase):
    class Runner:
        QT_APP = QtWidgets.QApplication([])

        def __init__(self, thread, worker):
            self._thread = thread
            self._worker = worker
            self._worker.signals.finished.connect(self.finished)
            self._worker.signals.start.emit()
            self.QT_APP.exec_()

        def finished(self):
            self._thread.quit()
            self._thread.wait()
            self.QT_APP.processEvents()
            self.QT_APP.exit(0)

    def setUp(self):
        pass

    def test_that_worker_thread_starts_and_finishes(self):
        worker = Worker(test_function, lst=[1, 2, 3, 4, 5])

        worker_started = mock.Mock()
        worker_finished = mock.Mock()

        thread = QtCore.QThread()
        worker.moveToThread(thread)
        worker.signals.started.connect(worker_started)
        worker.signals.start.connect(worker.run)
        worker.signals.finished.connect(worker_finished)
        thread.start()
        self.Runner(thread, worker)

        self.assertEqual(worker_started.call_count, 1)
        self.assertEqual(worker_finished.call_count, 1)

    def test_that_worker_thread_updates_progress_from_each_evaluation(self):
        worker = Worker(test_function, lst=[1, 2, 3, 4, 5])

        worker_progress = mock.Mock()

        thread = QtCore.QThread()
        worker.moveToThread(thread)
        worker.signals.start.connect(worker.run)
        worker.signals.progress.connect(worker_progress)
        thread.start()
        self.Runner(thread, worker)

        self.assertEqual(worker_progress.call_count, 5)

    def test_that_worker_thread_evaluates_results_correctly(self):
        worker = Worker(test_function, lst=[1, 2, 3, 4, 5])

        worker_result = mock.Mock()

        thread = QtCore.QThread()
        worker.moveToThread(thread)
        worker.signals.start.connect(worker.run)
        worker.signals.result.connect(worker_result)
        thread.start()
        self.Runner(thread, worker)

        self.assertEqual(worker_result.call_count, 1)
        self.assertEqual(worker_result.call_args_list[0][0][0][0]["lst"], [1, 2, 3, 4, 5])
        self.assertEqual(worker_result.call_args_list[0][0][0][1], [2, 4, 6, 8, 10])

    def test_that_error_slot_called_if_function_raises(self):
        worker = Worker(test_function_raises_on_n, lst=[1, 2, 3, 4, 5], raise_on=1)

        worker_result = mock.Mock()
        worker_error = mock.Mock()

        thread = QtCore.QThread()
        worker.moveToThread(thread)
        worker.signals.start.connect(worker.run)
        worker.signals.result.connect(worker_result)
        worker.signals.error.connect(worker_error)
        thread.start()
        self.Runner(thread, worker)

        self.assertEqual(worker_result.call_count, 0)
        self.assertEqual(worker_error.call_count, 1)


if __name__ == "__main__":
    unittest.main(verbosity=2)
