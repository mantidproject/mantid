# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import (absolute_import, unicode_literals)

import numpy as np

from matplotlib.path import Path
from matplotlib.patches import PathPatch
import matplotlib.patheffects as path_effects
from qtpy.QtCore import Signal, Qt
from qtpy.QtGui import QGuiApplication, QCursor

from mantidqt.utils.qt import import_qt


BaseBrowser = import_qt('.._common', 'mantidqt.widgets', 'FitPropertyBrowser')


class FitPropertyBrowser(BaseBrowser):

    closing = Signal()

    def __init__(self, canvas, parent=None):
        super(FitPropertyBrowser, self).__init__(parent)
        self.init()
        self.canvas = canvas
        self.tool = None

    def closeEvent(self, event):
        self.closing.emit()
        BaseBrowser.closeEvent(self, event)

    def show(self):
        self.tool = FitInteractiveTool(self.canvas)
        super(FitPropertyBrowser, self).show()

    def hide(self):
        if self.tool is not None:
            self.tool.disconnect()
        super(FitPropertyBrowser, self).hide()


class VerticalMarker(object):

    def __init__(self, canvas, x, color):
        self.x = x
        self.ax = canvas.figure.get_axes()[0]
        y0, y1 = self.ax.get_ylim()
        path = Path([(x, y0), (x, y1)], [Path.MOVETO, Path.LINETO])
        self.patch = PathPatch(path, facecolor='None', edgecolor=color, picker=5, linewidth=2.0, animated=True,
                               # path_effects=[path_effects.SimpleLineShadow(), path_effects.Normal()]
                               )
        self.ax.add_patch(self.patch)

    def remove(self):
        self.patch.remove()

    def redraw(self):
        y0, y1 = self.ax.get_ylim()
        vertices = self.patch.get_path().vertices
        vertices[0] = self.x, y0
        vertices[1] = self.x, y1
        self.ax.draw_artist(self.patch)

    def is_above(self, xd):
        x, y = self.patch.get_transform().transform((self.x, 0))
        return np.abs(x - xd) < 3


class FitInteractiveTool(object):

    def __init__(self, canvas):
        self.canvas = canvas
        ax = canvas.figure.get_axes()[0]
        self.ax = ax
        xlim = ax.get_xlim()
        dx = (xlim[1] - xlim[0]) / 5.
        start_x = xlim[0] + dx
        end_x = xlim[1] - dx
        self.fit_start_x = VerticalMarker(canvas, start_x, 'green')
        self.fit_end_x = VerticalMarker(canvas, end_x, 'green')

        self._cids = []
        self._cids.append(canvas.mpl_connect('draw_event', self.draw_callback))
        self._cids.append(canvas.mpl_connect('motion_notify_event', self.motion_notify_callback))
        # canvas.mpl_connect('figure_enter_event', self.enter_figure)

    def disconnect(self):
        for cid in self._cids:
            self.canvas.mpl_disconnect(cid)
        print (self.fit_start_x, self.fit_end_x)
        self.fit_start_x.remove()
        self.fit_end_x.remove()

    def draw_callback(self, event):
        self.fit_start_x.redraw()
        self.fit_end_x.redraw()
        # self.canvas.blit(self.ax.bbox)

    def motion_notify_callback(self, event):
        if self.fit_start_x.is_above(event.x) or self.fit_end_x.is_above(event.x):
            QGuiApplication.setOverrideCursor(QCursor(Qt.SizeHorCursor))
        else:
            QGuiApplication.restoreOverrideCursor()
