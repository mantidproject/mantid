# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
# std imports
from collections import namedtuple
from json import loads as json_loads

# 3rdparty imports

# local imports
from .base import PeakDrawable, IntegratedPeakRepresentation
from .noshape import PeakDrawableNoShape

# ellipse
Ellipse = namedtuple("Ellipse", ("width", "height", "angle"))


class PeakDrawableEllipse(PeakDrawable):
    """Draws an ellipse to represent an integration region of Peak"""

    def __init__(self, width, height, angle):
        """
        :param width: Size of ellipse in X dimension
        :param height: Size of ellipse in Y dimension
        :param angle: Angle width to X axis
        """
        self._width = width
        self._height = height
        self._angle = angle

    def draw_impl(self, painter, peak):
        """
        Draw this peak with the given Painter
        :param painter: A painter that understands how to draw
        :param peak: A reference to a Peak representation
        :return: The new Artist added
        """
        return painter.ellipse(
            peak.x,
            peak.y,
            self._width,
            self._height,
            self._angle,
            alpha=peak.alpha,
            linestyle="--",
            facecolor="none",
            edgecolor=peak.fg_color)


class EllipsoidalIntergratedPeakRepresentation(IntegratedPeakRepresentation):
    """"Extends the default PeakRepresentation adding radius properties
    describing Ellipsoidal integration"""
    # Additional "distance" to add to snap view on top of peak radius
    SNAP_VIEW_PAD = 0.25

    @classmethod
    def create(cls, x, y, z, alpha, peak_shape, fg_color, bg_color):
        """
        Create an instance of NonIntegratedPeakRepresentation from the given set of attributes
        :param x: Position of peak in X dimension
        :param y: Position of peak in Y dimension
        :param z: Position of peak in Z dimension
        :param alpha: Initial alpha value
        :param shape: A PeakShape object describing the shape properties (unused)
        :param fg_color: A string indicating the color of the marker
        :param: bg_color: A string indicating the color of the optionl background region
        :return: A new instance of this class
        """
        signal, bg_shell = _shape_info(peak_shape)
        drawables = [
            PeakDrawableNoShape(signal.width * 0.1),
            PeakDrawableEllipse(signal.width, signal.height, signal.angle)
        ]
        snap_width = cls.SNAP_VIEW_PAD + signal.width

        return cls(x, y, z, alpha, fg_color, bg_color, drawables, snap_width)


def _shape_info(peak_shape):
    """
    Parse shape information and return basic information on the geometry
    :param peak_shape: A reference to a PeakShape object
    :returns: An Ellipse an optional EllipticalShell object for the background region
    """
    shape_info = json_loads(peak_shape.toJSON())
    width = 2 * float(shape_info["radius0"])
    height = 2 * float(shape_info["radius1"])
    angle = 0.0

    return Ellipse(width, height, angle), None
