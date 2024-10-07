# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# local imports
from .alpha import compute_alpha
from .painter import MplPainter, Painted

# 3rd party
from mantidqt.widgets.sliceviewer.models.sliceinfo import SliceInfo

# standard library
from typing import Sequence


class NonIntegratedPeakRepresentation:
    """Create a collection of PeakDrawable objects for a non-integrated Peak"""

    VIEW_FRACTION = 0.015

    @classmethod
    def draw(
        cls,
        peak_origin: Sequence,
        peak_shape,
        slice_info: SliceInfo,
        painter: MplPainter,
        fg_color: str,
        _,  # unused PeakShape
    ) -> Painted:  # unused bg_color
        """
        Draw the representation of a slice through a peak with no shape
        :param peak_origin: Peak origin in original workspace frame
        :param peak_shape: A reference to the object describing the PeakShape
        :param slice_info: A SliceInfo object detailing the current slice
        :param painter: A reference to a object capable of drawing shapes
        :param fg_color: A str representing the color of the peak shape marker
        :param _: A str representing the color of the background region. Unused
        :return: A new instance of this class
        """
        peak_origin = slice_info.transform(peak_origin)
        x, y, z = peak_origin
        alpha = compute_alpha(z, slice_info.z_value, slice_info.z_width)
        painted = None
        if alpha > 0.0:
            # Non-integrated peaks have no size in Q space so we create an
            # effective size. It is scaled according to the current view limits
            # so not to dominate the view
            effective_radius = slice_info.z_width * cls.VIEW_FRACTION
            # yapf: disable
            effective_bbox = ((x - effective_radius, y - effective_radius),
                              (x + effective_radius, y + effective_radius))
            # yapf: enable
            view_xlim = painter.axes.get_xlim()
            deltax = view_xlim[1] - view_xlim[0]
            # more succinctly: view_radius = cls.VIEW_FRACTION * min(slice_info.z_width, deltax)
            view_radius = effective_radius
            if effective_radius > cls.VIEW_FRACTION * deltax:
                view_radius = cls.VIEW_FRACTION * deltax

            painted = Painted(painter, (painter.cross(x, y, view_radius, alpha=alpha, color=fg_color),), effective_bbox)

        return painted
