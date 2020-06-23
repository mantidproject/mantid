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
from typing import Any, Optional

# 3rd party imports
from matplotlib.artist import setp as set_artist_property
from matplotlib.axes import Axes
from matplotlib.image import AxesImage
from matplotlib.gridspec import GridSpec
from matplotlib.transforms import Bbox, BboxTransform
import numpy as np

# local imports
from mantidqt.widgets.colorbar.colorbar import ColorbarWidget
from .cursortracker import CursorTracker


class LinePlotter:
    """
    Provides facilities to add line cut plots to an exist
    image axes
    """
    __slots__ = ("_axx", "_axy", "_canvas", "_colorbar", "_fig", "_im_ax", "_image", "_plot_impl",
                 "_xfig", "_yfig")

    def __init__(self, image_axes: Axes, colorbar: ColorbarWidget, plottype_cls: Any):
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
        self._image = image_axes.images[0]
        self._fig = image_axes.figure
        self._canvas = self._fig.canvas
        self._colorbar = colorbar
        self._plot_impl = plottype_cls(self)

        self._add_line_plots()

    @property
    def image(self):
        return self._image

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
            self.update_line_plot_labels()
            self.update_line_plot_limits()
        self._canvas.draw_idle()

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
            self.update_line_plot_labels()
            self.update_line_plot_limits()
        self._canvas.draw_idle()

    def update_line_plot_limits(self):
        """
        Update limits of line plots to match colorbar scale
        """
        # set line plot intensity axes to match colorbar limits
        self._axx.set_ylim(self._colorbar.cmin_value, self._colorbar.cmax_value)
        self._axy.set_xlim(self._colorbar.cmin_value, self._colorbar.cmax_value)

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
        # self.mpl_toolbar.update()  # sync list of axes in navstack
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


class PixelLinePlot(CursorTracker):
    """
    Draws X/Y line plots from single rows/columns of an image.
    """
    def __init__(self, plotter: LinePlotter):
        """
        :param plotter: The object responsible for drawing the curves.
                        Must have plot_x_line & plot_y_line methods.
        """
        super().__init__(image_axes=plotter.image_axes)
        self._plotter = plotter

    # CursorTracker interface
    def on_cursor_at(self, xdata: float, ydata: float):
        """
        Notify the object that a new event has occurred at the given coordinates
        :param xdata: X position in data coordinates
        :param ydata: Y position in data coordinates
        """
        plotter = self._plotter
        cinfo = cursor_info(plotter.image, xdata, ydata)
        if cinfo is not None:
            arr, (xmin, xmax, ymin, ymax), (i, j) = cinfo
            if 0 <= i < arr.shape[0]:
                plotter.plot_x_line(np.linspace(xmin, xmax, arr.shape[1]), arr[i, :])
            if 0 <= j < arr.shape[1]:
                plotter.plot_y_line(np.linspace(ymin, ymax, arr.shape[0]), arr[:, j])

    def on_cursor_outside_axes(self):
        """
        Called when the mouse moves outside of the image axes
        """
        self._plotter.delete_line_plot_lines()


# Data type to store information related to a cursor over an image
CursorInfo = namedtuple("CursorInfo", ("array", "extent", "point"))


@lru_cache(maxsize=32)
def cursor_info(image: AxesImage, xdata: float, ydata: float) -> Optional[CursorInfo]:
    """Return information on the image for the given position in
    data coordinates.
    :param image: An instance of an image type
    :param xdata: X data coordinate of cursor
    :param xdata: Y data coordinate of cursor
    :return: None if point is not valid on the image else return CursorInfo type
    """
    extent = image.get_extent()
    xmin, xmax, ymin, ymax = image.get_extent()
    arr = image.get_array()
    data_extent = Bbox([[extent[2], extent[0]], [extent[3], extent[1]]])
    array_extent = Bbox([[0, 0], arr.shape[:2]])
    trans = BboxTransform(boxin=data_extent, boxout=array_extent)
    point = trans.transform_point([ydata, xdata])
    if any(np.isnan(point)):
        return None

    return CursorInfo(array=arr, extent=extent, point=point.astype(int))
