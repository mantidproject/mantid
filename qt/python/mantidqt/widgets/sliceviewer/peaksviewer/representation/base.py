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


def value_or_error(props, key):
    """Lookup a key in the properties and return it if it exists or raise a RuntimeError
    with an appropriate message"""
    try:
        return props[key]
    except KeyError:
        raise RuntimeError("Unable to find '{}' property for given PeakShape".format(key))


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
