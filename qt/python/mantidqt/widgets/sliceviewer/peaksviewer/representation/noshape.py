# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import absolute_import, division, unicode_literals

# std imports

# 3rdparty imports

# local imports
from .base import PEAK_ALPHA, PeakRepresentation


class PeakRepresentationNoShape(PeakRepresentation):
    """Describes representation of Peak with no shape for display"""
    MARKER = 'x'
    MARKER_SIZE_PTS_SQ = 150

    @classmethod
    def create(cls, center, _, marker_color):
        """
        Create an instance of PeakRepresentationSphere from the given peak object
        :param center: A V3D giving the center of the Peak
        :param peak_shape: A PeakShape object describing the shape properties (unused)
        :param marker_color: A string indicating the color of the marker
        :return: A new instance of this class
        """
        return PeakRepresentationNoShape(center=center, alpha=PEAK_ALPHA, marker_color=marker_color)

    def draw(self, ax):
        """
        Override base method andd draws a single peak center marker
        :param ax: An Axes object to accept the draw command
        :return: The new Artist/Artists added
        """
        center = self.center
        return ax.scatter(center.X(),
                          center.Y(),
                          alpha=self.alpha,
                          color=self.marker_color,
                          marker=self.MARKER,
                          s=self.MARKER_SIZE_PTS_SQ)
