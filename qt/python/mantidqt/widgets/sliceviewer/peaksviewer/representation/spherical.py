# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
# std imports
from json import loads as json_loads

# 3rdparty imports

# local imports
from .base import PeakDrawable, PeakRepresentation
from .noshape import PeakDrawableNoShape


class PeakDrawableCircle(PeakDrawable):
    """Draws a circle"""

    def __init__(self, radius):
        self._radius = radius

    def draw_impl(self, painter, peak):
        """
        Draw this peak with the given Painter
        :param painter: A painter that understands how to draw
        :param peak: A reference to a Peak representation
        :return: The new Artist added
        """
        return painter.circle(
            peak.x,
            peak.y,
            self._radius,
            alpha=peak.alpha,
            linestyle="--",
            facecolor='none',
            edgecolor=peak.fg_color)


class SphericallyIntergratedPeakRepresentation(PeakRepresentation):
    """"Extends the default PeakRepresentation adding radius properties
    describing a Spherical integration"""
    # Additional "distance" to add to snap view on top of peak radius
    SNAP_VIEW_PAD = 0.25

    @classmethod
    def create(cls, x, y, z, alpha, peak_shape, fg_color):
        """
        Create an instance of NonIntegratedPeakRepresentation from the given set of attributes
        :param x: Position of peak in X dimension
        :param y: Position of peak in Y dimension
        :param z: Position of peak in Z dimension
        :param alpha: Initial alpha value
        :param shape: A PeakShape object describing the shape properties (unused)
        :param fg_color: A string indicating the color of the marker
        :return: A new instance of this class
        """
        shape_info = json_loads(peak_shape.toJSON())
        radius = float(shape_info["radius"])
        drawables = (PeakDrawableNoShape(radius * 0.1), PeakDrawableCircle(radius))
        return cls(x, y, z, alpha, fg_color, drawables, snap_width=radius + cls.SNAP_VIEW_PAD)

    def __init__(self, x, y, z, alpha, fg_color, drawables, snap_width):
        """
        See PeakRepresentation.__init__ for other parameters
        :param snap_width: The width of the view when snapped to
        """
        super().__init__(x, y, z, alpha, fg_color, drawables)
        self._snap_width = snap_width

    def snap_width(self):
        """Return the width of the view when a peak is zoomed to"""
        return self._snap_width
