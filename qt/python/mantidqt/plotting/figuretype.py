# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""
Provides facilities to check plot types
"""
from __future__ import absolute_import

# third party
from enum import Enum
from matplotlib.container import ErrorbarContainer


class FigureType(Enum):
    """Enumerate possible types of Figure"""
    # The values are irrelevant

    # A figure with no axes
    Empty = 0
    # Line plot without error bars
    Line = 1
    # Line plot with error bars
    Errorbar = 2
    # An image plot from imshow, pcolor, pcolormesh
    Image = 3
    # Any other type of plot
    Other = 100


def axes_type(ax):
    """Return the plot type contained in the axes

    :param ax: A matplotlib.axes.Axes instance
    :return: An enumeration defining the plot type
    """
    axtype = FigureType.Other

    # an errorbar plot also has len(lines) > 0
    if len(ax.containers) > 0 and isinstance(ax.containers[0], ErrorbarContainer):
        axtype = FigureType.Errorbar
    elif len(ax.lines) > 0:
        axtype = FigureType.Line
    elif len(ax.images) > 0 or len(ax.collections) > 0:
        axtype = FigureType.Image

    return axtype


def figure_type(fig, ax=None):
    """Return the type of the figure. It inspects the axes
    return by fig.gca()

    :param fig: A matplotlib figure instance
    :param ax: A matplotlib axes instance
    :return: An enumeration defining the plot type
    """
    if len(fig.get_axes()) == 0:
        return FigureType.Empty
    else:
        ax_types = [axes_type(axis) for axis in fig.axes]
        if any([type == FigureType.Image for type in ax_types]):
            return FigureType.Image
        elif ax:
            return axes_type(ax)
        else:
            return axes_type(fig.axes[0])
