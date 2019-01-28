# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#

import sys

from qtpy.QtCore import Qt, QMetaObject, QObject, QThread, Slot
from qtpy.QtWidgets import QApplication

from six import reraise


class QAppThreadCall(QObject):
    """
    Wraps a callable object and forces any calls made to it to be executed
    on the same thread as the qApp object. This is required for anything
    called by the matplotlib figures, which run on a separate thread.
    """

    def __init__(self, callee):
        QObject.__init__(self)
        qapp = QApplication.instance()
        if qapp is not None:
            self.moveToThread(qapp.thread())
            self.qApp = qapp
        self.callee = callee
        # Help should then give the correct doc
        self.__call__.__func__.__doc__ = callee.__doc__
        self._args = None
        self._kwargs = None
        self._result = None
        self._exc_info = None

    def __call__(self, *args, **kwargs):
        """
        If the current thread is the qApp thread then this
        performs a straight call to the wrapped callable_obj. Otherwise
        it invokes the do_call method as a slot via a
        BlockingQueuedConnection.
        """
        if QThread.currentThread() == self.qApp.thread():
            return self.callee(*args, **kwargs)
        else:
            self._store_function_args(*args, **kwargs)
            QMetaObject.invokeMethod(self, "on_call",
                                     Qt.BlockingQueuedConnection)
            if self._exc_info is not None:
                reraise(*self._exc_info)
            return self._result

    @Slot()
    def on_call(self):
        """Perform a call to a GUI function across a
        thread and return the result
        """
        try:
            self._result = \
                self.callee(*self._args, **self._kwargs)
        except Exception: # pylint: disable=broad-except
            self._exc_info = sys.exc_info()

    def _store_function_args(self, *args, **kwargs):
        self._args = args
        self._kwargs = kwargs
        # Reset return value and exception
        self._result = None
        self._exc_info = None
