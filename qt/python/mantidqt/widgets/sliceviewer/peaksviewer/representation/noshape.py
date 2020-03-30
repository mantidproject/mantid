# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports

# 3rdparty imports

# local imports
from .base import PeakDrawable, PeakRepresentation


class PeakDrawableNoShape(PeakDrawable):
    """Describes representation of Peak with no shape for display"""
    SNAP_WIDTH = 0.5

    def __init__(self, half_width):
        """
        :param half_width: Width from center to the edge of cross
        """
        super().__init__()
        self._half_width = half_width

    @property
    def snap_width(self):
        """Return the width of the view when a peak is zoomed to"""
        return self.SNAP_WIDTH

    def draw_impl(self, painter, peak):
        """
        Override base method and draws a single peak center marker
        :param painter: A Painter object to accept the draw command
        :param peak: A reference to the PeakRepresentation object
        :return: The object added
        """
        return painter.cross(
            peak.x, peak.y, self._half_width, alpha=peak.alpha, color=peak.fg_color)


class NonIntegratedPeakRepresentation(PeakRepresentation):
    """Create a collection of PeakDrawable objects for a non-integrated Peak"""
    CROSS_HALF_WIDTH = 0.1

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
        return cls(
            x, y, z, alpha, fg_color, drawables=(PeakDrawableNoShape(cls.CROSS_HALF_WIDTH), ))
