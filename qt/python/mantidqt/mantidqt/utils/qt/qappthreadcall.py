# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from dataclasses import dataclass
import functools
import inspect
import sys
from typing import Any, Callable, Optional, Sequence

from qtpy.QtCore import Qt, QMetaObject, QObject, QThread, Slot
from qtpy.QtWidgets import QApplication


class QAppThreadCall(QObject):
    """
    Wraps a callable object and forces any calls made to it to be executed
    on the same thread as the qApp object. If QApplication does not yet exist
    at the point of call then a direct call is made
    """

    @dataclass
    class CallArgs:
        """Encapsulate the arguments to a callable"""

        args: list

    @dataclass
    class CallResult:
        """Encapsulate the result of a object call"""

        result: Any
        exc_info: Optional[tuple]

    @staticmethod
    def is_qapp_thread() -> bool:
        """Returns True iff a QApplication instance exists and the current
        thread matches the thread the QApplication was created in
        """
        qapp = QApplication.instance()
        return qapp is None or (QThread.currentThread() == qapp.thread())

    __slots__ = ["_callable", "_blocking", "_moved_to_app", "_pending_calls", "_completed_calls"]

    def __init__(self, callable: Callable, blocking: bool = True):
        """
        :param callable: A callable that should be executed on the QApplication thread
        :param blocking: If True, the asynchronous call will block until completed
        """
        super().__init__()
        functools.update_wrapper(self.__call__.__func__, callable)

        self._callable = callable
        self._blocking = blocking
        self._moved_to_app = False
        self._pending_calls, self._completed_calls = [], []

    def __call__(self, *args: Sequence) -> Any:
        """
        If the current thread is the qApp thread then this
        performs a straight call to the wrapped callable_obj. Otherwise
        it invokes the do_call method as a slot via a
        BlockingQueuedConnection.
        """
        if self.is_qapp_thread():
            return self._callable(*args)
        else:
            self._ensure_self_on_qapp_thread()
            connection_type = Qt.BlockingQueuedConnection if self._blocking else Qt.AutoConnection

            # A given object wraps a single callable. If calls are non-blocking
            # then a second call can enter this method before the previous has
            # had time to perform _on_call. Queue up the arguments to ensure
            # they are processed in the correct order. This method and _on_call
            # are very closely tied together. We append arguments to the end of a pending list
            # and pop results from the front of a results list. _on_call must understand
            # this ordering too.
            self._pending_calls.append(QAppThreadCall.CallArgs(args))
            QMetaObject.invokeMethod(self, "_on_call", connection_type)

            # Only return/re-raise exceptions in the blocking case or else the
            # user has moved on
            if self._blocking:
                call_result = self._completed_calls.pop(0)
                if call_result.exc_info is not None:
                    raise call_result.exc_info[1].with_traceback(call_result.exc_info[2])
                else:
                    return call_result.result
            else:
                # It makes little sense to return anything from a non-blocking call
                # as the client cannot have waited for it
                return None

    @Slot()
    def _on_call(self) -> None:
        """Perform a call to a GUI function across a
        thread. All exceptions are caught internally and stored to be propagated
        """
        # Changes to this method will very likely require changes to the __call__
        # method, especially w.r.t how the arguments and results are queued.
        result, exc_info = None, None
        try:
            call_info = self._pending_calls.pop(0)
            result = self._callable(*call_info.args)
        except Exception:  # pylint: disable=broad-except
            exc_info = sys.exc_info()

        self._completed_calls.append(QAppThreadCall.CallResult(result, exc_info))

    # private api
    def _ensure_self_on_qapp_thread(self):
        """Assuming the QApplication instance exists, ensure this object is on
        that thread"""
        if self._moved_to_app:
            return

        self.moveToThread(QApplication.instance().thread())
        self._moved_to_app = True


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
        if all_methods or not name.startswith("_"):
            setattr(instance, name, QAppThreadCall(method))

    return instance
