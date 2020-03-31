# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports

# 3rdparty imports
from matplotlib.path import Path
from matplotlib.patches import Circle, Ellipse, PathPatch, Wedge


class MplPainter(object):
    """
    Implementation of a PeakPainter that uses matplotlib to draw
    """
    SNAP_WIDTH = 0.5

    def __init__(self, axes):
        """
        :param axes: A matplotlib.axes.Axes instance to draw on
        """
        if not hasattr(axes, "scatter"):
            raise ValueError("Expected matplotlib.axes.Axes instance. Found {}.".format(type(axes)))
        self._axes = axes

    def remove(self, artist):
        """
        Remove the painted artifact from the drawn destination
        :param artists: The Artists drawn on the axes
        """
        try:
            artist.remove()
        except ValueError:
            # May have already been removed by a figure/axes clear
            pass

    def circle(self, x, y, radius, **kwargs):
        """Draw a circle on the Axes
        :param x: X coordinate of the center
        :param y: Y coordinate of the center
        :param radius: Radius of the circle
        :param kwargs: Additional matplotlib properties to pass to the call
        """
        return self._axes.add_patch(Circle((x, y), radius, **kwargs))

    def cross(self, x, y, half_width, **kwargs):
        """Draw a cross at the given location
        :param x: X coordinate of the center
        :param y: Y coordinate of the center
        :param half_width: Half-width of cross
        :param kwargs: Additional matplotlib properties to pass to the call
        """
        verts = ((x - half_width, y + half_width), (x + half_width, y - half_width),
                 (x + half_width, y + half_width), (x - half_width, y - half_width))
        codes = (Path.MOVETO, Path.LINETO, Path.MOVETO, Path.LINETO)
        return self._axes.add_patch(PathPatch(Path(verts, codes), **kwargs))

    def ellipse(self, x, y, width, height, angle=0.0, **kwargs):
        """Draw an ellipse at the given location
        :param x: X coordinate of the center
        :param y: Y coordinate of the center
        :param width: Size in X dimension
        :param height: Size in Y dimension
        :param angle: Angle in degrees w.r.t X axis
        :param kwargs: Additional matplotlib properties to pass to the call
        """
        return self._axes.add_patch(Ellipse((x, y), width, height, angle, **kwargs))

    def shell(self, x, y, outer_radius, thick, **kwargs):
        """Draw a wedge on the Axes
        :param x: X coordinate of the center
        :param y: Y coordinate of the center
        :param outer_radius: Radius of the circle outer edge of the shell
        :param thick: The thickness of the shell
        :param kwargs: Additional matplotlib properties to pass to the call
        """
        return self._axes.add_patch(
            Wedge((x, y), outer_radius, theta1=0.0, theta2=360., width=thick, **kwargs))

    def snap_to(self, x, y, width):
        """
        Set the display such the point (x, y) is at the center with width
        :param x: X location of scatter point
        :param y: Y location of scatter point
        :param width: The width/height of the view
        """

        def snap_limits(center):
            return (center - width, center + width)

        self._axes.set_xlim(*snap_limits(x))
        self._axes.set_ylim(*snap_limits(y))

    def update_properties(self, artist, **kwargs):
        """
        Update artist properties
        :param artist: Reference to a list of the Artist
        :param kwargs: Keywords giving Artist properties to change
        """
        artist.set(**kwargs)
