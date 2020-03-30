# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports
from abc import ABCMeta, abstractmethod

# transparency range
ALPHA_MIN, ALPHA_MAX = 0.0, 0.8
# fraction of view occupied by marker
VIEW_FRACTION = 0.015


def compute_alpha(z, slicepoint, slicedim_width):
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


def value_or_error(props, key):
    """Lookup a key in the properties and return it if it exists or raise a RuntimeError
    with an appropriate message"""
    try:
        return props[key]
    except KeyError:
        raise RuntimeError("Unable to find '{}' property for given PeakShape".format(key))


class PeakDrawable(metaclass=ABCMeta):
    # reference to drawn object
    _drawn = None

    def draw(self, painter, peak):
        """
        Draw the peak with the given Painter. Real drawing is deferred to
        the derived class
        :param painter: A painter that understands how to draw
        :param peak: A reference to the PeakReference object being represented
        """
        self._drawn = self.draw_impl(painter, peak)

    @abstractmethod
    def draw_impl(self, painter, peak):
        """
        Override this and draw the shape for the peak with the given Painter.
        :param painter: A painter that understands how to draw
        :param peak: A reference to the PeakReference object being represented
        :return The _painted object
        """
        pass

    def remove(self, painter):
        """Remove this drawable from view"""
        if self._drawn is not None:
            painter.remove(self._drawn)
            self._drawn = None

    def repaint(self, painter, peak):
        """Repaint this drawable on the view
        :param painter: A painter that understands how to draw
        :param peak: A reference to the PeakReference object being represented
        """
        if self._drawn is not None:
            painter.update_properties(self._drawn, alpha=peak.alpha)
        else:
            self.draw(painter, peak)


class PeakRepresentation(object):
    """Describes the representation of Peak for display as a collection of shapes"""

    def __init__(self, x, y, z, alpha, fg_color, drawables):
        """
        :param x: X position of center in slice plane
        :param y: Y position of center in slice plane
        :param z: Z position of center out of slice plane
        :param alpha: A float between 0.0, 1.0 defining the transparency
        :param fg_color: A str code defining the color of the peak marker
        :param drawables: A collection of objects that together represent the Peak visually
        """
        self._alpha = alpha
        # Store the previous alpha value to test whether a Peak used to be visible
        self._alpha_previous = alpha
        self._x, self._y, self._z = x, y, z
        self._fg_color = fg_color
        self._drawables = drawables

    @property
    def alpha(self):
        return self._alpha

    @property
    def fg_color(self):
        return self._fg_color

    @property
    def x(self):
        return self._x

    @property
    def y(self):
        return self._y

    @property
    def z(self):
        return self._z

    def draw(self, painter):
        """
        Draw the peak with the given Painter. Real drawing is deferred to
        the derived class
        :param painter: A painter that understands how to draw
        """
        for drawable in self._drawables:
            drawable.draw(painter, self)

    def remove(self, painter):
        """Remove all drawn objects for this peak from view
        :param painter: A Painter object to interact with the draw destination
        """
        for drawable in self._drawables:
            drawable.remove(painter)

    def repaint(self, painter):
        """Repaint all drawn objects for this peak on the view
        :param painter: A Painter object to interact with the draw destination
        """
        # 4 "transitions" possible:
        #  - visible->still visible but different alpha
        #  - visible->invisible
        #  - invisible->invisible
        #  - invisible->visible
        if self.alpha > 0.0:
            for drawable in self._drawables:
                drawable.repaint(painter, self)
        else:
            # peak becomes invisible
            self.remove(painter)

    def snap_to(self, painter):
        """
        Tell the painter to move to the peak center
        :param painter: A painter that understands how to draw
        """
        painter.snap_to(self.x, self.y, self.snap_width())

    def snap_width(self):
        """
        Compute the width on the view required to encompass the drawn shapes
        """
        return 0.5

    def update_alpha(self, slicepoint, slicedim_width):
        """Update transparency value based on slicepoint. Note that this does not call repaint
        :param painter: A Painter object to interact with the draw destination
        :param slicepoint: float giving current slice point
        :param slicedim_width: 2-tuple giving (min/max) values of the slicing dimension
        """
        self._alpha_previous = self._alpha
        self._alpha = compute_alpha(self.z, slicepoint, slicedim_width)


class IntegratedPeakRepresentation(PeakRepresentation):
    """Represents an integrated Peak region"""

    def __init__(self, x, y, z, alpha, fg_color, bg_color, drawables, snap_width):
        """
        See PeakRepresentation.__init__ for other parameters
        :param bg_color: Color of the optional background region
        :param snap_width: Width of view to encompass all of the shapes
        """
        super().__init__(x, y, z, alpha, fg_color, drawables)
        self._bg_color = bg_color
        self._snap_width = snap_width

    @property
    def bg_color(self):
        return self._bg_color

    def snap_width(self):
        """Return the width of the view when a peak is zoomed to"""
        return self._snap_width
