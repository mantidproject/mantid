#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
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
"""
Qt-based matplotlib backend that can operate when called from non-gui threads.

It uses qtagg for rendering but the ensures that any rendering calls
are done on the main thread of the application as the default
"""
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

# std imports
import importlib

# 3rd party imports
# Put these first so that the correct Qt version is selected by qtpy
from qtpy import QT_VERSION
from qtpy.QtWidgets import QApplication

# local imports
from workbench.plotting.figuremanager import new_figure_manager, new_figure_manager_given_figure, show  # noqa

if not QApplication.instance():
    raise ImportError("The 'qt_async_backend' requires an QApplication object to have been created")

# Import the *real* matplotlib backend for the canvas
mpl_qtagg_backend = importlib.import_module('matplotlib.backends.backend_qt{}agg'.format(QT_VERSION[0]))
try:
    FigureCanvas = getattr(mpl_qtagg_backend, 'FigureCanvasQTAgg')
except KeyError:
    raise ImportError("Unknown form of matplotlib Qt backend.")


# -----------------------------------------------------------------------------
# Threading helpers
# -----------------------------------------------------------------------------
# class QAppThreadCall(QObject):
#     """
#     Wraps a callable object and forces any calls made to it to be executed
#     on the same thread as the qApp object.
#     """
#
#     def __init__(self, callee):
#         QObject.__init__(self)
#         self.moveToThread(qApp.thread())
#         self.callee = callee
#         # Help should then give the correct doc
#         self.__call__.__func__.__doc__ = callee.__doc__
#         self._args = None
#         self._kwargs = None
#         self._result = None
#         self._exc_info = None
#
#     def __call__(self, *args, **kwargs):
#         """
#         If the current thread is the qApp thread then this
#         performs a straight call to the wrapped callable_obj. Otherwise
#         it invokes the do_call method as a slot via a
#         BlockingQueuedConnection.
#         """
#         if QThread.currentThread() == qApp.thread():
#             return self.callee(*args, **kwargs)
#         else:
#             self._store_function_args(*args, **kwargs)
#             QMetaObject.invokeMethod(self, "on_call",
#                                      Qt.BlockingQueuedConnection)
#             if self._exc_info is not None:
#                 reraise(*self._exc_info)
#             return self._result
#
#     @Slot()
#     def on_call(self):
#         """Perform a call to a GUI function across a
#         thread and return the result
#         """
#         try:
#             self._result = \
#                 self.callee(*self._args, **self._kwargs)
#         except Exception: #pylint: disable=broad-except
#             self._exc_info = sys.exc_info()
#
#     def _store_function_args(self, *args, **kwargs):
#         self._args = args
#         self._kwargs = kwargs
#         # Reset return value and exception
#         self._result = None
#         self._exc_info = None


# -----------------------------------------------------------------------------
# Backend implementation
# -----------------------------------------------------------------------------


# # Wrap other required calls
# show = QAppThreadCall(mpl_qtagg_backend.show)
#
#
# def _new_figure_manager_impl(num, *args, **kwargs):
#     """
#     Create a new figure manager instance
#     """
#
#     figure_class = kwargs.pop('FigureClass', Figure)
#     this_fig = figure_class(*args, **kwargs)
#     return new_figure_manager_given_figure(num, this_fig)
#
#
# def _new_figure_manager_given_figure_impl(num, figure):
#     """
#     Create a new figure manager instance for the given figure.
#     """
#     canvas = FigureCanvas(figure)
#     manager = FigureManager(canvas, num)
#     return manager
#
# new_figure_manager = QAppThreadCall(_new_figure_manager_impl)
# new_figure_manager_given_figure = QAppThreadCall(_new_figure_manager_given_figure_impl)
