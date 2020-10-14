# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

#  std imports
import json
from unittest.mock import MagicMock

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.test.modeltesthelpers \
    import create_slice_info
import numpy as np


def draw_representation(cls,
                        peak_origin,
                        shape_info,
                        painter,
                        fg_color,
                        bg_color,
                        slice_transform=None):
    """
    Calls draw on a given representation type
    :param cls: The representation type expected to have a classmethod called draw
    :param peak_origin: 3-element sequence specifying the origin
    :param shape_info: dict containing a relevant shape description
    :param painter: Reference to the painter object
    :param fg_color: str containing the foreground color
    :param bg_color: str containing the background color
    :param slice_transform: Optional function to assign as slice transformation. Defaults to identity
    """

    def identity(x):
        return x

    peak_shape = MagicMock()
    peak_shape.toJSON.return_value = json.dumps(shape_info)
    if slice_transform is None:
        slice_transform = identity
    slice_info = create_slice_info(slice_transform, slice_value=3., slice_width=10.)

    return cls.draw(peak_origin, peak_shape, slice_info, painter, fg_color, bg_color)


def create_ellipsoid_info(radii, axes, bkgd_radii):
    """
    Create a dict describing an ellipsoid.
    :param radii: 3-tuple containing radii for each axis
    :param axes: 3-tuple containing vector defining each principal axis
    :param bkgd_radii: 2-tuple of (bkgd_inner, bkgd_outer) radii containing radii for each axis
    """
    return {
        "radius0": radii[0],
        "radius1": radii[1],
        "radius2": radii[2],
        "direction0": axes[0],
        "direction1": axes[1],
        "direction2": axes[2],
        "background_outer_radius0": bkgd_radii[1][0],
        "background_outer_radius1": bkgd_radii[1][1],
        "background_outer_radius2": bkgd_radii[1][2],
        "background_inner_radius0": bkgd_radii[0][0],
        "background_inner_radius1": bkgd_radii[0][1],
        "background_inner_radius2": bkgd_radii[0][2]
    }


def create_sphere_info(radius, bkgd_radii=None):
    """
    Create description of a sphere
    :param radius: The radius of the sphere
    :param bkgd_radii: An optional 2-tuple of (inner,outer) radii
    """
    shape_descr = {
        "radius": radius,
    }
    if bkgd_radii is not None:
        shape_descr.update({
            "background_inner_radius": bkgd_radii[0],
            "background_outer_radius": bkgd_radii[1]
        })

    return shape_descr


class FuzzyMatch:
    """Matcher class to fuzzy compare a value with an expected value"""

    def __init__(self, expected, atol):
        """
        :param expected: The expected value
        :param atol: Absolute tolerance (see numpy.allclose reference)
        """
        self._atol = atol
        self._expected = expected

    def __eq__(self, value):
        return np.allclose(self._expected, value, atol=self._atol)

    def __repr__(self):
        return f"FuzzyMatch(expected={self._expected}, atol={self._atol})"
