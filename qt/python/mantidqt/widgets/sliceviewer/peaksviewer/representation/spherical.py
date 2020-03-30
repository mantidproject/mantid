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
from .base import PeakDrawable, IntegratedPeakRepresentation
from .noshape import PeakDrawableNoShape


class PeakDrawableCircle(PeakDrawable):
    """Draws a circle"""

    def __init__(self, radius):
        """
        :param radius: The radius of the circle
        """
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


class PeakDrawableShell(PeakDrawable):
    """Draws a shell of a given width and outer radius"""

    def __init__(self, outer_radius, thick):
        """
        :param outer_radius: The radius of the outer radius of the shell
        :param thick: The thickness of the shell
        """
        self._outer_radius = outer_radius
        self._thick = thick

    def draw_impl(self, painter, peak):
        """
        Draw this peak with the given Painter
        :param painter: A painter that understands how to draw
        :param peak: A reference to a Peak representation
        :return: The new Artist added
        """
        return painter.shell(
            peak.x,
            peak.y,
            self._outer_radius,
            self._thick,
            alpha=peak.alpha,
            linestyle="--",
            facecolor=peak.bg_color,
            edgecolor=peak.fg_color)


class SphericallyIntergratedPeakRepresentation(IntegratedPeakRepresentation):
    """"Extends the default PeakRepresentation adding radius properties
    describing a Spherical integration"""
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
        radius, bg_radii = _radius_info(peak_shape)
        drawables = [PeakDrawableNoShape(radius * 0.1), PeakDrawableCircle(radius)]
        snap_width = cls.SNAP_VIEW_PAD
        if bg_radii is not None:
            outer_radius = bg_radii[1]
            drawables.append(PeakDrawableShell(outer_radius, outer_radius - bg_radii[0]))
            snap_width += outer_radius
        else:
            snap_width += radius

        return cls(x, y, z, alpha, fg_color, bg_color, drawables, snap_width)


def _radius_info(peak_shape):
    """
    Parse shape information and return information on the radii
    :param peak_shape: A reference to a PeakShape object
    :returns: 2-tuple (radius, bg_radii) where radius is the signal
              integation region and bg_radii is the optional background
              integration region
    """
    shape_info = json_loads(peak_shape.toJSON())
    radius = float(shape_info["radius"])
    bg_inner_radius = shape_info.get("background_inner_radius", None)
    bg_outer_radius = shape_info.get("background_outer_radius", None)
    bg_radii = None
    if bg_inner_radius and bg_outer_radius:
        bg_radii = (float(bg_inner_radius), float(bg_outer_radius))

    return radius, bg_radii
