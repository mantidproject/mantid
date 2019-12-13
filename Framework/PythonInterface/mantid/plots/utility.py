# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
from __future__ import absolute_import

from contextlib import contextmanager

from matplotlib import cm
from matplotlib.container import ErrorbarContainer

from enum import Enum


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
