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
import weakref

# 3rdparty imports
from qtpy.QtCore import QEvent, Qt, Signal, Slot
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QMainWindow

# local imports
from mantidqt.plotting.figuretype import FigureType, figure_type
from mantidqt.widgets.observers.observing_view import ObservingView


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
        workspace_names = event.mimeData().text().split('\n')
        self._plot_on_here(workspace_names)
        QMainWindow.dropEvent(self, event)

    # private api

    def _plot_on_here(self, names):
        """
        Assume the list of strings refer to workspace names and they are to be plotted
        on this figure. If the current figure contains an image plot then
        a new image plot will replace the current image. If the current figure
        contains a line plot then the user will be asked what should be plotted and this
        will overplot onto the figure. If the first line of the plot
        :param names: A list of workspace names
        """
        if len(names) == 0:
            return
        # local import to avoid circular import with FigureManager
        from mantidqt.plotting.functions import pcolormesh_from_names, plot_from_names

        fig = self._canvas.figure
        fig_type = figure_type(fig)
        if fig_type == FigureType.Image:
            pcolormesh_from_names(names, fig=fig)
        else:
            plot_from_names(names, errors=(fig_type == FigureType.Errorbar),
                            overplot=True, fig=fig)
