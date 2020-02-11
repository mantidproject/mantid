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
from .ellipsoid import PeakRepresentationEllipsoid
from .spherical import PeakRepresentationSphere

# map shape names to representation classes
# the strings need to match whatever Peak.getPeakShape.shapeName returns
_PEAK_REPRESENTATIONS = {
    "none": PeakRepresentationNoShape,
    "spherical": PeakRepresentationSphere,
    "ellipsoid": PeakRepresentationEllipsoid
}


def create_peakrepresentation(peak, slicepoint, slicerange, marker_color):
    """
    A factory function to create an appropriate PeakRepresentation
    object for a peak.
    :param peak: A Peak object
    :param slicepoint: float giving current slice point
    :param slicerange: 2-tuple giving (min/max) values of the slicing dimension
    :param marker_color: A str representing the color of the peak shape marker
    :returns: A PeakRepresentation object describing the Peak aspects
              important for display
    """
    peak_shape = peak.getPeakShape()
    cls = _PEAK_REPRESENTATIONS[peak_shape.shapeName().lower()]
    return cls.create(peak.getQLabFrame(), slicepoint, slicerange, peak_shape, marker_color)
