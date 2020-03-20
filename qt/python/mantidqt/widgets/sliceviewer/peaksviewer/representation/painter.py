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


class Painter(with_metaclass(ABCMeta)):
    """
    Interface defined for types capable of drawing
    representations of different shapes
    """

    @abstractmethod
    def remove(self, artifact):
        """
        Remove the painted artist from the drawn destination
        """
        pass

    @abstractmethod
    def scatter(self, x, y, **kwargs):
        """
        Draw a scatter point at the given location
        :param x: X location of scatter point
        :param y: Y location of scatter point
        :param kwargs: Additional properties to control the display
        """
        pass

    @abstractmethod
    def snap_to(self, x, y):
        """
        Set the display such the point (x, y) is at the center
        :param x: X location of scatter point
        :param y: Y location of scatter point
        """
        pass

    @abstractmethod
    def update_properties(self, artist, **kwargs):
        """
        Update artist properties
        :param artist: Reference to the Artist
        :param kwargs: Keywords giving Artist properties to change
        """
        pass


class MplPainter(Painter):
    """
    Implementation of a PeakPainter that uses matplotlib to draw
    """
    SNAP_WIDTH = 0.5

    def __init__(self, axes):
        """
        :param axes: A matplotlib.axes.Axes instance to draw on
        """
        if not hasattr(axes, "scatter"):
            raise ValueError("Expected matplotlib.axes.Axes instance. Found {}.".format(type(axes)))
        self._axes = axes

    def remove(self, artist):
        """
        Remove the painted artifact from the drawn destination
        :param artist: The Artist drawn on the axes
        """
        try:
            artist.remove()
        except ValueError:
            # May have already been removed by a figure/axes clear
            pass

    def scatter(self, x, y, **kwargs):
        """Draw a scatter point at the given location
        See PeakPainter for description of arguments
        """
        return self._axes.scatter(x, y, **kwargs)

    def snap_to(self, x, y):
        """
        Set the display such the point (x, y) is at the center
        :param x: X location of scatter point
        :param y: Y location of scatter point
        """
        snap_width = self.SNAP_WIDTH

        def snap_limits(center):
            return (center - snap_width, center + snap_width)

        self._axes.set_xlim(*snap_limits(x))
        self._axes.set_ylim(*snap_limits(y))

    def update_properties(self, artist, **kwargs):
        """
        Update artist properties
        :param artist: Reference to the Artist
        :param kwargs: Keywords giving Artist properties to change
        """
        artist.set(**kwargs)
