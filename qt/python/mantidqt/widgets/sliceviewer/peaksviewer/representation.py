# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, unicode_literals)

# constants
PEAK_ALPHA = 0.8


class PeakRepresentation(object):
    """Describes representation of Peak for display"""

    def __init__(self, center, alpha, marker_color):
        """
        :param center: A V3D defining the center
        :param alpha: A float between 0.0, 1.0 defining the transparency
        :param marker_color: A str code defining the color of the peak marker
        """
        self._alpha = alpha
        self._center = center
        self._marker_color = marker_color

    @property
    def alpha(self):
        return self._alpha

    @property
    def center(self):
        return self._center

    @property
    def marker_color(self):
        return self._marker_color


class PeakRepresentationNoShape(PeakRepresentation):
    """Describes representation of Peak with no shape for display"""
    MARKER = 'x'
    MARKER_SIZE_PTS_SQ = 150

    def draw(self, ax):
        """
        Draw this peak on the given axes
        :param ax: An Axes object to accept the draw command
        :return: The new Artist/Artists added
        """
        center = self.center
        ax.scatter(center.X(), center.Y(),
                   alpha=self.alpha,color=self.marker_color,
                   marker=self.MARKER,
                   s=self.MARKER_SIZE_PTS_SQ)


def create_peakrepresentation(peak, marker_color):
    """
    A factory function to create an appropriate PeakRepresentation
    object for a peak.
    :param peak: A Peak object
    :param marker_color: A str representing the color of the peak shape marker
    :returns: A PeakRepresentation object describing the Peak aspects
              important for display
    """
    return PeakRepresentationNoShape(center=peak.getQLabFrame(),
                                     alpha=PEAK_ALPHA,
                                     marker_color=marker_color)
