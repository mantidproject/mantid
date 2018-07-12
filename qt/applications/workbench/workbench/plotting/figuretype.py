#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""
Provides facilities to check plot types
"""
from __future__ import absolute_import

from mantidqt.py3compat import Enum


class FigureType(Enum):
    """Enumerate possible types of Figure"""
    # The values are irrelevant

    # A figure with no axes
    Empty = 0
    # Line plot without error bars
    Line = 1
    # Line plot with error bars
    Errorbar = 2
    # An image plot from imshow, pcolor
    Image = 3
    # Any other type of plot
    Other = 100


def axes_type(ax):
    """Return the plot type contained in the axes

    :param ax: A matplotlib.axes.Axes instance
    :return: An enumeration defining the plot type
    """
    try:
        nrows, ncols, _ = ax.get_geometry()
    except AttributeError:
        nrows, ncols = 1, 1

    axtype = FigureType.Other
    if nrows == 1 and ncols == 1:
        if len(ax.lines) > 0:
            axtype = FigureType.Line
        elif len(ax.images) > 0:
            axtype = FigureType.Image

    return axtype


def figure_type(fig):
    """Return the type of the figure

    :param fig: A matplotlib figure instance
    :return: An enumeration defining the plot type
    """
    all_axes = fig.axes
    all_axes_length = len(all_axes)
    if all_axes_length == 0:
        return FigureType.Empty
    elif all_axes_length == 1:
        return axes_type(all_axes[0])
    else:
        return FigureType.Other
