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

# local imports
from .base import PEAK_ALPHA, value_or_error
from .ellipsoid import PeakRepresentationEllipsoid


class PeakRepresentationSphere(PeakRepresentationEllipsoid):
    """Describes a view of a Peak that has been integrated with a Spherical shape.
    Uses PeakRepresentationEllipsoid to draw.
    """

    def __init__(self, radius, center, alpha, marker_color, bkgd_radii=(0.0, 0.0)):
        """
        See PeakRepresentationEllipsoid for description of other parameters
        :param radius: Radius of circle
        """
        # ellipsoid uses diameters
        width = 2 * radius
        bkgd_radii = tuple(2 * bkgd_radii[i] for i in range(2))
        super(PeakRepresentationSphere, self).__init__(
            width=width,
            height=width,
            angle=0.0,
            center=center,
            alpha=alpha,
            marker_color=marker_color,
            bkgd_widths=bkgd_radii,
            bkgd_heights=bkgd_radii)

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
        return PeakRepresentationSphere(
            radius=value_or_error(properties, "radius"),
            center=center,
            alpha=PEAK_ALPHA,
            marker_color=marker_color,
            bkgd_radii=(properties.get("background_inner_radius", 0.0),
                        properties.get("background_outer_radius", 0.0)))

    @property
    def radius(self):
        return 0.5 * self._width

    @property
    def background_radii(self):
        return tuple(0.5 * self._bkgd_widths[i] for i in range(2))
