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
from qtpy.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout
from qtpy.QtCore import Qt
from mantidqt.MPLwidgets import FigureCanvas, NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
from .dimensionwidget import DimensionWidget
from mantidqt.widgets.colorbar.colorbar import ColorbarWidget
from mantidqt.plotting.functions import use_imshow


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

        # MPL figure + colorbar
        self.mpl_layout = QHBoxLayout()
        self.fig = Figure()
        self.fig.set_facecolor(self.palette().window().color().getRgbF())
        self.fig.set_tight_layout(True)
        self.canvas = FigureCanvas(self.fig)
        self.ax = self.fig.add_subplot(111, projection='mantid')
        self.mpl_layout.addWidget(self.canvas)
        self.colorbar = ColorbarWidget(self)
        self.colorbar.colorbarChanged.connect(self.canvas.draw_idle)
        self.mpl_layout.addWidget(self.colorbar)

        # MPL toolbar
        self.mpl_toolbar = NavigationToolbar(self.canvas, self)

        # layout
        self.layout = QVBoxLayout(self)
        self.layout.addWidget(self.dimensions)
        self.layout.addWidget(self.mpl_toolbar)
        self.layout.addLayout(self.mpl_layout, stretch=1)

        self.show()

    def plot_MDH(self, ws, **kwargs):
        """
        clears the plot and creates a new one using a MDHistoWorkspace
        """
        self.ax.clear()
        self.im = self.ax.imshow(ws, origin='lower', aspect='auto',
                                 transpose=self.dimensions.transpose,
                                 norm=self.colorbar.get_norm(), **kwargs)
        self.ax.set_title('')
        self.colorbar.set_mappable(self.im)
        self.mpl_toolbar.update() # clear nav stack
        self.canvas.draw_idle()

    def plot_matrix(self, ws, **kwargs):
        """
        clears the plot and creates a new one using a MatrixWorkspace
        """
        self.ax.clear()
        if use_imshow(ws):
            self.im = self.ax.imshow(ws, origin='lower', aspect='auto',
                                     transpose=self.dimensions.transpose,
                                     norm=self.colorbar.get_norm(), **kwargs)
        else:
            self.im = self.ax.pcolormesh(ws, transpose=self.dimensions.transpose,
                                         norm=self.colorbar.get_norm(), **kwargs)
        self.ax.set_title('')
        self.colorbar.set_mappable(self.im)
        self.mpl_toolbar.update() # clear nav stack
        self.canvas.draw_idle()

    def update_plot_data(self, data):
        """
        This just updates the plot data without creating a new plot
        """
        self.im.set_data(data.T)
        self.colorbar.update_clim()

    def closeEvent(self, event):
        self.deleteLater()
        super(SliceViewerView, self).closeEvent(event)
