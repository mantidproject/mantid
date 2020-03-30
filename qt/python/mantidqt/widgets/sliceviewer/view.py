# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from matplotlib import gridspec
from matplotlib.figure import Figure
from matplotlib.transforms import Bbox, BboxTransform

import mantid.api
from mantid.plots.datafunctions import get_normalize_by_bin_width
from mantidqt.MPLwidgets import FigureCanvas
from mantidqt.widgets.colorbar.colorbar import ColorbarWidget
from .dimensionwidget import DimensionWidget
from .samplingimage import imshow_sampling
from .toolbar import SliceViewerNavigationToolbar
from .peaksviewer.view import PeaksViewerCollectionView

import numpy as np

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QComboBox, QGridLayout, QLabel, QHBoxLayout, QVBoxLayout, QWidget


class SliceViewerView(QWidget):
    PEAK_CENTER_MARKER = 'x'
    # size in points^2.
    PEAK_CENTER_MARKER_SIZE_PTS_SQ = 150

    def __init__(self, presenter, dims_info, can_normalise, parent=None):
        super(SliceViewerView, self).__init__(parent)

        self.presenter = presenter

        self.setWindowTitle("SliceViewer")
        self.setWindowFlags(Qt.Window)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        self.line_plots = False
        self.can_normalise = can_normalise

        # Dimension widget
        self.dimensions_layout = QHBoxLayout()
        self.dimensions = DimensionWidget(dims_info, parent=self)
        self.dimensions.dimensionsChanged.connect(self.presenter.new_plot)
        self.dimensions.valueChanged.connect(self.presenter.update_plot_data)
        self.dimensions_layout.addWidget(self.dimensions)

        self.colorbar_layout = QVBoxLayout()

        # normalization options
        if can_normalise:
            self.norm_layout = QHBoxLayout()
            self.norm_label = QLabel("Normalization =")
            self.norm_layout.addWidget(self.norm_label)
            self.norm_opts = QComboBox()
            self.norm_opts.addItems(["None", "By bin width"])
            self.norm_opts.setToolTip("Normalization options")
            self.norm_layout.addWidget(self.norm_opts)
            self.colorbar_layout.addLayout(self.norm_layout)

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
        self.colorbar_layout.addWidget(self.colorbar)
        self.colorbar.colorbarChanged.connect(self.update_data_clim)
        self.colorbar.colorbarChanged.connect(self.update_line_plot_limits)
        self.mpl_layout.addLayout(self.colorbar_layout)

        # MPL toolbar
        self.mpl_toolbar = SliceViewerNavigationToolbar(self.canvas, self)
        self.mpl_toolbar.gridClicked.connect(self.toggle_grid)
        self.mpl_toolbar.linePlotsClicked.connect(self.line_plots_toggle)
        self.mpl_toolbar.plotOptionsChanged.connect(self.colorbar.mappable_changed)

        # layout
        self.layout = QGridLayout(self)
        self.layout.addLayout(self.dimensions_layout, 0, 0)
        self.layout.addWidget(self.mpl_toolbar, 1, 0)
        self.layout.addLayout(self.mpl_layout, 2, 0)

        # optional peaks
        self.peaks_tools = None

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
        try:  # clear old plots
            del self.xfig
            del self.yfig
        except AttributeError:
            pass

    def update_data_clim(self):
        self.im.set_clim(self.colorbar.colorbar.mappable.get_clim())
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

    def set_normalization(self, ws, **kwargs):
        normalize_by_bin_width, _ = get_normalize_by_bin_width(ws, self.ax, **kwargs)
        is_normalized = normalize_by_bin_width or ws.isDistribution()
        if is_normalized:
            self.presenter.normalization = mantid.api.MDNormalization.VolumeNormalization
            self.norm_opts.setCurrentIndex(1)
        else:
            self.presenter.normalization = mantid.api.MDNormalization.NoNormalization
            self.norm_opts.setCurrentIndex(0)

    # peaks tools
    def query_peaks_to_overlay(self):
        """Display a dialog to the user to ask which peaks to overlay"""
        return "peaksws",

    def attach_peaks_tools(self):
        """
        Create (if necessary) the view of PeaksWorkspace tools
        and attach it here
        :return: The PeaksViewerCollectionView
        """
        if self.peaks_tools is not None:
            return self.peaks_tools

        self.peaks_tools = PeaksViewerCollectionView()
        from_row, from_col, row_span, col_span = 0, 1, -1, 1
        self.layout.addWidget(self.peaks_tools, from_row, from_col,
                              row_span, col_span)
        return self.peaks_tools

    def draw_peak(self, x, y, alpha, color):
        """
        :param peak_info: A list of PeakRepresentation objects
        """
        self.ax.scatter(x, y, alpha=alpha, color=color,
                        marker=self.PEAK_CENTER_MARKER,
                        s=self.PEAK_CENTER_MARKER_SIZE_PTS_SQ)

    # event handlers
    def closeEvent(self, event):
        self.deleteLater()
        super(SliceViewerView, self).closeEvent(event)
