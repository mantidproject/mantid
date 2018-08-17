from __future__ import (absolute_import, division, print_function)

from qtpy import QtCore, QtWidgets
from qtpy.QtCore import Signal
from qtpy.QtCore import Slot
import traceback
import sys


class WorkerSignals(QtCore.QObject):
    """
    Defines the signals available from a running worker thread.
    """
    started = Signal()
    finished = Signal()
    error = Signal(tuple)
    result = Signal(object)
    progress = Signal(int)
    cancelled = Signal()


class Worker(QtCore.QThread):
    """
    Worker thread. Inherits from QRunnable to handler worker thread setup, signals and wrap-up.
    """

    def __init__(self, fn, *args, **kwargs):
        super(Worker, self).__init__()
        # Store constructor arguments (re-used for processing)
        self.fn = fn
        self.args = args
        self.kwargs = kwargs
        self.signals = WorkerSignals()
        # Add the callback to our kwargs
        self.kwargs['progress_callback'] = self.signals.progress

    @Slot()
    def run(self):
        """
        Initialise the runner function with passed args, kwargs.
        """
        self.signals.started.emit()
        try:
            result = self.fn(*self.args, **self.kwargs)
        except:
            traceback.print_exc()
            exctype, value = sys.exc_info()[:2]
            self.signals.error.emit((exctype, value, traceback.format_exc()))
        else:
            self.signals.result.emit(result)
        finally:
            self.signals.finished.emit()

    def cancel(self):
        print("Thread cancelled")
        self.signals.cancelled.emit()
        self.exit()

class WorkerManager(QtWidgets.QWidget):
    """
    fn : A function to be evaluated.
    arg_list : A list of (a single) input parameter to be given to fn
    num_threads : The max number of threads to be executed across the arg_list
    finished
    """

    finished = Signal()

    def __init__(self, fn, arg_list, num_threads,
                 callback_on_threads_complete=lambda: 0,
                 callback_on_progress_update=lambda x: 0,
                 callback_on_thread_exception=lambda x: 0):
        super(WorkerManager, self).__init__()

        # Callback called when all threads have executed
        self.thread_complete_callback = callback_on_threads_complete
        # Callback called when a single thread updates its progress
        self.progress_callback = callback_on_progress_update
        # Callback called when an error is thrown from a thread
        self.error_callback = callback_on_thread_exception

        self.fn = fn
        self.arg_list = arg_list

        self._threads = []
        self._results = []

        self._progress = 0.0  # current progress through all threads

        self._num_threads = num_threads  # total number of threads started
        self._thread_count = 0  # current threads running

        self.threadpool = QtCore.QThreadPool()
        self.mutex = QtCore.QMutex()
        print("Multithreading with maximum %d threads" % self.threadpool.maxThreadCount())

    def cancel_threads(self):
        for thread in self._threads:
            if thread.isRunning():
                thread.cancel()
            else:
                print("ERROR")

    def clear(self):
        print("Clearing")
        self.cancel_threads()
        self._threads = []
        self._thread_count = 0

    def isRunning(self):
        return self._thread_count > 0

    def chunkify(self, lst, n):
        return [lst[i::n] for i in xrange(n)]

    def start(self):
        self._thread_count = self._num_threads
        # split arg_list into num_threads equally sized lists
        thread_list = self.chunkify(self.arg_list, self._num_threads)
        for i, arg in enumerate(thread_list):
            worker = Worker(self.fn, lst=arg)
            worker.signals.started.connect(self.on_thread_start)
            worker.signals.result.connect(self.on_thread_result)
            worker.signals.finished.connect(self.on_thread_complete)
            worker.signals.progress.connect(self.on_thread_progress)
            worker.signals.error.connect(self.handle_error)
            worker.signals.cancelled.connect(self.handle_cancel)
            print("\tStarting thread " + str(i + 1) + " with arg : ", arg)
            self._threads += [worker]
            #self.threadpool.start(worker)
            worker.start()

    def handle_cancel(self):
        print("Cancelled")

    def handle_error(self, exctype, value, traceback):
        print("\thandle_error")
        self.error_callback(exctype, value, traceback)

    def on_thread_result(self, s):
        self.mutex.lock()
        print("\tThread Result : " , s)
        self._results += [s]
        self.mutex.unlock()

    def on_thread_start(self):
        print("\tThread started!\n")

    def on_thread_complete(self):
        print("\tTHREAD " + str(self._thread_count) + " COMPLETE!\n")
        self.mutex.lock()
        self._thread_count -= 1
        self.mutex.unlock()
        if self._thread_count == 0:
            self.thread_complete_callback()
            # This line is for unit testing so we can wait for the threads
            # to be finished.
            # self.signals.finished.emit()
            self.finished.emit()

    def update_progress(self, percentage):
        self.mutex.lock()
        self._progress += percentage
        self.progress_callback(self._progress)
        self.mutex.unlock()

    def on_thread_progress(self, percentage):
        """Optional functionality to allow the threads to inform on their progress"""
        self.mutex.lock()
        print("\tTHREAD progress : ", percentage)
        self.mutex.unlock()
        self.update_progress(percentage * 1 / self._num_threads)


class Runner:
    QT_APP = QtWidgets.QApplication([])

    def __init__(self, thread):
        self._thread = thread
        self._thread.finished.connect(self.finished)
        QtCore.QTimer.singleShot(2000, lambda:0)
        thread.clear()
        self.QT_APP.exec_()

    def finished(self):
        self.QT_APP.processEvents()
        self.QT_APP.exit(0)


if __name__ == "__main__":

    import time


    def test_function(progress_callback, lst):

        results = []
        for item in lst:
            time.sleep(1)
            results += [item]
            progress_callback.emit(100 / len(lst))
        return results


    worker_manager = WorkerManager(test_function, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10], 4)
    worker_manager.start()
    Runner(worker_manager)

    print(worker_manager._results)
