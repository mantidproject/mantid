# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt
# std imports
from collections import namedtuple
from functools import lru_cache
from typing import Any, Optional, Tuple

# 3rd party imports
from matplotlib.artist import setp as set_artist_property
from matplotlib.axes import Axes
from matplotlib.image import AxesImage
from matplotlib.gridspec import GridSpec
from matplotlib.transforms import Bbox, BboxTransform
from matplotlib.widgets import RectangleSelector
import numpy as np

# local imports
from mantidqt.widgets.colorbar.colorbar import ColorbarWidget
from .cursor import (CursorTracker, MoveMouseCursorUp, MoveMouseCursorDown, MoveMouseCursorLeft,
                     MoveMouseCursorRight)

# Limits for X/Y axes
Limits = Tuple[Tuple[float, float], Tuple[float, float]]


class LinePlots:
    """
    Provides facilities to add line cut plots to an existing
    image axes
    """
    __slots__ = ("_axx", "_axy", "_canvas", "_colorbar", "_fig", "_im_ax", "_xfig", "_yfig")

    def __init__(self, image_axes: Axes, colorbar: ColorbarWidget):
        """
        :param image_axes: A reference to the image axes containing the
                           source Image data for the cuts. It is assumed
                           the Axes contains a single AxesImage object in the
                           .images attribute
        :param colorbar: An instance of a mantidqt.widgets.colorbar.colorbar.ColorbarWidget
        :param plottype_cls: A class type defining how the plot should be created. It's __init__
        method must accept a LinePlotter instance as a parameter.
        """
        self._im_ax = image_axes
        self._fig = image_axes.figure
        self._canvas = self._fig.canvas
        self._colorbar = colorbar

        self._add_line_plots()

    @property
    def image(self):
        return self._im_ax.images[0]

    @property
    def image_axes(self):
        return self._im_ax

    def close(self):
        """
        Call to remove the line plots from the axes
        """
        self._remove_line_plots()

    def delete_line_plot_lines(self):
        """
        Remove plotted cut curves
        """
        try:
            try:
                self._xfig.remove()
                self._yfig.remove()
            except ValueError:
                pass
            del self._xfig
            del self._yfig
            self._canvas.draw_idle()
        except AttributeError:
            pass

    def plot_x_line(self, x: np.ndarray, y: np.ndarray):
        """
        Plots cut parallel to the X axis of the image
        :param x: Array of values for X axis
        :param y: Array of values for Y axis
        """
        try:
            self._xfig.set_data(x, y)
        except (AttributeError, IndexError):
            self._axx.clear()
            self._xfig = self._axx.plot(x, y, scalex=False)[0]
            self._xfig.set_linewidth(0.5)
            self.update_line_plot_labels()

    def plot_y_line(self, x: np.array, y: np.array):
        """
        Plots cut parallel to the Y axis of the image.
        :param x: Array of values for X axis
        :param y: Array of values for Y axis
        """
        try:
            self._yfig.set_data(y, x)
        except (AttributeError, IndexError):
            self._axy.clear()
            self._yfig = self._axy.plot(y, x, scaley=False)[0]
            self._yfig.set_linewidth(0.5)
            self.update_line_plot_labels()

    def redraw(self):
        """Redraw the canvas"""
        self._canvas.draw_idle()

    def sync_plot_limits_with_colorbar(self):
        """
        Update limits of line plots to match colorbar scale
        """
        # set line plot intensity axes to match colorbar limits
        self._axx.set_ylim(self._colorbar.cmin_value, self._colorbar.cmax_value)
        self._axy.set_xlim(self._colorbar.cmin_value, self._colorbar.cmax_value)

    def update_line_plot_limits(self):
        """
        Update line plot limits based on the data in them
        """
        # ensure plot labels are in sync with main axes
        self._axx.relim()
        self._axx.autoscale(axis='y')
        self._axy.relim()
        self._axy.autoscale(axis='x')

    def update_line_plot_labels(self):
        """
        Update line plot labels to match main image labels
        """
        # ensure plot labels are in sync with main axes
        self._axx.set_xlabel(self._im_ax.get_xlabel())
        self._axy.set_ylabel(self._im_ax.get_ylabel())

    def update_line_plot_scales(self):
        """
        Set line plot scale axes to match colorbar scale
        """
        scale, kwargs = self._colorbar.get_colorbar_scale()
        try:
            self._axx.set_yscale(scale, **kwargs)
            self._axy.set_xscale(scale, **kwargs)
        except AttributeError:
            pass

    # Private api
    def _add_line_plots(self):
        """
        Create new axes for the X/Y cuts and attach them to the
        existing image axes.
        """
        image_axes = self._im_ax

        # Create a new GridSpec and reposition the existing image Axes
        gs = GridSpec(2, 2, width_ratios=[1, 4], height_ratios=[4, 1], wspace=0.0, hspace=0.0)
        image_axes.set_position(gs[1].get_position(self._fig))
        set_artist_property(image_axes.get_xticklabels(), visible=False)
        set_artist_property(image_axes.get_yticklabels(), visible=False)
        self._axx = self._fig.add_subplot(gs[3], sharex=image_axes)
        self._axx.yaxis.tick_right()
        self._axy = self._fig.add_subplot(gs[0], sharey=image_axes)
        self._axy.xaxis.tick_top()

        self.update_line_plot_labels()
        self.update_line_plot_scales()
        self._canvas.draw_idle()

    def _remove_line_plots(self):
        """
        Remove the attached cut plots and restore the image axes to the central
        axes position
        """
        image_axes = self._im_ax

        self.delete_line_plot_lines()
        all_axes = self._fig.axes
        # The order is defined by the order of the add_subplot calls so we always want to remove
        # the last two Axes. Do it backwards to cope with the container size change
        all_axes[2].remove()
        all_axes[1].remove()

        gs = GridSpec(1, 1)
        image_axes.set_position(gs[0].get_position(self._fig))
        image_axes.xaxis.tick_bottom()
        image_axes.yaxis.tick_left()
        self._axx, self._axy = None, None

        # self.mpl_toolbar.update()  # sync list of axes in navstack
        self._canvas.draw_idle()


class KeyHandler:
    """Base-class for any lineplot tool that wants to allow export regions
    with keypresses.
    The derived class should implement to class attributes:
      - STATUS_MESSAGE: str - A string defining the information message to display
      - SELECTION_KEYS: tuple(str) - A tuple of strings defining keys to accept
    """
    def __init__(self, plotter: LinePlots, exporter: Any):
        """
        :param plotter: A reference to the object holding the line plot axes.
        :param roi_export_cb: An optional callback when an export of a selected region is requested
        """
        self._plotter = plotter
        self.exporter = exporter
        ax = plotter.image_axes
        self._key_cid = ax.figure.canvas.mpl_connect('key_release_event', self._on_key_released)

    def disconnect(self):
        """Called to disconnect from the axes events"""
        self.plotter.image_axes.figure.canvas.mpl_disconnect(self._key_cid)

    def status_message(self):
        """
        Return a status message to display when this tool is active
        """
        return self.STATUS_MESSAGE

    @property
    def plotter(self):
        return self._plotter

    # private api
    def _on_key_released(self, event):
        """
        Callback on key release event
        :param event: KeyEvent defining release
        """
        key = event.key
        if key not in self.SELECTION_KEYS:
            return

        # defer implementation
        try:
            self.handle_key(key)
        except Exception:
            import traceback
            traceback.print_exc()


class PixelLinePlot(CursorTracker, KeyHandler):
    """
    Draws X/Y line plots from single rows/columns of an image into a given
    set of line plots.
    """

    STATUS_MESSAGE = "Keys: arrow keys control mouse pointer, workspace cuts: c=both cuts, x=X, y=Y."
    SELECTION_KEYS = ('c', 'x', 'y', 'up', 'down', 'left', 'right')
    PIXEL_TRANSFORM_CLS = {
        'up': MoveMouseCursorUp,
        'down': MoveMouseCursorDown,
        'left': MoveMouseCursorLeft,
        'right': MoveMouseCursorRight,
    }

    def __init__(self, plotter: LinePlots, exporter: Any):
        """
        See RegionExtractionTool for parameter descriptions
        """
        CursorTracker.__init__(self, image_axes=plotter.image_axes, autoconnect=False)
        KeyHandler.__init__(self, plotter, exporter)

        # cache most current cursor position
        self._cursor_pos = None

    # CursorTracker interface
    def on_cursor_at(self, xdata: float, ydata: float):
        """
        Notify the object that a new event has occurred at the given coordinates
        :param xdata: X position in data coordinates
        :param ydata: Y position in data coordinates
        """
        plotter = self.plotter
        cinfo = cursor_info(plotter.image, xdata, ydata)
        if cinfo is not None:
            self._cursor_pos = (xdata, ydata)
            arr, (xmin, xmax, ymin, ymax), (i, j) = cinfo
            plotter.plot_x_line(np.linspace(xmin, xmax, arr.shape[1]), arr[i, :])
            plotter.plot_y_line(np.linspace(ymin, ymax, arr.shape[0]), arr[:, j])
            plotter.sync_plot_limits_with_colorbar()
            plotter.redraw()

    def on_cursor_outside_axes(self):
        """
        Called when the mouse moves outside of the image axes
        """
        self._plotter.delete_line_plot_lines()
        self._cursor_pos = None

    # KeyHandler interface
    def handle_key(self, key):
        """
        Called by KeyHandler if a key was accepted to perform a region
        extraction
        """
        if self._cursor_pos is None:
            return None

        pixel_transforms = self.PIXEL_TRANSFORM_CLS
        if key in pixel_transforms:
            to_next_pixel = pixel_transforms[key](self.plotter.image)
            to_next_pixel.move_from(self._cursor_pos)
        else:
            self.exporter.export_pixel_cut(self._cursor_pos, key)


class RectangleSelectionLinePlot(KeyHandler):
    """
    Draws X/Y line plots from a rectangular selection by summing across
    the orthogonal direction.
    """

    STATUS_MESSAGE = "Press key to send roi/cuts to workspaces: r=roi, c=both cuts, x=X, y=Y. Esc clears region"
    SELECTION_KEYS = ('r', 'c', 'x', 'y')

    def __init__(self, plotter: LinePlots, exporter: Any):
        """
        See KeyHandler for parameter descriptions
        """
        super().__init__(plotter, exporter)

        ax = plotter.image_axes
        self._selector = RectangleSelector(
            ax,
            self._on_rectangle_selected,
            drawtype='box',
            useblit=False,  # rectangle persists on button release
            button=[1],
            minspanx=5,
            minspany=5,
            spancoords='pixels',
            interactive=True)

    def disconnect(self):
        super().disconnect()
        for artist in self._selector.artists:
            artist.remove()
        del self._selector

    # RegionExtractionTool interface
    def handle_key(self, key):
        """
        Called if a keypress was accepted to export a region
        :param key: str identifying key
        :return: A string describing the result of the operation
        """
        # if the image has been moved and the selection is not visible then do nothing
        if not self._selector.artists[0].get_visible():
            return

        rect = self._selector.to_draw
        ll_x, ll_y = rect.get_xy()
        limits = ((ll_x, ll_x + rect.get_width()), (ll_y, ll_y + rect.get_height()))

        if key == 'r':
            self.exporter.export_roi(limits)
        if key in ('c', 'x', 'y'):
            self.exporter.export_cut(limits, cut_type=key)

    # private api
    def _on_rectangle_selected(self, eclick, erelease):
        """
        Callback when a rectangle has been draw on the axes
        :param eclick: Event marking where the mouse was clicked
        :param erelease: Event marking where the mouse was released
        """
        plotter = self.plotter
        cinfo_click = cursor_info(plotter.image, eclick.xdata, eclick.ydata)
        if cinfo_click is None:
            return
        cinfo_release = cursor_info(plotter.image, erelease.xdata, erelease.ydata)
        if cinfo_release is None:
            return

        arr, (xmin, xmax, ymin, ymax), (imin, jmin) = cinfo_click
        _, __, (imax, jmax) = cinfo_release
        plotter.plot_x_line(
            np.linspace(xmin, xmax, arr.shape[1])[jmin:jmax],
            np.sum(arr[imin:imax, jmin:jmax], axis=0))
        plotter.plot_y_line(
            np.linspace(ymin, ymax, arr.shape[0])[imin:imax],
            np.sum(arr[imin:imax, jmin:jmax], axis=1))
        plotter.update_line_plot_limits()
        plotter.redraw()


# Data type to store information related to a cursor over an image
CursorInfo = namedtuple("CursorInfo", ("array", "extent", "point"))


@lru_cache(maxsize=32)
def cursor_info(image: AxesImage, xdata: float, ydata: float, full_bbox: Bbox = None) -> Optional[CursorInfo]:
    """Return information on the image for the given position in
    data coordinates.
    :param image: An instance of an image type
    :param xdata: X data coordinate of cursor
    :param xdata: Y data coordinate of cursor
    :param full_bbox: Bbox of full workspace dimension to use for transforming mouse position
    :return: None if point is not valid on the image else return CursorInfo type
    """
    extent = image.get_extent()
    xmin, xmax, ymin, ymax = extent
    arr = image.get_array()
    data_extent = Bbox([[ymin, xmin], [ymax, xmax]])
    array_extent = Bbox([[0, 0], arr.shape[:2]])
    if full_bbox is None:
        trans = BboxTransform(boxin=data_extent, boxout=array_extent)
    else:
        # If the view is zoomed in and the slice is changed, then the image extents
        # and data extents change. This causes the cursor to be transformed to the
        # wrong point for certain MDH workspaces (since it cannot be dynamically rebinned).
        # This will use the full WS data dimensions to do the transformation
        trans = BboxTransform(boxin=full_bbox, boxout=array_extent)
    point = trans.transform_point([ydata, xdata])
    if any(np.isnan(point)):
        return None

    point = point.astype(int)
    if 0 <= point[0] < arr.shape[0] and 0 <= point[1] < arr.shape[1]:
        return CursorInfo(array=arr, extent=extent, point=point)
    else:
        return None
