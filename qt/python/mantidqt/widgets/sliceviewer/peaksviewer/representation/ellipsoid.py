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

    @property
    def width(self):
        return self._width

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


class PeakDrawableEllipticalShell(PeakDrawable):
    """Draws an elliptical shell to represent a background integration region of Peak"""

    def __init__(self, outer_width, outer_height, thick, angle):
        """
        :param outer_width: Size of outer ellipse in X dimension
        :param outer_height: Size of outer ellipse in Y dimension
        :param thick: Thickness of shell
        :param angle: Angle width to X axis
        """
        self._width = outer_width
        self._height = outer_height
        self._thick = thick
        self._angle = angle

    @property
    def width(self):
        return self._width

    def draw_impl(self, painter, peak):
        """
        Draw this peak with the given Painter
        :param painter: A painter that understands how to draw
        :param peak: A reference to a Peak representation
        :return: The new Artist added
        """
        return painter.elliptical_shell(
            peak.x,
            peak.y,
            self._width,
            self._height,
            self._thick,
            self._angle,
            alpha=peak.alpha,
            linestyle="--",
            facecolor=peak.bg_color,
            edgecolor='none')


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
        signal, background = _shape_info(peak_shape)
        drawables = [PeakDrawableNoShape(signal.width * 0.1), signal, background]
        snap_width = cls.SNAP_VIEW_PAD + background.width

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
    signal = PeakDrawableEllipse(width, height, angle)

    bkgd_inner_width = 2 * float(shape_info["background_inner_radius0"])
    bkgd_outer_width = 2 * float(shape_info["background_outer_radius0"])
    bkgd_outer_height = 2 * float(shape_info["background_outer_radius1"])
    thick = bkgd_outer_width - bkgd_inner_width
    background = PeakDrawableEllipticalShell(bkgd_outer_width, bkgd_outer_height, thick, angle)

    return signal, background
