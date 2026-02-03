# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Helper functions for DNS single crystal elastic calculations.
"""

import numpy as np
from numpy import asarray, cos, pi, radians, sin


def get_hkl_float_array(hkl_string):
    return asarray([float(x) for x in hkl_string.split(",")])


def angle_to_q(two_theta, omega, wavelength):  # should work with np arrays
    tt = radians(two_theta)
    w = radians(omega)
    two_pi_over_lambda = 2.0 * pi / wavelength
    qx = (cos(-w) - cos(-w + tt)) * two_pi_over_lambda
    qy = (sin(-w) - sin(-w + tt)) * two_pi_over_lambda
    return qx, qy


def filter_flattened_meshes(x, y, z, limits):
    if x is None:
        return [y, y, z]
    min_x, max_x, min_y, max_y = limits
    x = x.flatten()
    y = y.flatten()
    z = z.flatten()
    filtered = np.logical_and(np.logical_and(min_x < x, x < max_x), np.logical_and(min_y < y, y < max_y))
    x = x[filtered]
    y = y[filtered]
    z = z[filtered]
    return [x, y, z]


def get_z_min_max(z, xlim=None, ylim=None, plot_x=None, plot_y=None):
    flatten_z = z.flatten()
    if xlim is not None and ylim is not None:
        flatten_plot_y = plot_y.flatten()
        flatten_plot_x = plot_x.flatten()
        flatten_z = flatten_z[
            np.logical_and(
                np.logical_and(xlim[1] > flatten_plot_x, xlim[0] < flatten_plot_x),
                np.logical_and(ylim[1] > flatten_plot_y, ylim[0] < flatten_plot_y),
            )
        ]
    if flatten_z.size != 0:
        z_max = flatten_z.max()
        z_min = flatten_z.min()
        pz_min = min((i for i in flatten_z if i > 0), default=0)
    else:
        z_max = 0
        z_min = 0
        pz_min = 0
    return z_min, z_max, pz_min


def get_hkl_intensity_from_cursor(single_crystal_map, plot_settings, x, y):
    tau1 = single_crystal_map.hkl1.split(",")
    tau2 = single_crystal_map.hkl2.split(",")
    dx = single_crystal_map.dx
    dy = single_crystal_map.dy
    interpolation_on = bool(plot_settings["interpolate"])
    plot_type = plot_settings["plot_type"]

    if plot_settings["type"] == "angular":  # two_theta omega
        qx, qy = angle_to_q(two_theta=x, omega=y, wavelength=single_crystal_map.wavelength)
        hklx = tau_to_hkl_proj_vect(tau1, qx, dx)
        hkly = tau_to_hkl_proj_vect(tau2, qy, dy)
    elif plot_settings["type"] == "qxqy":  # qx qy
        qx, qy = x, y
        hklx = tau_to_hkl_proj_vect(tau1, qx, dx)
        hkly = tau_to_hkl_proj_vect(tau2, qy, dy)
    elif plot_settings["type"] == "hkl":  # hkl
        hklx = tau_to_hkl_proj_vect(tau1, x=x)
        hkly = tau_to_hkl_proj_vect(tau2, x=y)
    hkl = hkl_xy_to_hkl(hklx, hkly)

    if plot_type == "triangulation":
        z_per_triangle = single_crystal_map.z_face
        trifinder = single_crystal_map.triangulation.get_trifinder()
        pos_q = trifinder(x, y)
        z = z_per_triangle.flatten()[pos_q]
    else:  # quadmesh or scatterplot
        # in case of triangulation, axis switch is already included when mesh is created
        if plot_settings["switch"]:
            x, y = y, x
        if plot_settings["type"] == "angular":  # two_theta omega
            pos_q = closest_mesh_point(single_crystal_map.angular_mesh[0], single_crystal_map.angular_mesh[1], x, y)
            z = single_crystal_map.angular_mesh[2].flatten()[pos_q]
        elif plot_settings["type"] == "qxqy":  # qx qy
            pos_q = closest_mesh_point(single_crystal_map.qxqy_mesh[0], single_crystal_map.qxqy_mesh[1], x, y)
            z = single_crystal_map.qxqy_mesh[2].flatten()[pos_q]
        elif plot_settings["type"] == "hkl":  # hkl
            pos_q = closest_mesh_point(single_crystal_map.hkl_mesh[0], single_crystal_map.hkl_mesh[1], x, y)
            z = single_crystal_map.hkl_mesh[2].flatten()[pos_q]

    if interpolation_on or plot_type == "triangulation":
        # Errors are undefined on interpolated points. In addition, when triangulation is on
        # then the original points are not used and the errors cannot be assigned.
        error = None
    else:
        error = single_crystal_map.error_mesh.flatten()[pos_q]

    return [hkl[0], hkl[1], hkl[2], z, error]


# functions used by get_hkl_intensity_from_cursor()
def closest_mesh_point(x_mesh, y_mesh, x, y):
    closest_point = np.add(np.square(x_mesh - x), np.square(y_mesh - y)).argmin()
    return closest_point


def tau_to_hkl_proj_vect(tau, q=None, d=None, x=None):
    """
    Calculates a projection vector value hklx/hkly for the provided
    basis vector tau_1/tau_2 and corresponding projection x/y.
    When x/y is not explicitly provided, it is calculated from the
    given set of (qx, dx)/(qy, dy).
    """
    if d is None:
        return [x * float(a) for a in tau]
    return [q_xy_to_xy(q, d) * float(a) for a in tau]


def hkl_xy_to_hkl(hklx, hkly):
    """
    Obtains hkl from knowledge of its projection vector values
    hklx and hkly along specified basis vectors tau_1 and tau_2.
    hklx = x * tau_1, hkly = y * tau_2, hkl = hklx + hkly.
    hklx, hkly, hkl, tau_1, and tau_2 - represent vectors.
    """
    return [hklx[0] + hkly[0], hklx[1] + hkly[1], hklx[2] + hkly[2]]


def q_xy_to_xy(q, d):
    """
    Calculates x/y projection of hkl on a reciprocal basis
    vector tau_1/tau_2 using (qx, dx)/(qy, dy) as an input.
    """
    return q * d / 2.0 / np.pi


def get_projection(x, z):
    # use numpy's implementation of the optimal bin width calculation
    bin_edges = np.histogram_bin_edges(x, bins="auto")
    numpoints = bin_edges.size - 1
    # use weights to sum up intensity values corresponding to the same bin
    i_projection, x_bin_edges = np.histogram(x, numpoints, weights=z)
    x_bin_centers = (x_bin_edges[:-1] + x_bin_edges[1:]) / 2.0
    return x_bin_centers, i_projection
