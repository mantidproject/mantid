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
"""Provides the QMainWindow subclass for a plotting window"""
from __future__ import absolute_import

# std imports
import weakref

# 3rdparty imports
from qtpy.QtCore import QEvent, Signal
from qtpy.QtWidgets import QMainWindow

# local imports
from .figuretype import figure_type, FigureType


class FigureWindow(QMainWindow):
    """A MainWindow that will hold plots"""
    activated = Signal()
    closing = Signal()
    visibility_changed = Signal()

    def __init__(self, canvas, parent=None):
        QMainWindow.__init__(self, parent=parent)
        # attributes
        self._canvas = weakref.proxy(canvas)

        self.setAcceptDrops(True)

    def event(self, event):
        if event.type() == QEvent.WindowActivate:
            self.activated.emit()
        return QMainWindow.event(self, event)

    def closeEvent(self, event):
        self.closing.emit()
        QMainWindow.closeEvent(self, event)

    def hideEvent(self, event):
        self.visibility_changed.emit()
        QMainWindow.hideEvent(self, event)

    def showEvent(self, event):
        self.visibility_changed.emit()
        QMainWindow.showEvent(self, event)

