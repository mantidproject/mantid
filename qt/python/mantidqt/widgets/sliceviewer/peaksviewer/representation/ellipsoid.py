# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
# std imports
from json import loads as json_loads
import numpy as np
import numpy.linalg as linalg

# local imports
from .alpha import compute_alpha
from .painter import Painted


class EllipsoidalIntergratedPeakRepresentation(object):
    """Provide methods to display a representation of a slice through an
    Ellipsoidally intgerated region around a Peak"""

    @classmethod
    def draw(cls, peak_origin, peak_shape, slice_info, painter, fg_color, bg_color):
        """
        Draw the representation of a slice through an ellipsoid
        :param peak_origin: Peak origin in original workspace frame
        :param peak_shape: A reference to the object describing the PeakShape
        :param slice_info: A SliceInfo object detailing the current slice
        :param painter: A reference to a object capable of drawing shapes
        :param fg_color: A str representing the color of the peak shape marker
        :param bg_color: A str representing the color of the background region.
        :return: A new instance of this class
        """
        shape_info = json_loads(peak_shape.toJSON())
        # use signal ellipse to see if it is valid at this slice
        axes, signal_radii = _signal_ellipsoid_info(shape_info, slice_info.transform)
        peak_origin = slice_info.transform(peak_origin)

        slice_origin, major_radius, minor_radius, angle = slice_ellipsoid(
            peak_origin, *axes, *signal_radii, slice_info.value)
        if not np.any(np.isfinite((major_radius, minor_radius))):
            # slice not possible
            return None
        alpha = compute_alpha(slice_origin[2], slice_info.value, slice_info.width)
        if alpha < 0.0:
            return None

        signal_width, signal_height = 2 * major_radius, 2 * minor_radius
        # add background shell
        a, b, c, shell_thick = _bkgd_ellipsoid_info(shape_info, slice_info.transform)
        _, major_radius, minor_radius, angle = slice_ellipsoid(peak_origin, *axes, a, b, c,
                                                               slice_info.value)
        bkgd_width, bkgd_height = 2 * major_radius, 2 * minor_radius

        # yapf: disable
        artists = (
            painter.cross(
                slice_origin[0],
                slice_origin[1],
                0.1 * signal_width,
                alpha=alpha,
                color=fg_color,
            ),
            painter.ellipse(
                slice_origin[0],
                slice_origin[1],
                signal_width,
                signal_height,
                angle,
                alpha=alpha,
                linestyle="--",
                edgecolor=fg_color,
                facecolor="none"
            ),
            painter.elliptical_shell(
                slice_origin[0],
                slice_origin[1],
                bkgd_width,
                bkgd_height,
                shell_thick,
                angle,
                alpha=alpha,
                linestyle="--",
                edgecolor="none",
                facecolor=bg_color,
            )
        )
        # yapf: enable
        return Painted(painter, artists)


def _signal_ellipsoid_info(shape_info, transform):
    """
    Retrieve axes and radii from the PeakShape in the slice frame
    :param shape_info: A dictionary of ellipsoid properties
    :param transform: Transform function to move to the slice frame
    """

    def to_ndarray(axis_field):
        s = shape_info[axis_field]
        return np.array([float(x) for x in s.split()], dtype=float)

    axis_a, axis_b, axis_c = to_ndarray("direction0"), to_ndarray("direction1"), to_ndarray(
        "direction2")
    a, b, c = float(shape_info["radius0"]), float(shape_info["radius1"]), float(
        shape_info["radius2"])
    return transform((axis_a, axis_b, axis_c)), transform((a, b, c))


def _bkgd_ellipsoid_info(shape_info, transform):
    """
    Retrieve background radii and width from the PeakShape in the slice frame
    :param shape_info: A dictionary of ellipsoid properties
    :param transform: Callable to move to the slice frame
    """
    a, b, c = float(shape_info["background_outer_radius0"]), float(
        shape_info["background_outer_radius1"]), float(shape_info["background_outer_radius2"])
    inner_a, inner_b, inner_c = float(shape_info["background_inner_radius0"]), float(
        shape_info["background_inner_radius1"]), float(shape_info["background_inner_radius2"])
    a, b, c = transform((a, b, c))
    inner_a, inner_b, inner_c = transform((inner_a, inner_b, inner_c))
    width = max((a, b, c)) - max((inner_a, inner_b, inner_c))

    return (a, b, c, width)


def slice_ellipsoid(origin, axis_a, axis_b, axis_c, a, b, c, zp):
    """Return semi-axes lengths and angle of ellipse formed
    by cutting the ellipsoid with a plane at z=zp
    :param origin: Origin of ellipsoid
    :param axis_a: Axis of length a
    :param axis_b: Axis of length b
    :param axis_c: Axis of length c
    :param a: Axis 0 radius
    :param b: Axis 1 radius
    :param c: Axis 2 radius
    :param zp: Slice point in slice dimension
    :return (slice_origin, major_radius, minor_radius, angle):
    """
    # From  https://en.wikipedia.org/wiki/Ellipsoid#As_quadric:
    #
    #   Transpose[x-x0]*M*(x-x0) = 1
    #
    # defines arbitrary ellipsoid
    # We transform: x = x-x0 , y = y-y0, zk= zp -z0 (ie we look at a center-origin
    # system)
    #
    #  This results in the expression:
    #
    #  m00x^2 + 2*m01*x*y + 2*m02*zk*x + m11*y^2 + 2*m12*zk*y + m22*zk = 1
    #
    #  or in matrix form
    #
    #
    #  Transpose[Q]*A*Q  + Transpose[B]*Q + c = 1
    #
    #  with:
    #  Q = (x)
    #      (y)
    #
    #  A = (m00  m01)
    #      (m01  m11)
    #
    #  B = 2*zk *(m02)
    #            (m12)
    #
    #  c = m22*zk^2
    return slice_ellipsoid_matrix(origin, zp,
                                  calculate_ellipsoid_matrix(axis_a, axis_b, axis_c, a, b, c))


def calculate_ellipsoid_matrix(axis_a, axis_b, axis_c, a, b, c):
    """
    Compute matrix A defining ellipsoid
    https://en.wikipedia.org/wiki/Ellipsoid#As_quadric
    :param axis_a: Axis of length a
    :param axis_b: Axis of length b
    :param axis_c: Axis of length c
    :param a: Axis 0 radius
    :param b: Axis 1 radius
    :param c: Axis 2 radius
    """
    # Create matrix whose eigenvalues are squares of semi-axes lengths
    # and eigen vectors define principle axis vectors
    axes_lengths = np.diag((1 / a**2, 1 / b**2, 1 / c**2))
    # yapf: disable
    axes_dir = np.array((
        (axis_a[0], axis_b[0], axis_c[0]),
        (axis_a[1], axis_b[1], axis_c[1]),
        (axis_a[2], axis_b[2], axis_c[2])
    ))
    # yapf: enable
    return axes_dir @ axes_lengths @ np.transpose(axes_dir)


def slice_ellipsoid_matrix(origin, zp, ellipMatrix):
    """
    :param origin: Origin of the ellipsoid
    :param zp: Slice point in the slicing dimension
    :param M: Ellipsoid Matrix. See `calculate_ellipsoid_matrix` for definition
    """
    # slice point in frame with ellipsoid at origin
    z = zp - origin[2]

    A = ellipMatrix[:2, :2]
    A[1, 0] = ellipMatrix[0, 1]
    B = 2 * z * ellipMatrix[:2, 2]
    c = ellipMatrix[2][2] * z**2

    #  Using quadratic completion we can get rid of ( or rather refactor) the linear
    #  term:
    #  (Note we make use that A = Transpose[A]
    #
    #  Transpose[(Q-K)]*A*(Q-K) + Transpose[B + 2AK]*Q - Transpose[K]*A*Transpose[K]
    # + c = 1
    #
    #  K is set to -A^(-1)*B/2
    #
    #  which leads to the standard form for the 2D ellipse:
    #
    #  Transpose[Q + A^(-1)*B/2] * A/(Transpose[B]*A^(-1)*B/4 - (c-1)) *(Q +
    # A^(-1)*B/2) = 1
    #
    #
    #  The ellipse equation
    #  MM = A/(Transpose[B]*A^(-1)*B/4 - (c-1))
    #  The radii are the eigenvalues and the axes directions are the eigenvectors
    #
    #  The ellipse origin is
    #  -A^(-1)*B/2
    MM = A / (0.25 * np.transpose(B) @ linalg.inv(A) @ B - (c - 1))
    eigvalues, eigvectors = linalg.eig(MM)
    minor_radius, major_radius = 1 / np.sqrt(eigvalues[0]), 1 / np.sqrt(eigvalues[1])
    major_axis = eigvectors[:, 1]
    angle = np.rad2deg(np.arctan2(major_axis[1], major_axis[0]))
    slice_origin_xy = -0.5 * linalg.inv(A) @ B
    slice_origin = np.array((slice_origin_xy[0] + origin[0], slice_origin_xy[1] + origin[1],
                             z + origin[2]))

    return slice_origin, major_radius, minor_radius, angle
