# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# local imports
from .base import compute_alpha
from .noshape import NonIntegratedPeakRepresentation
from .spherical import SphericallyIntergratedPeakRepresentation

# map shape names to representation classes
# the strings need to match whatever Peak.getPeakShape.shapeName returns
PEAK_REPRESENTATION_FACTORY = {
    "none": NonIntegratedPeakRepresentation,
    "spherical": SphericallyIntergratedPeakRepresentation,
    "ellipsoid": NonIntegratedPeakRepresentation
}


def create_peakrepresentation(x, y, z, slicepoint, slicedim_width, peak_shape, marker_color,
                              bg_color):
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
    :param bg_color: A str representing the color of the background region if applicable
    :returns: A PeakRepresentation object describing the Peak aspects
              important for display
    """
    factory = PEAK_REPRESENTATION_FACTORY[peak_shape.shapeName().lower()]
    return factory.create(x, y, z, compute_alpha(z, slicepoint, slicedim_width), peak_shape,
                          marker_color, bg_color)
