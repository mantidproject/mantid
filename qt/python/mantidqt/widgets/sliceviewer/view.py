# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)
from qtpy.QtWidgets import QWidget, QVBoxLayout
from qtpy.QtCore import Qt
from matplotlib.figure import Figure
from MPLwidgets import FigureCanvas, NavigationToolbar2QT as NavigationToolbar
from .dimensionwidget import DimensionWidget


class SliceViewerView(QWidget):
    def __init__(self, presenter, dims_info, parent=None):
        super(SliceViewerView, self).__init__(parent)

        self.presenter = presenter

        self.setWindowTitle("SliceViewer")
        self.setWindowFlags(Qt.Window)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        # Dimension widget
        self.dimensions = DimensionWidget(dims_info, parent=self)
        self.dimensions.dimensionsChanged.connect(self.presenter.new_plot)
        self.dimensions.valueChanged.connect(self.presenter.update_plot_data)

        # MPL figure
        self.fig = Figure()
        self.fig.set_tight_layout(True)
        self.canvas = FigureCanvas(self.fig)
        self.ax = self.fig.add_subplot(111, projection='mantid')

        # MPL toolbar
        self.mpl_toolbar = NavigationToolbar(self.canvas, self)

        # layout
        self.layout = QVBoxLayout(self)
        self.layout.addWidget(self.dimensions)
        self.layout.addWidget(self.mpl_toolbar)
        self.layout.addWidget(self.canvas, stretch=1)

        self.show()

    def plot(self, ws, **kwargs):
        """
        clears the plot and creates a new one using the workspace
        """
        self.ax.clear()
        try:
            self.colorbar.remove()
        except AttributeError:
            pass
        self.im = self.ax.imshow(ws, origin='lower', **kwargs)
        self.ax.set_title('')
        self.colorbar = self.fig.colorbar(self.im)
        self.mpl_toolbar.update() # clear nav stack
        self.fig.canvas.draw_idle()

    def update_plot_data(self, data):
        """
        This just updates the plot data without creating a new plot
        """
        self.im.set_data(data.T)
        self.im.set_clim(data.min(), data.max())
        self.fig.canvas.draw_idle()

    def closeEvent(self, event):
        self.deleteLater()
        super(SliceViewerView, self).closeEvent(event)
