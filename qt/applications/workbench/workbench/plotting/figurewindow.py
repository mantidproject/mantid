# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""Provides the QMainWindow subclass for a plotting window"""
# std imports
import weakref

# 3rdparty imports
from matplotlib.collections import LineCollection, QuadMesh
from mpl_toolkits.mplot3d.axes3d import Axes3D
from mpl_toolkits.mplot3d.art3d import Line3DCollection, Poly3DCollection
from qtpy.QtCore import QEvent, Qt, Signal, Slot
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QMainWindow

# local imports
from mantidqt.plotting.figuretype import FigureType, figure_type
from mantidqt.widgets.observers.observing_view import ObservingView


class FigureWindow(QMainWindow, ObservingView):
    """A MainWindow that will hold plots"""
    # signals
    activated = Signal()
    closing = Signal()
    visibility_changed = Signal()
    show_context_menu = Signal()
    close_signal = Signal()

    def __init__(self, canvas, parent=None, window_flags=None):
        if window_flags is not None:
            QMainWindow.__init__(self, parent, window_flags)
        else:
            QMainWindow.__init__(self, parent)
        # attributes
        self._canvas = weakref.proxy(canvas)
        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))

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
            x = dpi_ratio * event.pos().x()
            y = dpi_ratio * self._canvas.figure.bbox.height / dpi_ratio - event.pos().y()

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
        from mantidqt.plotting.functions import pcolormesh, plot_from_names, plot_surface, plot_wireframe

        fig = self._canvas.figure
        fig_type = figure_type(fig, ax)
        if fig_type == FigureType.Image:
            # if the axes that a workspace has been dragged onto is a colorbar (a colorbar is identified by having a
            # QuadMesh), find the other axes on the figure that the colorbar 'belongs' to, so the plot type can be
            # determined.
            if any(isinstance(col, QuadMesh) for col in ax.collections):
                for axes in fig.get_axes():
                    if not any(isinstance(col, QuadMesh) for col in axes.collections):
                        ax = axes
                        break

            if isinstance(ax, Axes3D):
                if any(isinstance(col, Poly3DCollection) for col in ax.collections):
                    plot_surface(names, fig=fig)
                elif any(isinstance(col, Line3DCollection) for col in ax.collections):
                    plot_wireframe(names, fig=fig)
            else:
                pcolormesh(names, fig=fig, contour=any(isinstance(col, LineCollection) for col in ax.collections))
        else:
            plot_from_names(names, errors=(fig_type == FigureType.Errorbar),
                            overplot=ax, fig=fig)
