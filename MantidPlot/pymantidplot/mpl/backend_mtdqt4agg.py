"""
Matplotlib backend for MantidPlot.

It uses qt4agg for rendering but the ensures that any rendering calls
are done on the main thread of the application as the default
mode for MantidPlot is to run all python scripts asynchronously.
Providing this backend allows users to work with the standard matplotlib
API without modification
"""
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

try:
    # Newer matplotlib has qt_compat
    from matplotlib.backends.qt_compat import QtCore, QtWidgets
except ImportError:
    # whereas older has qt4_compat with no QtWidgets
    from matplotlib.backends.qt4_compat import QtCore
    from matplotlib.backends.qt4_compat import QtGui as QtWidgets
# Import everything from the *real* matplotlib backend
from matplotlib.backends.backend_qt4agg import *
import six

# Remove the implementations of new_figure_manager_*. We replace them below
del new_figure_manager
try:
    # v<1.2 didn't have this method and had a different
    # implementation of new_figure_manager. Use the absence
    # this to detect old versions
    del new_figure_manager_given_figure
    MPL_HAVE_GIVEN_FIG_METHOD = True
except NameError:
    MPL_HAVE_GIVEN_FIG_METHOD = False


class QAppThreadCall(QtCore.QObject):
    """
    Wraps a callable object and forces any calls made to it to be executed
    on the same thread as the QtGui.qApp object.
    """

    def __init__(self, callee):
        QtCore.QObject.__init__(self)
        self.moveToThread(QtWidgets.qApp.thread())
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
        if QtCore.QThread.currentThread() == QtWidgets.qApp.thread():
            return self.callee(*args, **kwargs)
        else:
            self._store_function_args(*args, **kwargs)
            QtCore.QMetaObject.invokeMethod(self, "on_call",
                                            QtCore.Qt.BlockingQueuedConnection)
            if self._exc_info is not None:
                six.reraise(*self._exc_info)
            return self._result

    @QtCore.pyqtSlot()
    def on_call(self):
        """Perform a call to a GUI function across a
        thread and return the result
        """
        try:
            self._result = \
                self.callee(*self._args, **self._kwargs)
        except Exception as exc: #pylint: disable=broad-except
            import sys
            self._exc_info = sys.exc_info()

    def _store_function_args(self, *args, **kwargs):
        self._args = args
        self._kwargs = kwargs
        # Reset return value and exception
        self._result = None
        self._exc_info = None


class ThreadAwareFigureManagerQT(FigureManagerQT):
    """Our own FigureManager that ensures the destroy method
    is invoked on the main Qt thread"""

    def __init__(self, canvas, num):
        FigureManagerQT.__init__(self, canvas, num)
        self._destroy_orig = self.destroy
        self.destroy = QAppThreadCall(self._destroy_orig)


# ----------------------------------------------------------------------------------------------------------------------
# Wrap the required functions
show = QAppThreadCall(show)
# Use our figure manager
FigureManager = ThreadAwareFigureManagerQT

if MPL_HAVE_GIVEN_FIG_METHOD:
    def _new_figure_manager_impl(num, *args, **kwargs):
        """
        Create a new figure manager instance
        """
        figure_class = kwargs.pop('FigureClass', Figure)
        this_fig = figure_class(*args, **kwargs)
        return new_figure_manager_given_figure(num, this_fig)


    def _new_figure_manager_given_figure_impl(num, figure):
        """
        Create a new figure manager instance for the given figure.
        """
        canvas = FigureCanvasQTAgg(figure)
        manager = ThreadAwareFigureManagerQT(canvas, num)
        return manager


    new_figure_manager_given_figure = QAppThreadCall(_new_figure_manager_given_figure_impl)
else:
    def _new_figure_manager_impl(num, *args, **kwargs):
        """
        Create a new figure manager instance
        """
        figure_class = kwargs.pop('FigureClass', Figure)
        this_fig = figure_class(*args, **kwargs)
        canvas = FigureCanvasQTAgg(this_fig)
        return ThreadAwareFigureManagerQT(canvas, num)
# endif

new_figure_manager = QAppThreadCall(_new_figure_manager_impl)
