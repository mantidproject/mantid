# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#

from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

# system imports

# third-party library imports
from matplotlib.backends.backend_qt5 import NavigationToolbar2QT
from mantidqt.icons import get_icon
from qtpy import QtCore, QtGui, QtPrintSupport, QtWidgets

# local package imports


class WorkbenchNavigationToolbar(NavigationToolbar2QT):

    sig_grid_toggle_triggered = QtCore.Signal()
    sig_active_triggered = QtCore.Signal()
    sig_hold_triggered = QtCore.Signal()
    sig_toggle_fit_triggered = QtCore.Signal()

    toolitems = (
        ('Home', 'Reset original view', 'fa.home', 'home', None),
        ('Pan', 'Pan axes with left mouse, zoom with right', 'fa.arrows-alt', 'pan', False),
        ('Zoom', 'Zoom to rectangle', 'fa.search-plus', 'zoom', False),
        (None, None, None, None, None),
        ('Grid', 'Toggle grid on/off', None, 'toggle_grid', False),
        ('Save', 'Save the figure', 'fa.save', 'save_figure', None),
        ('Print','Print the figure', 'fa.print', 'print_figure', None),
        (None, None, None, None, None),
        ('Customize', 'Configure plot options', 'fa.cog', 'edit_parameters', None),
        (None, None, None, None, None),
        ('Fit', 'Toggle fit browser on/off', None, 'toggle_fit', False),
    )

    def _init_toolbar(self):
        for text, tooltip_text, fa_icon, callback, checked in self.toolitems:
            if text is None:
                self.addSeparator()
            else:
                if fa_icon:
                    a = self.addAction(get_icon(fa_icon),
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

    def toggle_fit(self):
        fit_action = self._actions['toggle_fit']
        if fit_action.isChecked():
            if self._actions['zoom'].isChecked():
                self.zoom()
            if self._actions['pan'].isChecked():
                self.pan()
        self.sig_toggle_fit_triggered.emit()

    def trigger_fit_toggle_action(self):
        self._actions['toggle_fit'].trigger()

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

    def contextMenuEvent(self, event):
        pass


class ToolbarStateChecker(object):
    """
    An object that lets users check the state of the toolbar hiding at the same time any implementation details.
    """

    def __init__(self, toolbar):
        self._toolbar = toolbar

    def is_zoom_active(self):
        """
        Check if the Zoom button is checked
        """
        return self._toolbar._actions['zoom'].isChecked()

    def is_pan_active(self):
        """
        Check if the Pan button is checked
        """
        return self._toolbar._actions['pan'].isChecked()

    def is_tool_active(self):
        """
        Check if any of the zoom buttons are checked
        """
        return self.is_pan_active() or self.is_zoom_active()
