# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""Provides the QMainWindow subclass for a plotting window"""
from __future__ import absolute_import

# std imports
import platform
import weakref

# 3rdparty imports
from qtpy.QtCore import QEvent, Qt, Signal, Slot
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QApplication, QMainWindow

# local imports
from mantidqt.plotting.figuretype import FigureType, figure_type
from mantidqt.widgets.observers.observing_view import ObservingView
from workbench.app import MAIN_WINDOW_OBJECT_NAME


class FigureWindow(QMainWindow, ObservingView):
    """A MainWindow that will hold plots"""
    activated = Signal()
    closing = Signal()
    visibility_changed = Signal()
    show_context_menu = Signal()
    close_signal = Signal()

    def __init__(self, canvas, parent=None):
        QMainWindow.__init__(self, parent=parent)
        # attributes
        self._canvas = weakref.proxy(canvas)
        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))

        # On Windows, setting the Workbench's main window as this window's
        # parent always keeps this window on top, but still allows minimization.
        # On Ubuntu the child is NOT kept above the parent, hence we use the
        # focusWindowChanged event to bring this window back to the top when
        # the main window gets focus. This does cause a slight flicker effect
        # as the window is hidden and quickly brought back to the front. Using
        # the parent-child method at least avoids this flicker on Windows.

        # Using the Qt.WindowStaysOnTopFlag was tried, however this caused the
        # window to stay on top of all other windows, including external
        # applications. This flag could be toggled off when the application
        # was inactive, however a QWindow needs to be re-drawn when it is given
        # new flags, which again, causes a flicker.

        # Using the Qt.Tool flag, and setting the main window as this window's
        # parent, keeps this window on top. However it does not allow the
        # window to be minimized.
        if platform.system() == "Windows":
            from workbench.utils.windowfinder import get_main_window_widget
            self.setParent(get_main_window_widget(), Qt.Window)
        else:
            QApplication.instance().focusWindowChanged.connect(self._on_focusWindowChanged)
        self.close_signal.connect(self._run_close)
        self.setAcceptDrops(True)

    @Slot()
    def _run_close(self):
        self.close()

    def event(self, event):
        if event.type() == QEvent.WindowActivate:
            self.activated.emit()
        return QMainWindow.event(self, event)

    def closeEvent(self, event):
        self.closing.emit()
        QMainWindow.closeEvent(self, event)
        self.deleteLater()

    def hideEvent(self, event):
        self.visibility_changed.emit()
        QMainWindow.hideEvent(self, event)

    def showEvent(self, event):
        self.visibility_changed.emit()
        QMainWindow.showEvent(self, event)

    def dragEnterEvent(self, event):
        """
        Accepts drag events if the event contains text and no
        urls.

        :param event: A QDragEnterEvent instance for the most
                      recent drag
        """
        data = event.mimeData()
        if data.hasText() and not data.hasUrls():
            event.acceptProposedAction()

    def dropEvent(self, event):
        """
        If the event data contains a workspace reference then
        request a plot of the workspace.

        :param event: A QDropEvent containing the MIME
                      data of the action
        """
        from matplotlib.backend_bases import LocationEvent
        workspace_names = event.mimeData().text().split('\n')

        # This creates a matplotlib LocationEvent so that the axis in which the
        # drop event occurred can be calculated
        try:
            x, y = self._canvas.mouseEventCoords(event.pos())
        except AttributeError:  # matplotlib v1.5 does not have mouseEventCoords
            try:
                dpi_ratio = self._canvas.devicePixelRatio() or 1
            except AttributeError:
                dpi_ratio = 1
            x = dpi_ratio*event.pos().x()
            y = dpi_ratio*self._canvas.figure.bbox.height/dpi_ratio - event.pos().y()

        location_event = LocationEvent('AxesGetterEvent', self._canvas, x, y)
        ax = location_event.inaxes if location_event.inaxes else self._canvas.figure.axes[0]

        self._plot_on_here(workspace_names, ax)
        QMainWindow.dropEvent(self, event)

    # private api

    def _plot_on_here(self, names, ax):
        """
        Assume the list of strings refer to workspace names and they are to be plotted
        on this figure. If the current figure contains an image plot then
        a new image plot will replace the current image. If the current figure
        contains a line plot then the user will be asked what should be plotted and this
        will overplot onto the figure. If the first line of the plot
        :param names: A list of workspace names
        :param ax: The matplotlib axes object to overplot onto
        """
        if len(names) == 0:
            return
        # local import to avoid circular import with FigureManager
        from mantidqt.plotting.functions import pcolormesh_from_names, plot_from_names

        fig = self._canvas.figure
        fig_type = figure_type(fig, ax)
        if fig_type == FigureType.Image:
            pcolormesh_from_names(names, fig=fig, ax=ax)
        else:
            plot_from_names(names, errors=(fig_type == FigureType.Errorbar),
                            overplot=ax, fig=fig)

    def _on_focusWindowChanged(self, window):
        """
        The figure window should always remain on top of the main
        Workbench window.
        """
        # We hook into focusWindowChanged instead of focusChanged here.
        # focusChanged returns the widget that now has focus. We can determine
        # if this widget is a child of the main window but, we do not want to
        # raise this window above all children of the main window,e.g. matrix
        # workspace display. We only want to raise the window above the main
        # window's window.

        # The window object returned here is a QtGui.QWindow https://doc.qt.io/qt-5/qwindow.html
        # which only has access to the widget whose focus is being changed to,
        # e.g. MessageDisplay, but for reasons given above we cannot use this.
        # The object name of the window appears to be the object name of the
        # window's central widget with "Window" appended to it; so we check for
        # this.
        if window and MAIN_WINDOW_OBJECT_NAME + 'Window' == window.objectName():
            self.raise_()
