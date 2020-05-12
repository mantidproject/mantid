# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""
Provides facilities to check plot types
"""
# third party
from enum import Enum
from matplotlib.axes import Axes
from matplotlib.collections import LineCollection
from matplotlib.container import ErrorbarContainer
from mpl_toolkits.mplot3d.axes3d import Axes3D
from mpl_toolkits.mplot3d.art3d import Line3DCollection, Poly3DCollection


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
    # A 3D surface plot
    Surface = 4
    # A 3D wireframe plot
    Wireframe = 5
    # A contour plot
    Contour = 6
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
    elif isinstance(ax, Axes3D):
        if any(isinstance(col, Poly3DCollection) for col in ax.collections):
            axtype = FigureType.Surface
        elif any(isinstance(col, Line3DCollection) for col in ax.collections):
            axtype = FigureType.Wireframe
    elif len(ax.images) > 0 or len(ax.collections) > 0:
        if any(isinstance(col, LineCollection) for col in ax.collections):
            axtype = FigureType.Contour
        else:
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
        if ax:
            # If ax is a colorbar then find a non-colorbar axes on the figure so the plot type can be determined.
            if type(ax) == Axes:
                ax = next(axes for axes in fig.get_axes() if not type(axes) == Axes)

            return axes_type(ax)
        else:
            return axes_type(fig.axes[0])
