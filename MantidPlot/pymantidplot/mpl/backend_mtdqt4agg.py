"""
Matplotlib backend for MantidPlot.

It uses qt4agg for rendering but the ensures that any rendering calls
are done on the main thread of the application as the default
mode for MantidPlot is to run all python scripts asynchronously.

Providing this backend allows users to work with the standard matplotlib
API without modification
"""
from __future__ import division, print_function

try:
    from matplotlib.backends.qt_compat import QtCore, QtGui
except ImportError:
    from matplotlib.backends.qt4_compat import QtCore, QtGui

from matplotlib.backends.backend_qt4agg import draw_if_interactive #pylint: disable=unused-import
from matplotlib.backends.backend_qt4agg import new_figure_manager as _new_fig_mgr_qt4agg
from matplotlib.backends.backend_qt4agg import show as _show_qt4agg

class QAppThreadCall(QtCore.QObject):
    """
    Wraps a callable object and forces any calls made to it to be executed
    on the same thread as the QtGui.qApp object.
    """

    def __init__(self, callable_obj):
        QtCore.QObject.__init__(self)
        self.moveToThread(QtGui.qApp.thread())
        self.callable_obj = callable_obj
        # Help should then give the correct doc
        self.__call__.__func__.__doc__ = callable_obj.__doc__
        self._args = None
        self._kwargs = None
        self._retvalue = None
        self._exc = None

    def __call__(self, *args, **kwargs):
        """
        If the current thread is the qApp thread then this
        performs a straight call to the wrapped callable_obj. Otherwise
        it invokes the do_call method as a slot via a
        BlockingQueuedConnection.
        """
        if QtCore.QThread.currentThread() == QtGui.qApp.thread():
            return self.callable_obj(*args, **kwargs)
        else:
            self._clear_func_props()
            self._store_func_props(*args, **kwargs)
            QtCore.QMetaObject.invokeMethod(self, "on_call",
                                            QtCore.Qt.BlockingQueuedConnection)
            if self._exc is not None:
                raise self._exc #pylint: disable=raising-bad-type
            return self._retvalue

    @QtCore.pyqtSlot()
    def on_call(self):
        """Perform a call to a GUI function across a
        thread and return the result
        """
        try:
            self._retvalue = \
                self.callable_obj(*self._args, **self._kwargs)
        except Exception as exc: #pylint: disable=broad-except
            self._exc = exc

    def _clear_func_props(self):
        self._args = None
        self._kwargs = None
        self._retvalue = None
        self._exc = None

    def _store_func_props(self, *args, **kwargs):
        self._args = args
        self._kwargs = kwargs

# Wrap Qt4Agg methods
show = QAppThreadCall(_show_qt4agg) #pylint: disable=invalid-name
new_figure_manager = QAppThreadCall(_new_fig_mgr_qt4agg) #pylint: disable=invalid-name
