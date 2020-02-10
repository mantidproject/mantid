# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, unicode_literals)

# std imports
from abc import ABCMeta, abstractmethod
from json import loads as json_loads
from six import with_metaclass

# 3rdparty imports
from matplotlib.patches import Circle

# default transparency
PEAK_ALPHA = 0.8


class PeakRepresentation(with_metaclass(ABCMeta)):
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

    @abstractmethod
    def draw(self):
        """
        Draw this peak on the given axes
        :param ax: An Axes object to accept the draw command
        :return: The new Artist/Artists added
        """
        pass


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


class PeakRepresentationSphere(PeakRepresentation):
    """Describes a view of a Peak that has been integrated with a Spherical shape"""
    def __init__(self, radius, center, alpha, marker_color):
        """
        :param radius of the circle in units of the Axes
        :param center: A V3D defining the center
        :param alpha: A float between 0.0, 1.0 defining the transparency of the line
        :param marker_color: A str code defining the color of the peak marker
        """
        super(PeakRepresentationSphere, self).__init__(center, alpha, marker_color)
        self._radius = radius

    @classmethod
    def create(cls, center, peak_shape, marker_color):
        """
        Create an instance of PeakRepresentationSphere from the given peak object
        :param center: A V3D giving the center of the Peak
        :param peak_shape: A PeakShape object describing the shape properties
        :param marker_color: A string indicating the color of the marker
        :return: A new instance of this class
        """
        properties = json_loads(peak_shape.toJSON())
        try:
            return PeakRepresentationSphere(center=center,
                                            radius=properties["radius"],
                                            alpha=PEAK_ALPHA,
                                            marker_color=marker_color)
        except KeyError:
            raise RuntimeError("Unable to find 'radius' property for given PeakShape")

    @property
    def radius(self):
        return self._radius

    def draw(self, ax):
        """
        Override bse class method to draw spherical integration region with a background
        indicated by a dotted circle
        :param ax: An Axes object to accept the draw command
        :return: The new Artist/Artists added
        """
        center = self.center
        return ax.add_patch(
            Circle((center.X(), center.Y()),
                   radius=self.radius,
                   alpha=self.alpha,
                   facecolor='none',
                   edgecolor=self.marker_color))


# map shape names to representation classes
# the strings need to match whatever Peak.getPeakShape.shapeName returns
_PEAK_REPRESENTATIONS = {
    "none": PeakRepresentationNoShape,
    "spherical": PeakRepresentationSphere,
}


def create_peakrepresentation(peak, marker_color):
    """
    A factory function to create an appropriate PeakRepresentation
    object for a peak.
    :param peak: A Peak object
    :param marker_color: A str representing the color of the peak shape marker
    :returns: A PeakRepresentation object describing the Peak aspects
              important for display
    """
    peak_shape = peak.getPeakShape()
    cls = _PEAK_REPRESENTATIONS[peak_shape.shapeName().lower()]
    return cls.create(peak.getQLabFrame(), peak_shape, marker_color)
