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
from matplotlib.patches import Ellipse

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
        return ax.scatter(
            center.X(),
            center.Y(),
            alpha=self.alpha,
            color=self.marker_color,
            marker=self.MARKER,
            s=self.MARKER_SIZE_PTS_SQ)


class PeakRepresentationEllipsoid(PeakRepresentation):
    """Describes a 2D view of a Peak that has been integrated with an Ellipsoid shape"""

    def __init__(self, width, height, angle, center, alpha, marker_color):
        """
        :param width: Length of horizontal axis
        :param height: Length of vertical axis
        :param center: A V3D defining the center
        :param alpha: A float between 0.0, 1.0 defining the transparency of the line
        :param marker_color: A str code defining the color of the peak marker
        """
        super(PeakRepresentationEllipsoid, self).__init__(center, alpha, marker_color)
        self._angle = angle
        self._height = height
        self._width = width

    @property
    def width(self):
        return self._width

    @property
    def height(self):
        return self._height

    @classmethod
    def create(cls, center, peak_shape, marker_color):
        """
        Create an instance of PeakRepresentationEllipsoid from the given peak object.
        :param center: Center of ellipsoid
        :param peak_shape: Reference to a PeakShapeEllipsoid object
        :param marker_color: String describing color
        :return: A new instance of this class
        """

        def value_or_error(props, key):
            try:
                return props[key]
            except KeyError:
                raise RuntimeError("Unable to find '{}' property for given PeakShape".format(key))

        properties = json_loads(peak_shape.toJSON())
        return PeakRepresentationEllipsoid(
            center=center,
            width=2 * value_or_error(properties, "radius0"),
            height=2 * value_or_error(properties, "radius1"),
            angle=0.0,
            alpha=PEAK_ALPHA,
            marker_color=marker_color)

    def draw(self, ax):
        """
        Override base class method to draw spherical integration region with a background
        indicated by a dotted circle
        :param ax: An Axes object to accept the draw command
        :return: The new Artist/Artists added
        """
        center = self.center
        return ax.add_patch(
            Ellipse((center.X(), center.Y()),
                    width=self._width,
                    height=self._height,
                    facecolor='none',
                    edgecolor=self.marker_color))


class PeakRepresentationSphere(PeakRepresentationEllipsoid):
    """Describes a view of a Peak that has been integrated with a Spherical shape.
    Uses PeakRepresentationEllipsoid to draw.
    """

    def __init__(self, radius, center, alpha, marker_color):
        """
        See PeakRepresentationEllipsoid for description of other parameters
        :param radius: Radius of circle
        """
        super(PeakRepresentationSphere, self).__init__(2 * radius, 2 * radius, 0.0, center, alpha,
                                                       marker_color)

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
            return PeakRepresentationSphere(
                radius=properties["radius"],
                center=center,
                alpha=PEAK_ALPHA,
                marker_color=marker_color)
        except KeyError:
            raise RuntimeError("Unable to find 'radius' property for given PeakShape")

    @property
    def radius(self):
        return 0.5 * self._width


# map shape names to representation classes
# the strings need to match whatever Peak.getPeakShape.shapeName returns
_PEAK_REPRESENTATIONS = {
    "none": PeakRepresentationNoShape,
    "spherical": PeakRepresentationSphere,
    "ellipsoidal": PeakRepresentationEllipsoid
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
