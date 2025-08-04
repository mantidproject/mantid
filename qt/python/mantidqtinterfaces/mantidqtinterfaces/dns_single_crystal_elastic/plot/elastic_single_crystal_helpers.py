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
        pz_min = min(i for i in flatten_z if i > 0)
    else:
        z_max = 0
        z_min = 0
        pz_min = 0
    return z_min, z_max, pz_min


def get_hkl_intensity_from_cursor(single_crystal_map, axis_type, x, y):
    if axis_type["switch"]:  # switch axes
        x, y = y, x
    hkl1 = single_crystal_map.hkl1.split(",")
    hkl2 = single_crystal_map.hkl2.split(",")
    dx = single_crystal_map.dx
    dy = single_crystal_map.dy
    if axis_type["type"] == "angular":  # two_theta omega
        qx, qy = angle_to_q(two_theta=x, omega=y, wavelength=single_crystal_map.wavelength)
        hklx = hkl_to_hklx(hkl1, qx, dx)
        hkly = hkl_to_hklx(hkl2, qy, dy)
        pos_q = closest_mesh_point(single_crystal_map.two_theta_mesh, single_crystal_map.omega_mesh, x, y)
    elif axis_type["type"] == "qxqy":  # qx qy
        qx, qy = x, y
        hklx = hkl_to_hklx(hkl1, qx, dx)
        hkly = hkl_to_hklx(hkl2, qy, dy)
        pos_q = closest_mesh_point(single_crystal_map.qx_mesh, single_crystal_map.qy_mesh, qx, qy)
    elif axis_type["type"] == "hkl":  # hkl
        qx = hkl_xy_to_q(x, dx)
        qy = hkl_xy_to_q(y, dy)
        hklx = hkl_to_hklx(hkl1, x=x)
        hkly = hkl_to_hklx(hkl2, x=y)
        pos_q = closest_mesh_point(single_crystal_map.qx_mesh, single_crystal_map.qy_mesh, qx, qy)
    z = single_crystal_map.z_mesh.flatten()[pos_q]
    error = single_crystal_map.error_mesh.flatten()[pos_q]
    hkl = hkl_xy_to_hkl(hklx, hkly)
    return [hkl[0], hkl[1], hkl[2], z, error]


# functions used by get_hkl_intensity_from_cursor()
def closest_mesh_point(x_mesh, y_mesh, x, y):
    closest_point = np.add(np.square(x_mesh - x), np.square(y_mesh - y)).argmin()
    return closest_point


def hkl_to_hklx(hkl, q=None, d=None, x=None):
    if d is None:
        return [x * float(a) for a in hkl]
    return [q_to_hkl_xy(q, d) * float(a) for a in hkl]


def hkl_xy_to_hkl(hklx, hkly):
    return [hklx[0] + hkly[0], hklx[1] + hkly[1], hklx[2] + hkly[2]]


def hkl_xy_to_q(hkl, d):
    return hkl / d * 2.0 * np.pi


def string_range_to_float(string_range):
    if string_range.count(":") != 1:
        return [None, None]
    try:
        m_min, m_max = [float(x) for x in string_range.split(":")]
    except ValueError:
        return [None, None]
    return [m_min, m_max]


def q_to_hkl_xy(q, d):
    return q * d / 2.0 / np.pi


def get_projection(x, z):
    numpoints = min(int(np.sqrt(len(x))) + 1, len(np.unique(x)) + 1)
    projection, _dummy = np.histogram(x, numpoints, weights=z)
    counts, px = np.histogram(x, numpoints)
    px = (px[:-1] + px[1:]) / 2.0
    py = projection / counts
    return px, py
