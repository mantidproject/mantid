# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import absolute_import, division, unicode_literals

# std imports
from json import loads as json_loads

# 3rdparty imports
from matplotlib.transforms import Affine2D, IdentityTransform
from matplotlib.patches import Patch, Ellipse
from matplotlib.path import Path
import numpy as np

# local imports
from .base import PEAK_ALPHA, PeakRepresentation, value_or_error


class EllipticalRing(Patch):
    """
    Elliptical ring-shaped patch.
    """

    def __str__(self):
        pars = (self.center[0], self.center[1], self.width, self.height, self.ring_thick,
                self.angle)
        fmt = "EllipticalRing(center=(%g, %g), width=%g, height=%g, ring_width=%g, angle=%s)"
        return fmt % pars

    def __init__(self, center, width, height, ring_thick, angle=0.0, **kwargs):
        """
        Draw an elliptical ring centered at *x*, *y* center with outer width (horizontal diameter)
        *width* and outer height (vertical diameter) *height* with a ring thickness of *ring_thick*

        Valid kwargs are:

        %(Patch)s
        """
        super(EllipticalRing, self).__init__(**kwargs)
        self.center = center
        self.height, self.width = height, width
        self.ring_thick = ring_thick
        self.angle = angle
        self._recompute_path()
        # Note: This cannot be calculated until this is added to an Axes
        self._patch_transform = IdentityTransform()

    def _recompute_path(self):
        # Form the outer ring
        arc = Path.arc(theta1=0.0, theta2=360.0)
        # Draw the outer unit circle followed by a reversed and scaled inner circle
        v1 = arc.vertices
        v2 = arc.vertices[::-1] * float(1.0 - self.ring_thick)
        v = np.vstack([v1, v2, v1[0, :], (0, 0)])
        c = np.hstack([arc.codes, arc.codes, Path.MOVETO, Path.CLOSEPOLY])
        c[len(arc.codes)] = Path.MOVETO
        # Final shape acheieved through axis transformation. See _recompute_transform
        self._path = Path(v, c)

    def _recompute_transform(self):
        """NOTE: This cannot be called until after this has been added
                 to an Axes, otherwise unit conversion will fail. This
                 makes it very important to call the accessor method and
                 not directly access the transformation member variable.
        """
        center = (self.convert_xunits(self.center[0]), self.convert_yunits(self.center[1]))
        width = self.convert_xunits(self.width)
        height = self.convert_yunits(self.height)
        self._patch_transform = Affine2D() \
            .scale(width * 0.5, height * 0.5) \
            .rotate_deg(self.angle) \
            .translate(*center)

    def get_patch_transform(self):
        self._recompute_transform()
        return self._patch_transform

    def get_path(self):
        if self._path is None:
            self._recompute_path()
        return self._path


class PeakRepresentationEllipsoid(PeakRepresentation):
    """Describes a 2D view of a Peak that has been integrated with an Ellipsoid shape"""

    def __init__(self,
                 width,
                 height,
                 angle,
                 center,
                 alpha,
                 marker_color,
                 bkgd_widths=(0.0, 0.0),
                 bkgd_heights=(0.0, 0.0)):
        """
        :param width: Length of horizontal axis
        :param height: Length of vertical axis
        :param center: A V3D defining the center
        :param alpha: A float between 0.0, 1.0 defining the transparency of the line
        :param marker_color: A str code defining the color of the peak marker
        :param bkgd_widths: 2-tuple of (inner, outer) width of background ellipses (optional)
        :param bkgd_heights: 2-tuple of (inner, outer) heights of background ellipses (optional)
        """
        super(PeakRepresentationEllipsoid, self).__init__(center, alpha, marker_color)
        self._angle = angle
        self._height = height
        self._width = width
        self._bkgd_widths, self._bkgd_heights = bkgd_widths, bkgd_heights

    @property
    def width(self):
        return self._width

    @property
    def height(self):
        return self._height

    @property
    def background_widths(self):
        return self._bkgd_widths

    @property
    def background_heights(self):
        return self._bkgd_heights

    @classmethod
    def create(cls, center, peak_shape, marker_color):
        """
        Create an instance of PeakRepresentationEllipsoid from the given peak object.
        :param center: Center of ellipsoid
        :param peak_shape: Reference to a PeakShapeEllipsoid object
        :param marker_color: String describing color
        :return: A new instance of this class
        """
        properties = json_loads(peak_shape.toJSON())
        return PeakRepresentationEllipsoid(
            center=center,
            width=2 * value_or_error(properties, "radius0"),
            height=2 * value_or_error(properties, "radius1"),
            angle=90.0,
            alpha=PEAK_ALPHA,
            marker_color=marker_color,
            bkgd_widths=(2 * properties.get("background_inner_radius0", 0.0),
                         2 * properties.get("background_outer_radius0", 0.0)),
            bkgd_heights=(2 * properties.get("background_inner_radius1", 0.0),
                          2 * properties.get("background_outer_radius1", 0.0)))

    def draw(self, ax):
        """
        Override base class method to draw spherical integration region with a background
        indicated by a dotted circle
        :param ax: An Axes object to accept the draw command
        :return: The new Artist/Artists added
        """
        center = (self.center.X(), self.center.Y())
        bkgd_widths, bkgd_heights = self._bkgd_widths, self._bkgd_heights

        # signal region
        ax.add_patch(
            Ellipse(
                center,
                width=self._width,
                height=self._height,
                angle=self._angle,
                linestyle='--',
                facecolor='none',
                edgecolor=self.marker_color))
        # optional background
        if bkgd_heights[0] > 0.0:
            ax.add_patch(
                EllipticalRing(
                    center,
                    width=bkgd_widths[1],
                    height=bkgd_heights[1],
                    ring_thick=bkgd_widths[1] - bkgd_widths[0],
                    angle=self._angle,
                    facecolor=self.marker_color,
                    edgecolor='none'))
