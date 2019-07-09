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

    home_clicked = QtCore.Signal()
    sig_grid_toggle_triggered = QtCore.Signal()
    sig_active_triggered = QtCore.Signal()
    sig_hold_triggered = QtCore.Signal()
    sig_toggle_fit_triggered = QtCore.Signal()
    sig_plot_options_triggered = QtCore.Signal()

    toolitems = (
        ('Home', 'Reset original view', 'mdi.home', 'on_home_clicked', None),
        ('Pan', 'Pan axes with left mouse, zoom with right', 'mdi.arrow-all', 'pan', False),
        ('Zoom', 'Zoom to rectangle', 'mdi.magnify-plus-outline', 'zoom', False),
        (None, None, None, None, None),
        ('Grid', 'Toggle grid on/off', 'mdi.grid', 'toggle_grid', False),
        ('Save', 'Save the figure', 'mdi.content-save', 'save_figure', None),
        ('Print','Print the figure', 'mdi.printer', 'print_figure', None),
        (None, None, None, None, None),
        ('Customize', 'Configure plot options', 'mdi.settings', 'launch_plot_options', None),
        (None, None, None, None, None),
        ('Fit', 'Toggle fit browser on/off', None, 'toggle_fit', False),
    )

    def _init_toolbar(self):
        for text, tooltip_text, mdi_icon, callback, checked in self.toolitems:
            if text is None:
                self.addSeparator()
            else:
                if mdi_icon:
                    a = self.addAction(get_icon(mdi_icon),
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

    # TODO remove & use Harry's changes
    # def on_home_clicked(self, *args, **kwargs):
    #     self.home(*args, **kwargs)
    #     self.home_clicked.emit()

    def launch_plot_options(self):
        self.sig_plot_options_triggered.emit()

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


class ToolbarStateManager(object):
    """
    An object that lets users check and manipulate the state of the toolbar
    whilst hiding any implementation details.
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

    def toggle_fit_button_checked(self):
        fit_action = self._toolbar._actions['toggle_fit']
        if fit_action.isChecked():
            fit_action.setChecked(False)
        else:
            fit_action.setChecked(True)
