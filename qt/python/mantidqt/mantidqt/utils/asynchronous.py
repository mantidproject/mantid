# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
# system imports
import ctypes
import sys
import threading
import time
from traceback import extract_tb
from mantid.api import IAlgorithm
from enum import IntEnum


class TaskExitCode(IntEnum):
    OK = 0
    ERROR = 1
    SYNTAX_ERROR = 2


class AsyncTask(threading.Thread):
    def __init__(self, target, args=(), kwargs=None, success_cb=None, error_cb=None, finished_cb=None):
        """
        Runs a task asynchronously. Exit code is set on task finish/error.

        :param target: A Python callable object
        :param args: Arguments to pass to the callable
        :param kwargs: Keyword arguments to pass to the callable
        :param success_cb: Optional callback called when operation was successful
        :param error_cb: Optional callback called when operation was not successful
        :param finished_cb: Optional callback called when operation has finished
        """
        super(AsyncTask, self).__init__()
        if not callable(target):
            raise TypeError("Target object is required to be callable")

        self.target = target
        self.args = args
        self.kwargs = kwargs if kwargs is not None else {}

        self.success_cb = success_cb if success_cb is not None else lambda x: None
        self.error_cb = error_cb if error_cb is not None else lambda x: None
        self.finished_cb = finished_cb if finished_cb is not None else lambda: None

        self.exit_code = None

    @staticmethod
    def _elapsed(start):
        return time.time() - start

    def run(self):
        """
        DO NOT CALL DIRECTLY. Use .start() to begin the thread execution.
        """
        time_start = time.time()

        try:
            out = self.target(*self.args, **self.kwargs)
        except SyntaxError as exc:
            self.exit_code = TaskExitCode.SYNTAX_ERROR
            # treat SyntaxErrors as special as the traceback makes no sense
            # and the lineno is part of the exception instance
            self.error_cb(AsyncTaskFailure(self._elapsed(time_start), SyntaxError, exc, None))
        except:
            self.exit_code = TaskExitCode.ERROR
            self.error_cb(AsyncTaskFailure.from_excinfo(self._elapsed(time_start)))
        else:
            self.exit_code = TaskExitCode.OK
            self.success_cb(AsyncTaskSuccess(self._elapsed(time_start), out))

        self.finished_cb()

    def abort(self, interrupt=True):
        """Cancel an asynchronous execution"""
        # Implementation is based on
        # https://stackoverflow.com/questions/5019436/python-how-to-terminate-a-blocking-thread
        if interrupt:
            ctypes.pythonapi.PyThreadState_SetAsyncExc(ctypes.c_long(self.ident), ctypes.py_object(KeyboardInterrupt))
        # now try and cancel the running algorithm
        alg = IAlgorithm._algorithmInThread(self.ident)
        if alg is not None:
            alg.cancel()
        time.sleep(0.1)


class _Receiver(object):
    def __init__(self, success_cb=None, error_cb=None):
        self.output = None
        self.exc_value = None

        self.success_cb = success_cb
        self.error_cb = error_cb

    def on_success(self, result):
        self.output = result.output
        if self.success_cb is not None:
            self.success_cb(result)

    def on_error(self, result):
        self.exc_value = result.exc_value
        if self.error_cb is not None:
            self.error_cb(result)


class BlockingAsyncTaskWithCallback(AsyncTask):
    def __init__(self, target, args=(), kwargs=None, success_cb=None, error_cb=None, blocking_cb=None, period_secs=0.05, finished_cb=None):
        """Run the target in a separate thread and block the calling thread
        until execution is complete,the calling thread is blocked, except that
        the blocking_cb will be executed in every period in it.

        :param target: A Python callable object
        :param args: Arguments to pass to the callable
        :param kwargs: Keyword arguments to pass to the callable
        :param blocking_cb: An optional callback to process while waiting for the task
        to finish
        :param period_secs: Sleep for this many seconds at the start of each loop that checks
        the task is still alive. This will be the minimum time between calls to blocking_cb.
        :returns: An AsyncTaskResult object
        """
        super(BlockingAsyncTaskWithCallback, self).__init__(target)

        self.period_secs = period_secs

        def create_callback(cb):
            return cb if cb is not None else lambda: None

        self.blocking_cb = create_callback(blocking_cb)
        self.success_cb = create_callback(success_cb)
        self.error_cb = create_callback(error_cb)
        self.finished_cb = create_callback(finished_cb)

        self.recv = _Receiver(success_cb=success_cb, error_cb=error_cb)
        self.task = AsyncTask(target, args, kwargs, success_cb=self.recv.on_success, error_cb=self.recv.on_error, finished_cb=finished_cb)

    def start(self):
        """:returns: An AsyncTaskResult object"""
        self.task.start()
        while self.task.is_alive():
            time.sleep(self.period_secs)
            self.blocking_cb()

        if self.recv.exc_value is not None:
            raise self.recv.exc_value
        else:
            return self.recv.output

    def abort(self):
        """Cancel an asynchronous execution"""
        # Implementation is based on
        # https://stackoverflow.com/questions/5019436/python-how-to-terminate-a-blocking-thread
        ctypes.pythonapi.PyThreadState_SetAsyncExc(ctypes.c_long(self.task.ident), ctypes.py_object(KeyboardInterrupt))
        # now try and cancel the running algorithm
        alg = IAlgorithm._algorithmInThread(self.task.ident)
        if alg is not None:
            alg.cancel()
        time.sleep(0.1)


class AsyncTaskResult(object):
    """Object describing the execution of an asynchronous task"""

    def __init__(self, elapsed_time):
        self.elapsed_time = elapsed_time
        self.timestamp = time.ctime()


class AsyncTaskSuccess(AsyncTaskResult):
    """Object describing the successful execution of an asynchronous task"""

    def __init__(self, elapsed_time, output):
        super(AsyncTaskSuccess, self).__init__(elapsed_time)
        self.output = output

    @property
    def success(self):
        return True


class AsyncTaskFailure(AsyncTaskResult):
    """Object describing the failed execution of an asynchronous task"""

    @staticmethod
    def from_excinfo(elapsed_time):
        """
        Create an AsyncTaskFailure from the current exception info

        :param elapsed_time Time take for task
        :param chop: Trim this number of entries from
        the top of the stack listing
        :return: A new AsyncTaskFailure object
        """
        exc_type, exc_value, exc_tb = sys.exc_info()
        return AsyncTaskFailure(elapsed_time, exc_type, exc_value, exc_tb)

    def __init__(self, elapsed_time, exc_type, exc_value, stack):
        super(AsyncTaskFailure, self).__init__(elapsed_time)
        self.exc_type = exc_type
        self.exc_value = exc_value
        self.stack = stack

    def __str__(self):
        error_name = type(self.exc_value).__name__
        filename, lineno, _, _ = extract_tb(self.stack)[-1]
        return f"{error_name} on line {lineno} of '{filename}': {self.exception_msg()}"

    def exception_msg(self):
        msg = self.exc_value.args
        if msg and isinstance(msg, tuple):
            msg = msg[0]
        return msg

    @property
    def success(self):
        return False


# From https://stackoverflow.com/questions/5179467/equivalent-of-setinterval-in-python by Andrea
# (https://stackoverflow.com/users/249001/andrea) with help of jd (https://stackoverflow.com/users/280490/jd)
def set_interval(interval):
    """
    This is a decorator function that will repeat once per interval.
    """

    def outer_wrap(function):
        def wrap(*args, **kwargs):
            stop = threading.Event()

            def inner_wrap():
                function(*args, **kwargs)
                while not stop.is_set():
                    stop.wait(interval)
                    function(*args, **kwargs)

            t = threading.Timer(0, inner_wrap)
            t.start()
            return stop

        return wrap

    return outer_wrap
