# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import absolute_import, division, unicode_literals

# std imports
from abc import ABCMeta, abstractmethod

# 3rdparty imports
from six import with_metaclass

# default transparency
PEAK_ALPHA = 0.8
# transparency range
ALPHA_MIN, ALPHA_MAX = 0.0, 0.8
# fraction of view occupied by marker
VIEW_FRACTION = 0.015


def value_or_error(props, key):
    """Lookup a key in the properties and return it if it exists or raise a RuntimeError
    with an appropriate message"""
    try:
        return props[key]
    except KeyError:
        raise RuntimeError("Unable to find '{}' property for given PeakShape".format(key))


class PeakRepresentation(with_metaclass(ABCMeta)):
    """Describes representation of Peak for display"""
    def __init__(self, x, y, z, alpha, marker_color):
        """
        :param x: X position of center in slice plane
        :param y: Y position of center in slice plane
        :param z: Z position of center out of slice plane
        :param alpha: A float between 0.0, 1.0 defining the transparency
        :param marker_color: A str code defining the color of the peak marker
        """
        self._alpha = alpha
        self._x, self._y, self._z = x, y, z
        self._marker_color = marker_color

    @property
    def alpha(self):
        return self._alpha

    @property
    def marker_color(self):
        return self._marker_color

    @property
    def x(self):
        return self._x

    @property
    def y(self):
        return self._y

    @property
    def z(self):
        return self._z

    @classmethod
    def compute_alpha(cls, z, slicepoint, slicedim_width):
        """Calculate the alpha value based on the peak position and slicepoint
        :param z: Z position of center out of slice plane
        :param slicepoint: float giving current slice point
        :param slicedim_width:
        :returns: float transparency value in range 0.0->1.0
        """
        # Apply a linear transform to convert from a distance to an opacity between
        # alpha min & max
        gradient = (ALPHA_MIN - ALPHA_MAX) / (slicedim_width * VIEW_FRACTION)
        distance = abs(slicepoint - z)
        alpha = (gradient * distance) + ALPHA_MAX
        # < 0 then assume peak is not visible
        if alpha < 0.0:
            alpha = 0.0
        elif alpha > 1.0:
            alpha = 1.0

        return alpha

    @abstractmethod
    def draw(self, painter):
        """
        Draw this peak on the given axes
        :param ax: An Axes object to accept the draw command
        :return: The new Artist/Artists added
        """
        pass

    @abstractmethod
    def remove(self, painter):
        """
        Remove the painted artist
        """
        pass
