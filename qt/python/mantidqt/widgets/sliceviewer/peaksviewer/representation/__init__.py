# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, unicode_literals)

# local imports
from .noshape import PeakRepresentationNoShape

# map shape names to representation classes
# the strings need to match whatever Peak.getPeakShape.shapeName returns
_PEAK_REPRESENTATIONS = {
    "none": PeakRepresentationNoShape,
    "spherical": PeakRepresentationNoShape,
    "ellipsoid": PeakRepresentationNoShape
}


def create_peakrepresentation(x, y, z, slicepoint, slicedim_width, peak_shape, marker_color):
    """
    A factory function to create an appropriate PeakRepresentation
    object for a peak.
    :param x: X position of peak in slice plane
    :param x: Y position of peak in slice plane
    :param z: Z position of peak out of slice plane
    :param slicepoint: float giving current slice point
    :param slicedim_width: Total width of the slicing dimension
    :param peak_shape: A reference to the object describing the PeakShape
    :param marker_color: A str representing the color of the peak shape marker
    :returns: A PeakRepresentation object describing the Peak aspects
              important for display
    """
    cls = _PEAK_REPRESENTATIONS[peak_shape.shapeName().lower()]
    return cls.create(x, y, z, slicepoint, slicedim_width, peak_shape, marker_color)
