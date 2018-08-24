from __future__ import (absolute_import, division, print_function)

import traceback
import sys
import time

from qtpy import QtCore
from qtpy.QtCore import Signal


# Wraps a function, adding the following :
# - Converts function arguments to allow for lists of arguments to be passed with the same names.
# - Catches exceptions and returns a list of these as second return value.
# - Adds a new argument 'progress_callback', to which user passes a signal, this is then emitting with
# the progression of the evaluation of the argument list.
def threading_decorator(fn):
    def wrapper(progress_callback, **kwargs):
        results, failed_results_exceptions = [], []
        num_evals = len(list(kwargs.values())[0])
        input_list = [{key: value[i] for key, value in kwargs.items()} for i in range(num_evals)]
        for inputs in input_list:
            try:
                result = fn(**inputs)
            except Exception as e:
                failed_results_exceptions.append(e)
                continue
            results.append(result)
            progress_callback.emit(100 / num_evals)
        return results, failed_results_exceptions

    return wrapper


class WorkerSignals(QtCore.QObject):
    """
    Defines the signals available from a running worker thread.
    """
    started = Signal()
    finished = Signal()
    error = Signal(dict)
    result = Signal(tuple)
    progress = Signal(int)
    cancelled = Signal()
    # Use this to start the worker
    start = Signal()


class Worker(QtCore.QObject):
    """
    Worker thread. Inherits from QThread to handle worker thread setup, signals and wrap-up. Using QThread here
    (instead of QRunnable) so we can cancel the thread.

    fn : A function to evaluate
    kwargs : keyword arguments to pass to fn

    returns are via signals defined in WorkerSignals()
    """

    def __init__(self, fn, **kwargs):
        """
        :param fn: The function to be evaluated.
        :param kwargs: Keyword argument lists to pass to the function (e.g. x =[1,2,3] for f(x) where x is an integer).
        """
        super(Worker, self).__init__()
        self.fn = threading_decorator(fn)
        self.kwargs = kwargs
        self.signals = WorkerSignals()
        # Add callback so function can update its progress
        self.kwargs['progress_callback'] = self.signals.progress

    def run(self):
        self.signals.started.emit()
        try:
            result, fails = self.fn(**self.kwargs)
            if 'progress_callback' in self.kwargs:
                del self.kwargs['progress_callback']
            for fail in fails:
                # Exceptions caught whilst executing the function
                exctype, value = type(fail), next(iter(fail.args), "")
                self.signals.error.emit({"inputs": self.kwargs,
                                         "exctype": exctype,
                                         "value": value,
                                         "traceback": traceback.format_exc()})
        except:
            # Catch errors not thrown by the function evaluation
            exctype, value = sys.exc_info()[:2]  # traceback.print_exc()
            if 'progress_callback' in self.kwargs:
                del self.kwargs['progress_callback']
            self.signals.error.emit({"inputs": self.kwargs,
                                     "exctype": exctype,
                                     "value": value,
                                     "traceback": traceback.format_exc()})
        else:
            # Successful evaluation
            if 'progress_callback' in self.kwargs:
                del self.kwargs['progress_callback']
            self.signals.result.emit((self.kwargs, result))
        finally:
            self.signals.finished.emit()
