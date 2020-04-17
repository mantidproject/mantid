# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
# std imports
from json import loads as json_loads

# 3rdparty imports
import numpy as np

# local imports
from .alpha import compute_alpha
from .ellipsoid import slice_ellipsoid_matrix
from .painter import Painted


class SphericallyIntergratedPeakRepresentation(object):
    """Provide methods to display a representation of a slice through an
    Spherically intgerated region around a Peak"""

    @classmethod
    def draw(cls, peak_origin, peak_shape, slice_info, painter, fg_color, bg_color):
        """
        Draw the representation of a slice through an sphere
        :param peak_origin: Peak origin in original workspace frame
        :param peak_shape: A reference to the object describing the PeakShape
        :param slice_info: A SliceInfo object detailing the current slice
        :param painter: A reference to a object capable of drawing shapes
        :param fg_color: A str representing the color of the peak shape marker
        :param bg_color: A str representing the color of the background region.
        :return: A new instance of this class
        """
        shape_info = json_loads(peak_shape.toJSON())
        signal_radius = _signal_radius_info(shape_info)
        peak_origin = slice_info.transform(peak_origin)

        slice_origin, signal_radius = slice_sphere(peak_origin, signal_radius, slice_info.value)
        if not np.any(np.isfinite((signal_radius, ))):
            # slice not possible
            return None
        alpha = compute_alpha(slice_origin[2], slice_info.value, slice_info.width)
        if alpha < 0.0:
            return None

        center_x, center_y = slice_origin[:2]
        # yapf: disable
        artists = [
            painter.cross(
                center_x,
                center_y,
                0.1 * signal_radius,
                alpha=alpha,
                color=fg_color
            ),
            painter.circle(
                center_x,
                center_y,
                signal_radius,
                alpha=alpha,
                linestyle="--",
                facecolor="none",
                edgecolor=fg_color
            ),
        ]
        # yapf: enable

        # add background if we have one
        bkgd_radius_thick = _bkgd_radius_info(shape_info)
        if bkgd_radius_thick is not None:
            bkgd_outer_radius, thickness = bkgd_radius_thick
            _, bkgd_circle_radius = slice_sphere(peak_origin, bkgd_outer_radius, slice_info.value)
            # yapf: disable
            artists.append(
                painter.shell(
                    center_x,
                    center_y,
                    bkgd_circle_radius,
                    thickness,
                    alpha=alpha,
                    linestyle="--",
                    facecolor=bg_color,
                    edgecolor="none"
                )
            )
            # yapf: enable

        return Painted(painter, artists)


def _signal_radius_info(shape_info):
    """
    Parse shape information and return information on the radius of the signal region of the Sphere
    :param shape_info: A reference to a a dictionary of parameters from a PeakShape object
    :return: The radius of the Sphere
    """
    return float(shape_info["radius"])


def _bkgd_radius_info(shape_info):
    """
    Parse shape information and return information on the background radii of the Sphere
    :param shape_info: A reference to a a dictionary of parameters from a PeakShape object
    :return: 2-tuple of (outer_radius, thickness) or None if no background region is defined
    """
    bg_inner_radius = shape_info.get("background_inner_radius", None)
    bg_outer_radius = shape_info.get("background_outer_radius", None)
    radius_thickness = None
    if bg_inner_radius and bg_outer_radius:
        radius_thickness = (bg_outer_radius, bg_outer_radius - bg_inner_radius)

    return radius_thickness


def slice_sphere(peak_origin, radius, zp):
    """Slice a Sphere at a point z=zp and return the radius of the resulting circle
    :param peak_origin: Origin of Sphere
    :param radius: Radius of the Sphere
    :param zp: Slice point in slice dimension
    """
    # Sphere is just a special case of an ellipse
    slice_origin, circle_radius, _, __ = slice_ellipsoid_matrix(peak_origin, zp,
                                                                calculate_spherical_matrix(radius))
    return slice_origin, circle_radius


def calculate_spherical_matrix(radius):
    """
    Compute matrix defining Sphere:
    https://en.wikipedia.org/wiki/Ellipsoid#As_quadric
    :param radius: Radius of Sphere
    """
    # Create matrix whose eigenvalues are squares of semi-axes lengths
    # and eigen vectors define principle axis vectors
    inv_rad_sq = 1 / radius**2
    axes_lengths = np.diag((inv_rad_sq, ) * 3)
    return axes_lengths
