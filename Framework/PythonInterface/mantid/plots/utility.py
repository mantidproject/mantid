# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
# std imports
from contextlib import contextmanager
from enum import Enum

# 3rd party imports
from matplotlib import colors
from matplotlib.legend import Legend
from matplotlib import colormaps
from matplotlib.container import ErrorbarContainer
from mantid.kernel import ConfigService

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
# Restrict zooming out of plots.
ZOOM_LIMIT = 1e300

MARKER_MAP = {
    "square": "s",
    "plus (filled)": "P",
    "point": ".",
    "tickdown": 3,
    "triangle_right": ">",
    "tickup": 2,
    "hline": "_",
    "vline": "|",
    "pentagon": "p",
    "tri_left": "3",
    "caretdown": 7,
    "caretright (centered at base)": 9,
    "tickright": 1,
    "caretright": 5,
    "caretleft": 4,
    "tickleft": 0,
    "tri_up": "2",
    "circle": "o",
    "pixel": ",",
    "caretleft (centered at base)": 8,
    "diamond": "D",
    "star": "*",
    "hexagon1": "h",
    "octagon": "8",
    "hexagon2": "H",
    "tri_right": "4",
    "x (filled)": "X",
    "thin_diamond": "d",
    "tri_down": "1",
    "triangle_left": "<",
    "plus": "+",
    "triangle_down": "v",
    "triangle_up": "^",
    "x": "x",
    "caretup": 6,
    "caretup (centered at base)": 10,
    "caretdown (centered at base)": 11,
    "None": "None",
}
# Use the correct draggable method based on the matplotlib version
if hasattr(Legend, "set_draggable"):
    SET_DRAGGABLE_METHOD = "set_draggable"
else:
    SET_DRAGGABLE_METHOD = "draggable"


# Any changes here must be reflected in the definition in
# the C++ MplCpp/Plot.h header. See the comment in that file
# for the reason for duplication.
class MantidAxType(Enum):
    """Define an list of possible axis types for plotting"""

    BIN = 0
    SPECTRUM = 1


@contextmanager
def artists_hidden(artists):
    """Context manager that hides matplotlib artists."""
    hidden = []
    for artist in artists:
        try:
            if artist.get_visible():
                hidden.append(artist)
                artist.set_visible(False)
        except AttributeError:
            pass
    try:
        yield
    finally:
        for artist in hidden:
            artist.set_visible(True)


@contextmanager
def autoscale_on_update(ax, state, axis="both"):
    """
    Context manager to temporarily change value of autoscale_on_update
    :param matplotlib.axes.Axes ax: The axes to disable autoscale on
    :param bool state: True turns auto-scaling on, False off
    :param str axis: {'both', 'x', 'y'} The axis to set the scaling on
    """
    original_state = ax.get_autoscale_on()
    try:
        # If we are making the first plot on an axes object
        # i.e. ax.lines is empty, axes has default ylim values.
        # Therefore we need to autoscale regardless of state parameter.
        if ax.lines:
            if axis == "both":
                original_state = ax.get_autoscale_on()
                ax.set_autoscale_on(state)
            elif axis == "x":
                original_state = ax.get_autoscalex_on()
                ax.set_autoscalex_on(state)
            elif axis == "y":
                original_state = ax.get_autoscaley_on()
                ax.set_autoscaley_on(state)
        yield
    finally:
        if ax.lines:
            if axis == "both":
                ax.set_autoscale_on(original_state)
            elif axis == "x":
                ax.set_autoscalex_on(original_state)
            elif axis == "y":
                ax.set_autoscaley_on(original_state)


def find_errorbar_container(line, containers):
    """
    Finds the ErrorbarContainer associated with the plot line.

    :param line: Line that is looked for
    :param containers: Collection of containers that contain `ErrorbarContainer`s
    :return: The container that contains the line
    """
    for container in containers:
        if line == container[0]:
            return container


def get_colormap_names():
    return sorted([cmap for cmap in colormaps.keys() if not cmap.endswith("_r")])


def get_errorbar_containers(ax):
    return [e_cont for e_cont in ax.containers if isinstance(e_cont, ErrorbarContainer)]


def get_autoscale_limits(ax, axis):
    """
    Get limits that would be set by autoscale.

    This is a trimmed down  version of the function 'handle_single_axis'
    that is found within 'matplotlib.axes._base._AxesBase.autoscale_view'
    """
    axis_min, axis_max = getattr(ax.dataLim, "interval{}".format(axis))
    ax_margin = getattr(ax, "_{}margin".format(axis))
    if ax_margin > 0:
        padding = (axis_max - axis_min) * ax_margin
        return axis_min - padding, axis_max + padding
    if not ax._tight:
        locator = getattr(ax, "{}axis".format(axis)).get_major_locator()
        return locator.view_limits(axis_min, axis_max)


def legend_set_draggable(legend, state, use_blit=False, update="loc"):
    """Utility function to support varying Legend api around draggable status across
    the versions of matplotlib we support. Function arguments match those from matplotlib.
    See matplotlib documentation for argument descriptions
    """
    getattr(legend, SET_DRAGGABLE_METHOD)(state, use_blit, update)


def get_current_cmap(object):
    """Utility function to support varying get_cmap api across
    the versions of matplotlib we support.
    """
    if hasattr(object, "cmap"):
        return object.cmap
    else:
        return object.get_cmap()


def row_num(ax):
    """
    Returns the row number of an input axes with relation to a gridspec
    """
    # An 'inset' axes does not have a subplotspec, so return None
    return ax.get_subplotspec().rowspan.start if ax.get_subplotspec() is not None else None


def col_num(ax):
    """
    Returns the column number of an input axes with relation to a gridspec
    """
    # An 'inset' axes does not have a subplotspec, so return None
    return ax.get_subplotspec().colspan.start if ax.get_subplotspec() is not None else None


def zoom_axis(ax, coord, x_or_y, factor):
    """
    Zoom in around the value 'coord' along the given axis.

    :param matplotlib.axes.Axes ax: The Axes object to zoom in on
    :param float coord: The value in the axis to zoom around
    :param str x_or_y: The axis to zoom along ('x' or 'y')
    :param float factor: The factor by which to zoom in, a factor less than 1 zooms out
    """
    if x_or_y.lower() not in ["x", "y"]:
        raise ValueError("Can only zoom on axis 'x' or 'y'. Found '{}'." "".format(x_or_y))
    get_lims = getattr(ax, "get_{}lim".format(x_or_y.lower()))
    set_lims = getattr(ax, "set_{}lim".format(x_or_y.lower()))

    ax_min, ax_max = get_lims()
    dist_to_min = coord - ax_min
    dist_to_max = ax_max - coord
    new_ax_min = coord - dist_to_min / factor
    new_ax_max = coord + dist_to_max / factor

    # Don't allow further zooming out if we're beyond the limit. Zooming in is allowed.
    # The abs in the second half of the conditional statements accounts for the case when the min and max axis limits
    # are flipped, which is possible in matplotlib.
    if abs(new_ax_max) > ZOOM_LIMIT and abs(new_ax_max) > abs(ax_max):
        new_ax_max = ax_max

    if abs(new_ax_min) > ZOOM_LIMIT and abs(new_ax_min) > abs(ax_min):
        new_ax_min = ax_min

    set_lims((new_ax_min, new_ax_max))

    return new_ax_min, new_ax_max


def zoom(ax, x, y, factor):
    """
    Zoom in around point (x, y) in Axes ax, i.e. point (x, y) maintains
    its position on the axes during the zoom.

    The range of each axis is scaled by 'factor', axis limits are then
    adjusted such that the ratio of distances between the point (x, y)
    and the edge of the each axis is constant. This keeps the point
    (x, y) in the same position within the axes' view.

    :param matplotlib.axes.Axes ax: The Axes object to zoom in on
    :param float x: The x coordinate to center the zoom around
    :param float y: The y coordinate to center the zoom around
    :param float factor: The factor by which to zoom in, a factor less
        than 1 zooms out
    """
    return zoom_axis(ax, x, "x", factor), zoom_axis(ax, y, "y", factor)


def get_single_workspace_log_value(ws_index, *, log_values=None, matrix_ws=None, log_name=None):
    if log_values is None:
        if log_name in ["Workspace index", "Workspace", ""]:
            return ws_index

        return matrix_ws.run().getPropertyAsSingleValueWithTimeAveragedMean(log_name)
    else:
        if ws_index >= len(log_values):
            return 0

        return log_values[ws_index]


def colormap_as_plot_color(number_colors: int, colormap_name: str = "viridis", cmap=None):
    if not cmap:
        cmap = colormaps[colormap_name]

    for i in range(number_colors):
        yield cmap(float(i) / number_colors)


def convert_color_to_hex(color):
    """Convert a matplotlib color to its hex form"""
    try:
        return colors.cnames[color]
    except (KeyError, TypeError):
        rgb = colors.colorConverter.to_rgb(color)
        return colors.rgb2hex(rgb)


def get_plot_specific_properties(ws, plot_type, plot_kwargs):
    """
    Set plot specific properties from the workspace
    :param ws:
    :param ax:
    :param errors:
    :param plot_kwargs:
    """

    if plot_type in ["errorbar_x", "errorbar_y", "errorbar_xy"]:
        plot_kwargs["linestyle"] = "None"
        plot_kwargs["marker"] = MARKER_MAP[ConfigService.getString("plots.errorbar.MarkerStyle")]
        plot_kwargs["markersize"] = float(ConfigService.getString("plots.errorbar.MarkerSize"))
        if "capsize" not in plot_kwargs:
            plot_kwargs["capsize"] = float(ConfigService.getString("plots.errorbar.Capsize"))
        if "capthick" not in plot_kwargs:
            plot_kwargs["capthick"] = float(ConfigService.getString("plots.errorbar.CapThickness"))
        if "errorevery" not in plot_kwargs:
            plot_kwargs["errorevery"] = int(ConfigService.getString("plots.errorbar.errorEvery"))
        if "elinewidth" not in plot_kwargs:
            plot_kwargs["elinewidth"] = float(ConfigService.getString("plots.errorbar.Width"))
    else:
        if plot_type == "marker":
            plot_kwargs["linestyle"] = "None"
            if ws.getMarkerStyle():
                plot_kwargs["marker"] = MARKER_MAP[ws.getMarkerType()]
            else:
                plot_kwargs["marker"] = MARKER_MAP[ConfigService.getString("plots.markerworkspace.MarkerStyle")]
            marker_size = ws.getMarkerSize()
            plot_kwargs["markersize"] = (
                marker_size if marker_size != 6 else float(ConfigService.getString("plots.markerworkspace.MarkerSize"))
            )
        plot_kwargs.pop("capsize", None)
        plot_kwargs.pop("capthick", None)
        plot_kwargs.pop("errorevery", None)
        plot_kwargs.pop("elinewidth", None)

    return plot_kwargs
