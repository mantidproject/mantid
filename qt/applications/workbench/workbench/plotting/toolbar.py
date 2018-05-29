#    This file is part of the mantid workbench.
#
#    Copyright (C) 2017 mantidproject
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

# system imports

# third-party library imports
from matplotlib.backends.backend_qt5 import NavigationToolbar2QT
import qtawesome as qta
from qtpy import QtCore, QtGui, QtPrintSupport, QtWidgets

# local package imports


class WorkbenchNavigationToolbar(NavigationToolbar2QT):

    sig_grid_toggle_triggered = QtCore.Signal()
    sig_active_triggered = QtCore.Signal()
    sig_hold_triggered = QtCore.Signal()

    toolitems = (
        ('Home', 'Reset original view', 'fa.home', 'home', None),
        ('Pan', 'Pan axes with left mouse, zoom with right', 'fa.arrows-alt', 'pan', False),
        ('Zoom', 'Zoom to rectangle', 'fa.search-plus', 'zoom', False),
        (None, None, None, None, None),
        ('Grid', 'Toggle grid on/off', None, 'toggle_grid', False),
        ('Save', 'Save the figure', 'fa.save', 'save_figure', None),
        ('Print','Print the figure', 'fa.print', 'print_figure', None),
        (None, None, None, None, None),
        ('Customize', 'Configure plot options', 'fa.cog', 'edit_parameters', None)
    )

    def _init_toolbar(self):
        for text, tooltip_text, fa_icon, callback, checked in self.toolitems:
            if text is None:
                self.addSeparator()
            else:
                if fa_icon:
                    a = self.addAction(qta.icon(fa_icon),
                                       text, getattr(self, callback))
                else:
                    a = self.addAction(text, getattr(self, callback))
                self._actions[callback] = a
                if checked is not None:
                    a.setCheckable(True)
                    a.setChecked(checked)
                if tooltip_text is not None:
                    a.setToolTip(tooltip_text)

        self.buttons = {}
        # Add the x,y location widget at the right side of the toolbar
        # The stretch factor is 1 which means any resizing of the toolbar
        # will resize this label instead of the buttons.
        if self.coordinates:
            self.locLabel = QtWidgets.QLabel("", self)
            self.locLabel.setAlignment(
                    QtCore.Qt.AlignRight | QtCore.Qt.AlignTop)
            self.locLabel.setSizePolicy(
                QtWidgets.QSizePolicy(QtWidgets.Expanding,
                                      QtWidgets.QSizePolicy.Ignored))
            labelAction = self.addWidget(self.locLabel)
            labelAction.setVisible(True)

        # reference holder for subplots_adjust window
        self.adj_window = None

        # Adjust icon size or they are too small in PyQt5 by default
        self.setIconSize(QtCore.QSize(24, 24))

    def toggle_grid(self):
        self.sig_grid_toggle_triggered.emit()

    def print_figure(self):
        printer = QtPrintSupport.QPrinter(QtPrintSupport.QPrinter.HighResolution)
        printer.setOrientation(QtPrintSupport.QPrinter.Landscape)
        print_dlg = QtPrintSupport.QPrintDialog(printer)
        if print_dlg.exec_() == QtWidgets.QDialog.Accepted:
            painter = QtGui.QPainter(printer)
            page_size = printer.pageRect()
            pixmap = self.canvas.grab().scaled(page_size.width(), page_size.height(),
                                               QtCore.Qt.KeepAspectRatio)
            painter.drawPixmap(0, 0, pixmap)
            painter.end()
