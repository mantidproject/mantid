# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import functools
import inspect
import sys

from qtpy.QtCore import Qt, QMetaObject, QObject, QThread, Slot
from qtpy.QtWidgets import QApplication


class QAppThreadCall(QObject):
    """
    Wraps a callable object and forces any calls made to it to be executed
    on the same thread as the qApp object. If QApplication does not yet exist
    at the point of call then a direct call is made
    """
    __slots__ = ["callee", "_args", "_kwargs", "_result", "_exc_info", "_moved_to_app"]

    @staticmethod
    def is_qapp_thread():
        """Returns True iff a QApplication instance exists and the current
        thread matches the thread the QApplication was created in
        """
        qapp = QApplication.instance()
        return qapp is None or (QThread.currentThread() == qapp.thread())

    def __init__(self, callee):
        QObject.__init__(self)
        self.callee = callee
        functools.update_wrapper(self.__call__.__func__, callee)
        self._moved_to_app = False
        self._store_function_args(None, None)

    def __call__(self, *args, **kwargs):
        """
        If the current thread is the qApp thread then this
        performs a straight call to the wrapped callable_obj. Otherwise
        it invokes the do_call method as a slot via a
        BlockingQueuedConnection.
        """
        if self.is_qapp_thread():
            return self.callee(*args, **kwargs)
        else:
            self._ensure_self_on_qapp_thread()
            self._store_function_args(args, kwargs)
            QMetaObject.invokeMethod(self, "on_call", Qt.BlockingQueuedConnection)
            if self._exc_info is not None:
                raise self._exc_info[1].with_traceback(self._exc_info[2])
            return self._result

    @Slot()
    def on_call(self):
        """Perform a call to a GUI function across a
        thread and return the result
        """
        try:
            self._result = \
                self.callee(*self._args, **self._kwargs)
        except Exception:  # pylint: disable=broad-except
            self._exc_info = sys.exc_info()

    # private api
    def _ensure_self_on_qapp_thread(self):
        """Assuming the QApplication instance exists, ensure this object is on
        that thread"""
        if self._moved_to_app:
            return

        self.moveToThread(QApplication.instance().thread())
        self._moved_to_app = True

    def _store_function_args(self, args, kwargs):
        """
        Store arguments for future function call and reset result/exception info
        from previous call
        """
        self._args, self._kwargs = args, kwargs
        self._result, self._exc_info = None, None


def force_method_calls_to_qapp_thread(instance, *, all_methods=False):
    """
    Replaces each method on the instance with a QAppThreadCall object that forces
    that method to be called on the QApplication thread
    :param instance: Instance whose methods should be force to be called on QApplication thread
    :param all_methods: If True all methods, including those starting with '_' are replaced
    :return: The instance object with methods replaced
    """

    # Ideally we would use inspect.getmembers but it calls getattr to test the type of the attribute
    # and causes properties to be evaluated. If accessing the attribute code has side effects
    # then these will occur immediately
    # We implement a simpler method using getattr_static to avoid anything being called
    def getmethods(obj):
        attrs = dir(obj)
        results = []
        for name in attrs:
            # returns unbound functions not methods
            attr = inspect.getattr_static(obj, name)
            if callable(attr):
                results.append((name, getattr(obj, name)))
        return results

    for name, method in getmethods(instance):
        if all_methods or not name.startswith('_'):
            setattr(instance, name, QAppThreadCall(method))

    return instance
