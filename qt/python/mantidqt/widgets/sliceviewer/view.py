# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
# 3rd party imports

import mantid.api
from mantid.plots.axesfunctions import _pcolormesh_nonortho as pcolormesh_nonorthogonal
from mantid.plots.datafunctions import get_normalize_by_bin_width
from matplotlib import gridspec
from matplotlib.figure import Figure
from matplotlib.transforms import Bbox, BboxTransform
from mpl_toolkits.axisartist import Subplot as CurveLinearSubPlot
from mpl_toolkits.axisartist.grid_helper_curvelinear import GridHelperCurveLinear
import numpy as np
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QComboBox, QGridLayout, QLabel, QHBoxLayout, QSplitter, QVBoxLayout, QWidget

# local imports
from mantidqt.MPLwidgets import FigureCanvas
from mantidqt.widgets.colorbar.colorbar import ColorbarWidget
from .dimensionwidget import DimensionWidget
from .samplingimage import imshow_sampling
from .toolbar import SliceViewerNavigationToolbar, ToolItemText
from .peaksviewer.workspaceselection import \
    (PeaksWorkspaceSelectorModel, PeaksWorkspaceSelectorPresenter,
     PeaksWorkspaceSelectorView)
from .peaksviewer.view import PeaksViewerCollectionView
from .peaksviewer.representation.painter import MplPainter


class SliceViewerDataView(QWidget):
    """The view for the data portion of the sliceviewer"""
    def __init__(self, presenter, dims_info, can_normalise, parent=None):
        super().__init__(parent)

        self.presenter = presenter

        self.image = None
        self.line_plots = False
        self.can_normalise = can_normalise
        self.nonortho_tr = None

        # Dimension widget
        self.dimensions_layout = QHBoxLayout()
        self.dimensions = DimensionWidget(dims_info, parent=self)
        self.dimensions.dimensionsChanged.connect(self.presenter.dimensions_changed)
        self.dimensions.valueChanged.connect(self.presenter.slicepoint_changed)
        self.dimensions_layout.addWidget(self.dimensions)

        self.colorbar_layout = QVBoxLayout()
        self.colorbar_layout.setContentsMargins(0,0,0,0)
        self.colorbar_layout.setSpacing(0)

        # normalization options
        if can_normalise:
            self.norm_layout = QVBoxLayout()
            self.norm_layout.setContentsMargins(0, 0, 0, 0)
            self.norm_layout.setSpacing(0)
            self.norm_label = QLabel("Normalization")
            self.norm_layout.addWidget(self.norm_label)
            self.norm_opts = QComboBox()
            self.norm_opts.addItems(["None", "By bin width"])
            self.norm_opts.setToolTip("Normalization options")
            self.norm_layout.addWidget(self.norm_opts)
            self.colorbar_layout.addLayout(self.norm_layout)

        # MPL figure + colorbar
        self.mpl_layout = QHBoxLayout()
        self.mpl_layout.setContentsMargins(0,0,0,0)
        self.mpl_layout.setSpacing(0)
        self.fig = Figure()
        self.fig.set_tight_layout(True)
        self.ax = None
        self._grid_on = False
        self.fig.set_facecolor(self.palette().window().color().getRgbF())
        self.canvas = FigureCanvas(self.fig)
        self.canvas.mpl_connect('motion_notify_event', self.mouse_move)
        self.create_axes_orthogonal()
        self.mpl_layout.addWidget(self.canvas)
        self.colorbar_label = QLabel("Colormap")
        self.colorbar_layout.addWidget(self.colorbar_label)
        self.colorbar = ColorbarWidget(self)
        self.colorbar_layout.addWidget(self.colorbar)
        self.colorbar.colorbarChanged.connect(self.update_data_clim)
        self.colorbar.colorbarChanged.connect(self.update_line_plot_limits)
        self.mpl_layout.addLayout(self.colorbar_layout)

        # MPL toolbar
        self.mpl_toolbar = SliceViewerNavigationToolbar(self.canvas, self)
        self.mpl_toolbar.gridClicked.connect(self.toggle_grid)
        self.mpl_toolbar.linePlotsClicked.connect(self.on_line_plots_toggle)
        self.mpl_toolbar.plotOptionsChanged.connect(self.colorbar.mappable_changed)
        self.mpl_toolbar.nonOrthogonalClicked.connect(self.on_non_orthogonal_axes_toggle)

        # layout
        self.layout = QGridLayout(self)
        self.layout.setSpacing(1)
        self.layout.addLayout(self.dimensions_layout, 0, 0)
        self.layout.addWidget(self.mpl_toolbar, 1, 0)
        self.layout.addLayout(self.mpl_layout, 2, 0)

    @property
    def grid_on(self):
        return self._grid_on

    @property
    def nonorthogonal_mode(self):
        return self.nonortho_tr is not None

    def create_axes_orthogonal(self):
        self.clear_figure()
        self.nonortho_tr = None
        self.ax = self.fig.add_subplot(111, projection='mantid')
        if self.grid_on:
            self.ax.grid(self.grid_on)
        if self.line_plots:
            self.add_line_plots()
        self.plot_MDH = self.plot_MDH_orthogonal

        self.canvas.draw_idle()

    def create_axes_nonorthogonal(self, transform):
        self.clear_figure()
        self.set_nonorthogonal_transform(transform)
        self.ax = CurveLinearSubPlot(self.fig,
                                     1,
                                     1,
                                     1,
                                     grid_helper=GridHelperCurveLinear(
                                         (self.nonortho_tr, transform.inv_tr)))
        self.set_grid_on()
        self.fig.add_subplot(self.ax)
        self.plot_MDH = self.plot_MDH_nonorthogonal

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
        gs = gridspec.GridSpec(2,
                               2,
                               width_ratios=[1, 4],
                               height_ratios=[4, 1],
                               wspace=0.0,
                               hspace=0.0)
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

    def plot_MDH_orthogonal(self, ws, **kwargs):
        """
        clears the plot and creates a new one using a MDHistoWorkspace
        """
        self.clear_image()
        self.image = self.ax.imshow(ws,
                                    origin='lower',
                                    aspect='auto',
                                    transpose=self.dimensions.transpose,
                                    norm=self.colorbar.get_norm(),
                                    **kwargs)
        self.draw_plot()

    def plot_MDH_nonorthogonal(self, ws, **kwargs):
        self.clear_image()
        self.image = pcolormesh_nonorthogonal(self.ax,
                                              ws,
                                              self.nonortho_tr,
                                              transpose=self.dimensions.transpose,
                                              norm=self.colorbar.get_norm(),
                                              **kwargs)
        # pcolormesh clears any grid that was previously visible
        if self.grid_on:
            self.ax.grid(self.grid_on)
        self.draw_plot()

    def plot_matrix(self, ws, **kwargs):
        """
        clears the plot and creates a new one using a MatrixWorkspace
        """
        self.clear_image()
        self.image = imshow_sampling(self.ax,
                                     ws,
                                     origin='lower',
                                     aspect='auto',
                                     interpolation='none',
                                     transpose=self.dimensions.transpose,
                                     norm=self.colorbar.get_norm(),
                                     **kwargs)
        self.image._resample_image()
        self.draw_plot()

    def clear_image(self):
        """Removes any image from the axes"""
        if self.image is not None:
            self.image.remove()
            self.image = None

    def clear_figure(self):
        """Removes everything from the figure"""
        self.image = None
        self.fig.clf()

    def draw_plot(self):
        self.ax.set_title('')
        self.colorbar.set_mappable(self.image)
        self.colorbar.update_clim()
        self.mpl_toolbar.update()  # clear nav stack
        self.clear_line_plots()
        self.canvas.draw_idle()

    def update_plot_data(self, data):
        """
        This just updates the plot data without creating a new plot
        """
        if self.nonortho_tr:
            self.image.set_array(data.T.ravel())
        else:
            self.image.set_data(data.T)
        self.colorbar.update_clim()

    def on_line_plots_toggle(self, state):
        self.presenter.line_plots(state)
        self.line_plots = state

    def on_non_orthogonal_axes_toggle(self, state):
        """
        Switch state of the non-orthognal axes on/off
        """
        self.presenter.nonorthogonal_axes(state)

    def enable_lineplots_button(self):
        """
        Enables line plots functionality
        """
        self.mpl_toolbar.set_action_enabled(ToolItemText.LINEPLOTS, True)

    def disable_lineplots_button(self):
        """
        Disabled line plots functionality
        """
        self.mpl_toolbar.set_action_enabled(ToolItemText.LINEPLOTS, False)

    def enable_peaks_button(self):
        """
        Enables line plots functionality
        """
        self.mpl_toolbar.set_action_enabled(ToolItemText.OVERLAYPEAKS, True)

    def disable_peaks_button(self):
        """
        Disables line plots functionality
        """
        self.mpl_toolbar.set_action_enabled(ToolItemText.OVERLAYPEAKS, False)

    def enable_nonorthogonal_axes_button(self):
        """
        Enables access to non-orthogonal axes functionality
        """
        self.mpl_toolbar.set_action_enabled(ToolItemText.NONORTHOGONAL_AXES, True)

    def disable_nonorthogonal_axes_button(self):
        """
        Disables non-orthorognal axes functionality
        """
        self.mpl_toolbar.set_action_enabled(ToolItemText.NONORTHOGONAL_AXES, state=False)

    def clear_line_plots(self):
        try:  # clear old plots
            del self.xfig
            del self.yfig
        except AttributeError:
            pass

    def update_data_clim(self):
        self.image.set_clim(self.colorbar.colorbar.mappable.get_clim())
        self.canvas.draw_idle()

    def update_line_plot_limits(self):
        if self.line_plots:
            self.axx.set_ylim(self.colorbar.cmin_value, self.colorbar.cmax_value)
            self.axy.set_xlim(self.colorbar.cmin_value, self.colorbar.cmax_value)

    def set_grid_on(self):
        """
        If not visible sets the grid visibility
        """
        if not self._grid_on:
            self._grid_on = True
            self.mpl_toolbar.set_action_checked(ToolItemText.GRID, state=self._grid_on)

    def set_nonorthogonal_transform(self, transform):
        """
        Set the transform for nonorthogonal axes mode
        :param transform: An object with a tr method to transform from nonorthognal
                          coordinates to display coordinates
        """
        self.nonortho_tr = transform.tr

    def toggle_grid(self,state):
        """
        Toggle the visibility of the grid on the axes
        """
        self._grid_on = state
        self.ax.grid(self._grid_on)
        self.canvas.draw_idle()

    def mouse_move(self, event):
        if self.line_plots and event.inaxes == self.ax:
            self.update_line_plots(event.xdata, event.ydata)

    def plot_x_line(self, x, y):
        try:
            self.xfig[0].set_data(x, y)
        except (AttributeError, IndexError):
            self.axx.clear()
            self.xfig = self.axx.plot(x, y, scalex=False)
            self.axx.set_xlabel(self.ax.get_xlabel())
            self.update_line_plot_limits()
        self.canvas.draw_idle()

    def plot_y_line(self, x, y):
        try:
            self.yfig[0].set_data(y, x)
        except (AttributeError, IndexError):
            self.axy.clear()
            self.yfig = self.axy.plot(y, x, scaley=False)
            self.axy.set_ylabel(self.ax.get_ylabel())
            self.update_line_plot_limits()
        self.canvas.draw_idle()

    def update_line_plots(self, x, y):
        xmin, xmax, ymin, ymax = self.image.get_extent()
        arr = self.image.get_array()
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
            self._peaks_view = PeaksViewerCollectionView(MplPainter(self.data_view.ax),
                                                         self.presenter)
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
        model = PeaksWorkspaceSelectorModel(mantid.api.AnalysisDataService.Instance(),
                                            checked_names=current_overlayed_names)
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
