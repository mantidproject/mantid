#  This file is part of the mantid workbench.
#
#  Copyright (C) 2018 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
        self.qApp = QApplication.instance()

        QObject.__init__(self)
        self.moveToThread(self.qApp.thread())
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
