# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from matplotlib.backends.qt_compat import is_pyqt5
from mantidqt.icons import get_icon
if is_pyqt5():
    from matplotlib.backends.backend_qt5agg import (
        NavigationToolbar2QT as NavigationToolbar)
else:
    from matplotlib.backends.backend_qt4agg import (
        NavigationToolbar2QT as NavigationToolbar)

from qtpy import QtCore, QtWidgets

class PlotToolbar(NavigationToolbar):
    def __init__(self, figure_canvas, parent=None):
        self.toolitems = (('Home', 'Reset original view', 'mdi.home', 'home'),
                          ('Back', 'Back to previous view', 'mdi.arrow-left', 'back'),
                          ('Forward', 'Forward to next view', 'mdi.arrow-right', 'forward'),
                          (None, None, None, None),
                          ('Pan', 'Pan axes with left mouse, zoom with right', 'mdi.arrow-all', 'pan'),
                          ('Zoom', 'Zoom to rectangle', 'mdi.magnify', 'zoom'),
                          (None, None, None, None),
                          ('Show major','Show major gridlines','mdi.grid-large','show_major_gridlines'),
                          ('Show minor','Show minor gridlines','mdi.grid','show_minor_gridlines' ),
                          (None, None, None, None),
                          ('Subplots', 'Edit subplots', 'mdi.settings', 'configure_subplots'),
                          ('Save', 'Save the figure', 'mdi.content-save', 'save_figure'),
                          (None, None, None, None),
                          ('Show/hide legend', 'Toggles the legend on/off', None, 'toggle_legend'),
                          )
        self.is_major_grid_on = False
        self.is_minor_grid_on = False
        NavigationToolbar.__init__(self, figure_canvas, parent=parent)

    def _init_toolbar(self):
        for text, tooltip_text, mdi_icon, callback in self.toolitems:
            if text is None:
                self.addSeparator()
            else:

                if mdi_icon:
                    a = self.addAction(get_icon(mdi_icon), text, getattr(self, callback))
                else:
                    a = self.addAction(text, getattr(self, callback))
                self._actions[callback] = a
                if tooltip_text is not None:
                    a.setToolTip(tooltip_text)

        if self.coordinates:
            self.locLabel = QtWidgets.QLabel("", self)
            self.locLabel.setAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignTop)
            self.locLabel.setSizePolicy(QtWidgets.QSizePolicy(QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Expanding,
                                                                                    QtWidgets.QSizePolicy.Ignored)))
            labelAction = self.addWidget(self.locLabel)
            labelAction.setVisible(True)

        # Adjust icon size or they are too small in PyQt5 by default
        dpi_ratio = QtWidgets.QApplication.instance().desktop().physicalDpiX() / 100
        self.setIconSize(QtCore.QSize(24 * dpi_ratio, 24 * dpi_ratio))

    def toggle_legend(self):
        for ax in self.canvas.figure.get_axes():
            if ax.get_legend() is not None:
                ax.get_legend().set_visible(not ax.get_legend().get_visible())
        self.canvas.figure.tight_layout()
        self.canvas.draw()

    def show_major_gridlines(self):
        if self.is_major_grid_on:
            for ax in self.canvas.figure.get_axes():
                ax.grid(False)
            self.is_major_grid_on = False
        else:
            for ax in self.canvas.figure.get_axes():
                ax.grid(True , color='black')
            self.is_major_grid_on = True
        self.canvas.draw()

    def show_minor_gridlines(self):
        if self.is_minor_grid_on:
            for ax in self.canvas.figure.get_axes():
                ax.minorticks_off()
            self.is_major_grid_on = False
        else:
            for ax in self.canvas.figure.get_axes():
                ax.minorticks_on()
                ax.grid(which='minor')
            self.is_major_grid_on = True
        self.canvas.draw()