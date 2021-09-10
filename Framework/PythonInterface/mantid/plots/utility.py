# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
# std imports
import collections
from contextlib import contextmanager
from enum import Enum
from distutils.version import LooseVersion

# 3rd party imports
from matplotlib.legend import Legend
from matplotlib import cm, __version__ as mpl_version_str
from matplotlib.container import ErrorbarContainer

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
# matplotlib version information
MPLVersionInfo = collections.namedtuple("MPLVersionInfo", ("major", "minor", "patch"))
MATPLOTLIB_VERSION_INFO = MPLVersionInfo._make(map(int, mpl_version_str.split(".")))

# Restrict zooming out of plots.
ZOOM_LIMIT = 1e300

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
def autoscale_on_update(ax, state, axis='both'):
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
            if axis == 'both':
                original_state = ax.get_autoscale_on()
                ax.set_autoscale_on(state)
            elif axis == 'x':
                original_state = ax.get_autoscalex_on()
                ax.set_autoscalex_on(state)
            elif axis == 'y':
                original_state = ax.get_autoscaley_on()
                ax.set_autoscaley_on(state)
        yield
    finally:
        if ax.lines:
            if axis == 'both':
                ax.set_autoscale_on(original_state)
            elif axis == 'x':
                ax.set_autoscalex_on(original_state)
            elif axis == 'y':
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
    return sorted([cmap for cmap in cm.cmap_d.keys() if not cmap.endswith('_r')])


def get_errorbar_containers(ax):
    return [e_cont for e_cont in ax.containers
            if isinstance(e_cont, ErrorbarContainer)]


def get_autoscale_limits(ax, axis):
    """
    Get limits that would be set by autoscale.

    This is a trimmed down  version of the function 'handle_single_axis'
    that is found within 'matplotlib.axes._base._AxesBase.autoscale_view'
    """
    axis_min, axis_max = getattr(ax.dataLim, 'interval{}'.format(axis))
    ax_margin = getattr(ax, '_{}margin'.format(axis))
    if ax_margin > 0:
        padding = (axis_max - axis_min)*ax_margin
        return axis_min - padding, axis_max + padding
    if not ax._tight:
        locator = getattr(ax, '{}axis'.format(axis)).get_major_locator()
        return locator.view_limits(axis_min, axis_max)


def legend_set_draggable(legend, state, use_blit=False, update='loc'):
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


def mpl_version_info():
    """Returns a namedtuple of (major,minor,patch)"""
    return MATPLOTLIB_VERSION_INFO


def row_num(ax):
    """
    Returns the row number of an input axes with relation to a gridspec
    Version check to avoid calling depreciated method in matplotlib > 3.2
    """
    if LooseVersion(mpl_version_str) >= LooseVersion("3.2.0"):
        return ax.get_subplotspec().rowspan.start
    else:
        return ax.rowNum


def col_num(ax):
    """
    Returns the column number of an input axes with relation to a gridspec
    Version check to avoid calling depreciated method in matplotlib > 3.2
    """
    if LooseVersion(mpl_version_str) >= LooseVersion("3.2.0"):
        return ax.get_subplotspec().colspan.start
    else:
        return ax.colNum


def zoom_axis(ax, coord, x_or_y, factor):
    """
    Zoom in around the value 'coord' along the given axis.

    :param matplotlib.axes.Axes ax: The Axes object to zoom in on
    :param float coord: The value in the axis to zoom around
    :param str x_or_y: The axis to zoom along ('x' or 'y')
    :param float factor: The factor by which to zoom in, a factor less than 1 zooms out
    """
    if x_or_y.lower() not in ['x', 'y']:
        raise ValueError("Can only zoom on axis 'x' or 'y'. Found '{}'."
                         "".format(x_or_y))
    get_lims = getattr(ax, "get_{}lim".format(x_or_y.lower()))
    set_lims = getattr(ax, "set_{}lim".format(x_or_y.lower()))

    ax_min, ax_max = get_lims()
    dist_to_min = coord - ax_min
    dist_to_max = ax_max - coord
    new_ax_min = coord - dist_to_min/factor
    new_ax_max = coord + dist_to_max/factor

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
    return zoom_axis(ax, x, 'x', factor), zoom_axis(ax, y, 'y', factor)


def get_single_workspace_log_value(ws_index, *, log_values=None, matrix_ws=None, log_name=None):
    if log_values is None:
        if log_name in ["Workspace index", "Workspace", ""]:
            return ws_index

        return matrix_ws.run().getPropertyAsSingleValueWithTimeAveragedMean(log_name)
    else:
        if ws_index >= len(log_values):
            return 0

        return log_values[ws_index]


def colormap_as_plot_color(number_colors: int, colormap_name: str = 'viridis', cmap=None):
    if not cmap:
        cmap = cm.get_cmap(name=colormap_name)

    for i in range(number_colors):
        yield cmap(float(i) / number_colors)
