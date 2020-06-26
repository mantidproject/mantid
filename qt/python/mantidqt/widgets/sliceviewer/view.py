# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
# std imports
import sys

# 3rd party imports

import mantid.api
from mantid.plots.axesfunctions import _pcolormesh_nonortho as pcolormesh_nonorthogonal
from mantid.plots.datafunctions import get_normalize_by_bin_width
from matplotlib.figure import Figure
from mpl_toolkits.axisartist import Subplot as CurveLinearSubPlot
from mpl_toolkits.axisartist.grid_helper_curvelinear import GridHelperCurveLinear
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QCheckBox, QComboBox, QGridLayout, QLabel, QHBoxLayout, QSplitter, QVBoxLayout, QWidget

# local imports
from mantidqt.MPLwidgets import FigureCanvas
from mantidqt.widgets.colorbar.colorbar import ColorbarWidget
from .dimensionwidget import DimensionWidget
from .imageinfowidget import ImageInfoWidget, ImageInfoTracker
from .lineplots import LinePlotter, PixelLinePlot
from .samplingimage import imshow_sampling
from .toolbar import SliceViewerNavigationToolbar, ToolItemText
from .peaksviewer.workspaceselection import \
    (PeaksWorkspaceSelectorModel, PeaksWorkspaceSelectorPresenter,
     PeaksWorkspaceSelectorView)
from .peaksviewer.view import PeaksViewerCollectionView
from .peaksviewer.representation.painter import MplPainter
from .zoom import ScrollZoomMixin

# Constants
DBLMAX = sys.float_info.max


class SliceViewerCanvas(ScrollZoomMixin, FigureCanvas):
    pass


class SliceViewerDataView(QWidget):
    """The view for the data portion of the sliceviewer"""
    def __init__(self, presenter, dims_info, can_normalise, parent=None):
        super().__init__(parent)

        self.presenter = presenter

        self.image = None
        self.line_plots = False
        self.can_normalise = can_normalise
        self.nonortho_tr = None
        self.ws_type = dims_info[0]['type']

        self._line_plotter = None
        self._image_info_tracker = None

        # Dimension widget
        self.dimensions_layout = QGridLayout()
        self.dimensions = DimensionWidget(dims_info, parent=self)
        self.dimensions.dimensionsChanged.connect(self.presenter.dimensions_changed)
        self.dimensions.valueChanged.connect(self.presenter.slicepoint_changed)
        self.dimensions_layout.addWidget(self.dimensions, 1, 0, 1, 1)

        self.colorbar_layout = QVBoxLayout()
        self.colorbar_layout.setContentsMargins(0, 0, 0, 0)
        self.colorbar_layout.setSpacing(0)

        self.image_info_widget = ImageInfoWidget(self.ws_type, self)
        self.track_cursor = QCheckBox("Track Cursor", self)
        self.track_cursor.setToolTip(
            "Update the image readout table when the cursor is over the plot. "
            "If unticked the table will update only when the plot is clicked")
        if self.ws_type == 'MDE':
            self.colorbar_layout.addWidget(self.image_info_widget, alignment=Qt.AlignCenter)
            self.colorbar_layout.addWidget(self.track_cursor)
        else:
            self.dimensions_layout.setHorizontalSpacing(10)
            self.dimensions_layout.addWidget(self.track_cursor, 0, 1, Qt.AlignRight)
            self.dimensions_layout.addWidget(self.image_info_widget, 1, 1)
        self.track_cursor.setChecked(True)
        self.track_cursor.stateChanged.connect(self.on_track_cursor_state_change)

        # normalization options
        if can_normalise:
            self.norm_label = QLabel("Normalization")
            self.colorbar_layout.addWidget(self.norm_label)
            self.norm_opts = QComboBox()
            self.norm_opts.addItems(["None", "By bin width"])
            self.norm_opts.setToolTip("Normalization options")
            self.colorbar_layout.addWidget(self.norm_opts)

        # MPL figure + colorbar
        self.fig = Figure()
        self.ax = None
        self.image = None
        self._grid_on = False
        self.fig.set_facecolor(self.palette().window().color().getRgbF())
        self.canvas = SliceViewerCanvas(self.fig)
        self.canvas.mpl_connect('button_release_event', self.mouse_release)

        self.colorbar_label = QLabel("Colormap")
        self.colorbar_layout.addWidget(self.colorbar_label)
        self.colorbar = ColorbarWidget(self)
        self.colorbar_layout.addWidget(self.colorbar)
        self.colorbar.colorbarChanged.connect(self.update_data_clim)
        # make width larger to fit image readout table
        if self.ws_type == 'MDE':
            self.colorbar.setMaximumWidth(155)

        # MPL toolbar
        self.toolbar_layout = QHBoxLayout()
        self.mpl_toolbar = SliceViewerNavigationToolbar(self.canvas, self, False)
        self.mpl_toolbar.gridClicked.connect(self.toggle_grid)
        self.mpl_toolbar.linePlotsClicked.connect(self.on_line_plots_toggle)
        self.mpl_toolbar.homeClicked.connect(self.on_home_clicked)
        self.mpl_toolbar.plotOptionsChanged.connect(self.colorbar.mappable_changed)
        self.mpl_toolbar.nonOrthogonalClicked.connect(self.on_non_orthogonal_axes_toggle)
        self.mpl_toolbar.zoomPanFinished.connect(self.on_data_limits_changed)
        self.toolbar_layout.addWidget(self.mpl_toolbar)

        # layout
        layout = QGridLayout(self)
        layout.setSpacing(1)
        layout.addLayout(self.dimensions_layout, 0, 0, 1, 2)
        layout.addLayout(self.toolbar_layout, 1, 0, 1, 2)
        layout.addWidget(self.canvas, 2, 0, 1, 1)
        layout.addLayout(self.colorbar_layout, 1, 1, 2, 1)
        layout.setRowStretch(2, 1)

    @property
    def grid_on(self):
        return self._grid_on

    @property
    def nonorthogonal_mode(self):
        return self.nonortho_tr is not None

    def create_axes_orthogonal(self, redraw_on_zoom=False):
        """Create a standard set of orthogonal axes
        :param redraw_on_zoom: If True then when scroll zooming the canvas is redrawn immediately
        """
        self.clear_figure()
        self.nonortho_tr = None
        self.ax = self.fig.add_subplot(111, projection='mantid')
        self.enable_zoom_on_mouse_scroll(redraw_on_zoom)
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
        # don't redraw on zoom as the data is rebinned and has to be redrawn again anyway
        self.enable_zoom_on_mouse_scroll(redraw=False)
        self.set_grid_on()
        self.fig.add_subplot(self.ax)
        self.plot_MDH = self.plot_MDH_nonorthogonal

        self.canvas.draw_idle()

    def enable_zoom_on_mouse_scroll(self, redraw):
        """Enable zoom on scroll the mouse wheel for the created axes
        :param redraw: Pass through to redraw option in enable_zoom_on_scroll
        """
        self.canvas.enable_zoom_on_scroll(self.ax,
                                          redraw=redraw,
                                          toolbar=self.mpl_toolbar,
                                          callback=self.on_data_limits_changed)

    def add_line_plots(self):
        """Assuming line plots are currently disabled, enable them on the current figure
        The image axes must have been created first.
        """
        if self.line_plots:
            return
        self.line_plots = True
        self._line_plotter = LinePlotter(self.ax, self.colorbar, PixelLinePlot)

    def remove_line_plots(self):
        """Assuming line plots are currently enabled, remove them from the current figure
        """
        if not self.line_plots:
            return

        #self._cursor_tracker.unsubscribe(self._line_plotter)
        self._line_plotter.close()
        self._line_plotter = None
        self.line_plots = False

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
        self.on_track_cursor_state_change(self.track_cursor.isChecked())

        # ensure the axes data limits are updated to match the
        # image. For example if the axes were zoomed and the
        # swap dimensions was clicked we need to restore the
        # appropriate extents to see the image in the correct place
        extent = self.image.get_extent()
        self.ax.set_xlim(extent[0], extent[1])
        self.ax.set_ylim(extent[2], extent[3])

        self.draw_plot()

    def plot_MDH_nonorthogonal(self, ws, **kwargs):
        self.clear_image()
        self.image = pcolormesh_nonorthogonal(self.ax,
                                              ws,
                                              self.nonortho_tr,
                                              transpose=self.dimensions.transpose,
                                              norm=self.colorbar.get_norm(),
                                              **kwargs)
        self.on_track_cursor_state_change(self.track_cursor.isChecked())

        # swapping dimensions in nonorthogonal mode currently resets back to the
        # full data limits as the whole axes has been recreated so we don't have
        # access to the original limits
        # pcolormesh clears any grid that was previously visible
        if self.grid_on:
            self.ax.grid(self.grid_on)
        self.draw_plot()

    def plot_matrix(self, ws, **kwargs):
        """
        clears the plot and creates a new one using a MatrixWorkspace keeping
        the axes limits that have already been set
        """
        # ensure view is correct if zoomed in while swapping dimensions
        # compute required extent and just have resampling imshow deal with it
        old_extent = None
        if self.image is not None:
            old_extent = self.image.get_extent()
            if self.image.transpose != self.dimensions.transpose:
                e1, e2, e3, e4 = old_extent
                old_extent = e3, e4, e1, e2

        self.clear_image()
        self.image = imshow_sampling(self.ax,
                                     ws,
                                     origin='lower',
                                     aspect='auto',
                                     interpolation='none',
                                     transpose=self.dimensions.transpose,
                                     norm=self.colorbar.get_norm(),
                                     extent=old_extent,
                                     **kwargs)
        self.on_track_cursor_state_change(self.track_cursor.isChecked())

        self.draw_plot()

    def clear_image(self):
        """Removes any image from the axes"""
        if self.image is not None:
            if self.line_plots:
                self._line_plotter.delete_line_plot_lines()
            self.image_info_widget.updateTable(DBLMAX, DBLMAX, DBLMAX)
            self.image.remove()
            self.image = None

    def clear_figure(self):
        """Removes everything from the figure"""
        if self.line_plots:
            self._line_plotter.close()
        self.image = None
        self.canvas.disable_zoom_on_scroll()
        self.fig.clf()
        self.ax = None

    def draw_plot(self):
        self.ax.set_title('')
        self.colorbar.set_mappable(self.image)
        self.colorbar.update_clim()
        self.mpl_toolbar.update()  # clear nav stack
        if self.line_plots:
            self._line_plotter.delete_line_plot_lines()
            self._line_plotter.update_line_plot_labels()
        self.canvas.draw_idle()

    def select_zoom(self):
        """Select the zoom control on the toolbar"""
        self.mpl_toolbar.zoom()

    def update_plot_data(self, data):
        """
        This just updates the plot data without creating a new plot. The extents
        can change if the data has been rebinned.
        """
        if self.nonortho_tr:
            self.image.set_array(data.T.ravel())
        else:
            self.image.set_data(data.T)
        self.colorbar.update_clim()

    def on_track_cursor_state_change(self, state):
        """
        Called to notify the current state of the track cursor box
        """
        if self._image_info_tracker is not None:
            self._image_info_tracker.disconnect()

        self._image_info_tracker = ImageInfoTracker(image=self.image,
                                                    transpose_xy=self.dimensions.transpose,
                                                    widget=self.image_info_widget)

        if state:
            self._image_info_tracker.connect()
        else:
            self._image_info_tracker.disconnect()

    def on_home_clicked(self):
        """Reset the view to encompass all of the data"""
        self.presenter.show_all_data_requested()

    def on_line_plots_toggle(self, state):
        """Switch state of the line plots"""
        self.presenter.line_plots(state)

    def on_region_selection_toggled(self, state):
        """Switch state of the region selection"""
        self.presenter.region_selection(state)

    def on_non_orthogonal_axes_toggle(self, state):
        """
        Switch state of the non-orthognal axes on/off
        """
        self.presenter.nonorthogonal_axes(state)

    def on_data_limits_changed(self):
        """
        React to when the data limits have changed
        """
        self.presenter.data_limits_changed()

    def enable_lineplots_button(self):
        """
        Enable line plot button
        """
        self.mpl_toolbar.set_action_enabled(ToolItemText.LINEPLOTS, True)

    def disable_lineplots_button(self):
        """
        Disable line plot button
        """
        self.mpl_toolbar.set_action_enabled(ToolItemText.LINEPLOTS, False)

    def deactivate_lineplots(self):
        """
        Turn off the lineplots as if a user had clicked the button
        """
        self.mpl_toolbar.set_action_checked(ToolItemText.LINEPLOTS, False)

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

    def get_axes_limits(self):
        """
        Return the limits of the image axes or None if no image yet exists
        """
        if self.image is None:
            return None
        else:
            return self.ax.get_xlim(), self.ax.get_ylim()

    def set_axes_limits(self, xlim, ylim):
        """
        Set the view limits on the image axes to the given extents
        :param xlim: 2-tuple of (xmin, xmax)
        :param ylim: 2-tuple of (ymin, ymax)
        """
        self.ax.set_xlim(xlim)
        self.ax.set_ylim(ylim)

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

    def toggle_grid(self, state):
        """
        Toggle the visibility of the grid on the axes
        """
        self._grid_on = state
        self.ax.grid(self._grid_on)
        self.canvas.draw_idle()

    def mouse_release(self, event):
        if event.inaxes != self.ax:
            return
        if event.button == 1:
            self._image_info_tracker.on_cursor_at(event.xdata, event.ydata)
        if event.button == 3:
            self.on_home_clicked()

    def update_data_clim(self):
        self.image.set_clim(self.colorbar.colorbar.mappable.get_clim())
        if self.line_plots:
            self._line_plotter.update_line_plot_limits()
        self.canvas.draw_idle()

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
