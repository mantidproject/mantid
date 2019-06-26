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
from mantidqt.MPLwidgets import FigureCanvas
from .toolbar import SliceViewerNavigationToolbar
from matplotlib.figure import Figure
from matplotlib import gridspec
from .dimensionwidget import DimensionWidget
from mantidqt.widgets.colorbar.colorbar import ColorbarWidget
from matplotlib.transforms import Bbox, BboxTransform
import numpy as np
from .samplingimage import imshow_sampling


class SliceViewerView(QWidget):
    def __init__(self, presenter, dims_info, parent=None):
        super(SliceViewerView, self).__init__(parent)

        self.presenter = presenter

        self.setWindowTitle("SliceViewer")
        self.setWindowFlags(Qt.Window)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        self.line_plots = False

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
        self.canvas.mpl_connect('motion_notify_event', self.mouse_move)
        self.create_axes()
        self.mpl_layout.addWidget(self.canvas)
        self.colorbar = ColorbarWidget(self)
        self.colorbar.colorbarChanged.connect(self.update_data_clim)
        self.colorbar.colorbarChanged.connect(self.update_line_plot_limits)
        self.mpl_layout.addWidget(self.colorbar)

        # MPL toolbar
        self.mpl_toolbar = SliceViewerNavigationToolbar(self.canvas, self)
        self.mpl_toolbar.gridClicked.connect(self.toggle_grid)
        self.mpl_toolbar.linePlotsClicked.connect(self.line_plots_toggle)

        # layout
        self.layout = QVBoxLayout(self)
        self.layout.addWidget(self.dimensions)
        self.layout.addWidget(self.mpl_toolbar)
        self.layout.addLayout(self.mpl_layout, stretch=1)

        self.show()

    def create_axes(self):
        self.fig.clf()
        if self.line_plots:
            gs = gridspec.GridSpec(2, 2,
                                   width_ratios=[1, 4],
                                   height_ratios=[4, 1],
                                   wspace=0.0, hspace=0.0)
            self.ax = self.fig.add_subplot(gs[1], projection='mantid')
            self.ax.xaxis.set_visible(False)
            self.ax.yaxis.set_visible(False)
            self.axx=self.fig.add_subplot(gs[3], sharex=self.ax)
            self.axx.yaxis.tick_right()
            self.axy=self.fig.add_subplot(gs[0], sharey=self.ax)
            self.axy.xaxis.tick_top()
        else:
            self.ax = self.fig.add_subplot(111, projection='mantid')
        self.canvas.draw_idle()

    def plot_MDH(self, ws, **kwargs):
        """
        clears the plot and creates a new one using a MDHistoWorkspace
        """
        self.ax.clear()
        self.im = self.ax.imshow(ws, origin='lower', aspect='auto',
                                 transpose=self.dimensions.transpose,
                                 norm=self.colorbar.get_norm(), **kwargs)
        self.draw_plot()

    def plot_matrix(self, ws, **kwargs):
        """
        clears the plot and creates a new one using a MatrixWorkspace
        """
        self.ax.clear()
        self.im = imshow_sampling(self.ax, ws, origin='lower', aspect='auto',
                                  interpolation='none',
                                  transpose=self.dimensions.transpose,
                                  norm=self.colorbar.get_norm(), **kwargs)
        self.im._resample_image()
        self.draw_plot()

    def draw_plot(self):
        self.ax.set_title('')
        self.colorbar.set_mappable(self.im)
        self.colorbar.update_clim()
        self.mpl_toolbar.update() # clear nav stack
        self.clear_line_plots()
        self.canvas.draw_idle()

    def update_plot_data(self, data):
        """
        This just updates the plot data without creating a new plot
        """
        self.im.set_data(data.T)
        self.colorbar.update_clim()

    def line_plots_toggle(self, state):
        self.line_plots = state
        self.clear_line_plots()
        self.presenter.line_plots()

    def clear_line_plots(self):
        try: # clear old plots
            del self.xfig
            del self.yfig
        except AttributeError:
            pass

    def update_data_clim(self):
        self.im.set_clim(self.colorbar.colorbar.get_clim()) # force clim update, needed for RHEL7
        self.canvas.draw_idle()

    def update_line_plot_limits(self):
        if self.line_plots:
            self.axx.set_ylim(self.colorbar.cmin_value, self.colorbar.cmax_value)
            self.axy.set_xlim(self.colorbar.cmin_value, self.colorbar.cmax_value)

    def toggle_grid(self):
        self.ax.grid()
        self.canvas.draw_idle()

    def mouse_move(self, event):
        if event.inaxes == self.ax:
            if self.line_plots:
                self.update_line_plots(event.xdata, event.ydata)

    def plot_x_line(self, x, y):
        try:
            self.xfig[0].set_data(x, y)
        except (AttributeError, IndexError):
            self.axx.clear()
            self.xfig = self.axx.plot(x, y)
            self.axx.set_xlabel(self.ax.get_xlabel())
            self.update_line_plot_limits()
        self.canvas.draw_idle()

    def plot_y_line(self, x, y):
        try:
            self.yfig[0].set_data(y, x)
        except (AttributeError, IndexError):
            self.axy.clear()
            self.yfig = self.axy.plot(y, x)
            self.axy.set_ylabel(self.ax.get_ylabel())
            self.update_line_plot_limits()
        self.canvas.draw_idle()

    def update_line_plots(self, x, y):
        xmin, xmax, ymin, ymax = self.im.get_extent()
        arr = self.im.get_array()
        data_extent = Bbox([[ymin, xmin], [ymax, xmax]])
        array_extent = Bbox([[0, 0], arr.shape[:2]])
        trans = BboxTransform(boxin=data_extent, boxout=array_extent)
        point = trans.transform_point([y, x])
        if any(np.isnan(point)):
            return
        i, j = point.astype(int)
        if 0 <= i < arr.shape[0]:
            self.plot_x_line(np.linspace(xmin, xmax, arr.shape[1]), arr[i,:])
        if 0 <= j < arr.shape[1]:
            self.plot_y_line(np.linspace(ymin, ymax, arr.shape[0]), arr[:,j])

    def closeEvent(self, event):
        self.deleteLater()
        super(SliceViewerView, self).closeEvent(event)
