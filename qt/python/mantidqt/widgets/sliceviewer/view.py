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
from .peaksviewer.workspaceselection import \
    (PeaksWorkspaceSelectorModel, PeaksWorkspaceSelectorPresenter,
     PeaksWorkspaceSelectorView)
from .peaksviewer.view import PeaksViewerCollectionView
from .peaksviewer.representation.painter import MplPainter

import numpy as np

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QComboBox, QGridLayout, QLabel, QHBoxLayout, QSplitter, QVBoxLayout, QWidget


class SliceViewerDataView(QWidget):
    """The view for the data portion of the sliceviewer"""

    def __init__(self, presenter, dims_info, can_normalise, parent=None):
        super().__init__(parent)

        self.presenter = presenter

        self.line_plots = False
        self.can_normalise = can_normalise

        # Dimension widget
        self.dimensions_layout = QHBoxLayout()
        self.dimensions = DimensionWidget(dims_info, parent=self)
        self.dimensions.dimensionsChanged.connect(self.presenter.dimensions_changed)
        self.dimensions.valueChanged.connect(self.presenter.slicepoint_changed)
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
        self.ax = None
        self.fig.set_facecolor(self.palette().window().color().getRgbF())
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

    def create_axes(self):
        self.fig.clf()
        self.ax = self.fig.add_subplot(111, projection='mantid')
        if self.line_plots:
            self.add_line_plots()

        self.canvas.draw_idle()

    def add_line_plots(self):
        """Assuming line plots are currently disabled, enable them on the current figure
        The image axes must have been created first.
        """
        if self.line_plots:
            return
        image_axes = self.ax
        if image_axes is None:
            return

        # Create a new GridSpec and reposition the existing image Axes
        gs = gridspec.GridSpec(
            2, 2, width_ratios=[1, 4], height_ratios=[4, 1], wspace=0.0, hspace=0.0)
        image_axes.set_position(gs[1].get_position(self.fig))
        image_axes.xaxis.set_visible(False)
        image_axes.yaxis.set_visible(False)
        self.axx = self.fig.add_subplot(gs[3], sharex=image_axes)
        self.axx.yaxis.tick_right()
        self.axy = self.fig.add_subplot(gs[0], sharey=image_axes)
        self.axy.xaxis.tick_top()

        self.mpl_toolbar.update()  # sync list of axes in navstack
        self.canvas.draw_idle()

    def remove_line_plots(self):
        """Assuming line plots are currently enabled, remove them from the current figure
        """
        if not self.line_plots:
            return
        image_axes = self.ax
        if image_axes is None:
            return

        self.clear_line_plots()
        all_axes = self.fig.axes
        # The order is defined by the order of the add_subplot calls so we always want to remove
        # the last two Axes. Do it backwards to cope with the container size change
        all_axes[2].remove()
        all_axes[1].remove()

        gs = gridspec.GridSpec(1, 1)
        image_axes.set_position(gs[0].get_position(self.fig))
        image_axes.xaxis.set_visible(True)
        image_axes.yaxis.set_visible(True)
        self.axx, self.axy = None, None

        self.mpl_toolbar.update()  # sync list of axes in navstack
        self.canvas.draw_idle()

    def plot_MDH(self, ws, **kwargs):
        """
        clears the plot and creates a new one using a MDHistoWorkspace
        """
        self.ax.clear()
        self.im = self.ax.imshow(
            ws,
            origin='lower',
            aspect='auto',
            transpose=self.dimensions.transpose,
            norm=self.colorbar.get_norm(),
            **kwargs)
        self.draw_plot()

    def plot_matrix(self, ws, **kwargs):
        """
        clears the plot and creates a new one using a MatrixWorkspace
        """
        self.ax.clear()
        self.im = imshow_sampling(
            self.ax,
            ws,
            origin='lower',
            aspect='auto',
            interpolation='none',
            transpose=self.dimensions.transpose,
            norm=self.colorbar.get_norm(),
            **kwargs)
        self.im._resample_image()
        self.draw_plot()

    def draw_plot(self):
        self.ax.set_title('')
        self.colorbar.set_mappable(self.im)
        self.colorbar.update_clim()
        self.mpl_toolbar.update()  # clear nav stack
        self.clear_line_plots()
        self.canvas.draw_idle()

    def update_plot_data(self, data):
        """
        This just updates the plot data without creating a new plot
        """
        self.im.set_data(data.T)
        self.colorbar.update_clim()

    def line_plots_toggle(self, state):
        self.presenter.line_plots(state)
        self.line_plots = state

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
        if self.line_plots and event.inaxes == self.ax:
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
            self.plot_x_line(np.linspace(xmin, xmax, arr.shape[1]), arr[i, :])
        if 0 <= j < arr.shape[1]:
            self.plot_y_line(np.linspace(ymin, ymax, arr.shape[0]), arr[:, j])

    def set_normalization(self, ws, **kwargs):
        normalize_by_bin_width, _ = get_normalize_by_bin_width(ws, self.ax, **kwargs)
        is_normalized = normalize_by_bin_width or ws.isDistribution()
        if is_normalized:
            self.presenter.normalization = mantid.api.MDNormalization.VolumeNormalization
            self.norm_opts.setCurrentIndex(1)
        else:
            self.presenter.normalization = mantid.api.MDNormalization.NoNormalization
            self.norm_opts.setCurrentIndex(0)


class SliceViewerView(QWidget):
    """Combines the data view for the slice viewer with the optional peaks viewer."""

    def __init__(self, presenter, dims_info, can_normalise, parent=None):
        super().__init__(parent)

        self.presenter = presenter

        self.setWindowTitle("SliceViewer")
        self.setWindowFlags(Qt.Window)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        self._splitter = QSplitter(self)
        self._data_view = SliceViewerDataView(presenter, dims_info, can_normalise, self)
        self._splitter.addWidget(self._data_view)
        #  peaks viewer off by default
        self._peaks_view = None

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self._splitter)
        self.setLayout(layout)

        # connect up additional peaks signals
        self.data_view.mpl_toolbar.peaksOverlayClicked.connect(self.peaks_overlay_clicked)

    @property
    def data_view(self):
        return self._data_view

    @property
    def dimensions(self):
        return self._data_view.dimensions

    @property
    def peaks_view(self):
        """Lazily instantiates PeaksViewer and returns it"""
        if self._peaks_view is None:
            self._peaks_view = PeaksViewerCollectionView(
                MplPainter(self.data_view.ax), self.presenter)
            self._splitter.addWidget(self._peaks_view)

        return self._peaks_view

    def peaks_overlay_clicked(self):
        """Peaks overlay button has been toggled
        """
        self.presenter.overlay_peaks_workspaces()

    def draw_peak(self, peak_info):
        """
        :param peak_info: A PeakRepresentation object
        """
        return peak_info.draw(self.ax)

    def query_peaks_to_overlay(self, current_overlayed_names):
        """Display a dialog to the user to ask which peaks to overlay
        :param current_overlayed_names: A list of names that are currently overlayed
        :returns: A list of workspace names to overlay on the display
        """
        model = PeaksWorkspaceSelectorModel(
            mantid.api.AnalysisDataService.Instance(), checked_names=current_overlayed_names)
        view = PeaksWorkspaceSelectorView(self)
        presenter = PeaksWorkspaceSelectorPresenter(view, model)
        return presenter.select_peaks_workspaces()

    def set_peaks_viewer_visible(self, on):
        """
        Set the visiblity of the PeaksViewer.
        :param on: If True make the view visible, else make it invisible
        :return: The PeaksViewerCollectionView
        """
        self.peaks_view.set_visible(on)

    # event handlers
    def closeEvent(self, event):
        self.deleteLater()
        super().closeEvent(event)
