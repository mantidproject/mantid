# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

# local imports
from .noshape import NonIntegratedPeakRepresentation
from .ellipsoid import EllipsoidalIntegratedPeakRepresentation
from .painter import MplPainter, Painted

# 3rd party
from mantid.geometry import PeakShape
from mantidqt.widgets.sliceviewer.models.sliceinfo import SliceInfo

# standard library
from typing import Sequence

# map shape names to representation classes
# the strings need to match whatever Peak.getPeakShape.shapeName returns
_PEAK_REPRESENTATION_FACTORY = {
    "none": NonIntegratedPeakRepresentation,
    "spherical": EllipsoidalIntegratedPeakRepresentation,
    "ellipsoid": EllipsoidalIntegratedPeakRepresentation,
    "detectorbin": NonIntegratedPeakRepresentation,
}


def _get_factory(peak_shape: PeakShape):
    """
    :param peak_shape: A PeakShape object
    :return: A factory object with a create method able to
             create a representation for the given shape type
    """
    shape_name = peak_shape.shapeName()
    try:
        return _PEAK_REPRESENTATION_FACTORY[shape_name.lower()]
    except KeyError:
        from mantid.kernel import logger

        logger.warning(f"An {shape_name} shape is not yet supported. Only the peak center will be shown.")
        return NonIntegratedPeakRepresentation


def draw_peak_representation(
    peak_origin: Sequence, peak_shape: PeakShape, slice_info: SliceInfo, painter: MplPainter, fg_color: str, bg_color: str
) -> Painted:
    """
    A factory function to create an appropriate PeakRepresentation
    object for a peak and draw it.
    :param peak_origin: Peak origin in original workspace frame
    :param peak_shape: A reference to the object describing the PeakShape
    :param slice_info: A SliceInfo object detailing the current slice
    :param painter: A reference to a object capable of drawing shapes
    :param fg_color: A str representing the color of the peak shape marker
    :param bg_color: A str representing the color of the background region if applicable
    :returns: A collection of drawn objects or None if nothing is visible
    """
    return _get_factory(peak_shape).draw(peak_origin, peak_shape, slice_info, painter, fg_color, bg_color)
