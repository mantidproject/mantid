# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,

#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Helper functions for DNS simulation calculations
"""

import numpy as np
from numpy import cos, pi, radians, sin


def get_hkl_float_array(hklstring):
    return np.asarray([float(x) for x in hklstring.split(',')])


def angle_to_q(ttheta, omega, wavelength):  # should work with np arrays
    tt = radians(ttheta)
    w = radians(omega)
    two_pi_over_lambda = 2.0 * pi / wavelength
    qx = (cos(-w) - cos(-w + tt)) * two_pi_over_lambda
    qy = (sin(-w) - sin(-w + tt)) * two_pi_over_lambda
    return qx, qy


def filter_flattend_meshs(x, y, z, limits):
    if x is None:
        return [y, y, z]
    minx, maxx, miny, maxy = limits
    x = x.flatten()
    y = y.flatten()
    z = z.flatten()
    filtered = np.logical_and(np.logical_and(minx < x, x < maxx),
                              np.logical_and(miny < y, y < maxy))
    x = x[filtered]
    y = y[filtered]
    z = z[filtered]
    return [x, y, z]


def get_z_min_max(z, xlim=None, ylim=None, plotx=None, ploty=None):
    fz = z.flatten()
    if xlim is not None and ylim is not None:
        fploty = ploty.flatten()
        fplotx = plotx.flatten()
        fz = fz[np.logical_and(
            np.logical_and(xlim[1] > fplotx, xlim[0] < fplotx),
            np.logical_and(ylim[1] > fploty, ylim[0] < fploty))]
    if fz.size != 0:
        zmax = fz.max()
        zmin = fz.min()
        pzmin = min(i for i in fz if i > 0)
    else:
        zmax = 0
        zmin = 0
        pzmin = 0
    return zmin, zmax, pzmin


def get_hkl_intensity_from_cursor(single_crystal_map, axis_type, x, y):
    if axis_type['switch']:  # switch axes
        x, y = y, x
    hkl1 = single_crystal_map.hkl1.split(',')
    hkl2 = single_crystal_map.hkl2.split(',')
    dx = single_crystal_map.dx
    dy = single_crystal_map.dy
    if axis_type['type'] == 'two_theta_and_omega':  # two_theta omega
        qx, qy = angle_to_q(ttheta=x, omega=y, wavelength=single_crystal_map.wavelength)
        hklx = hkl_to_hklx(hkl1, qx, dx)
        hkly = hkl_to_hklx(hkl2, qy, dy)
        pos_q = closest_mesh_point(single_crystal_map.two_theta_mesh, single_crystal_map.omega_mesh, x, y)
    elif axis_type['type'] == 'qxqy':  # qx qy
        qx, qy = x, y
        hklx = hkl_to_hklx(hkl1, qx, dx)
        hkly = hkl_to_hklx(hkl2, qy, dy)
        pos_q = closest_mesh_point(single_crystal_map.qx_mesh, single_crystal_map.qy_mesh, qx, qy)
    elif axis_type['type'] == 'hkl':  # hkl
        qx = hklxy_to_q(x, dx)
        qy = hklxy_to_q(y, dy)
        hklx = hkl_to_hklx(hkl1, x=x)
        hkly = hkl_to_hklx(hkl2, x=y)
        pos_q = closest_mesh_point(single_crystal_map.qx_mesh, single_crystal_map.qy_mesh, qx, qy)
    z = single_crystal_map.z_mesh.flatten()[pos_q]
    error = single_crystal_map.error_mesh.flatten()[pos_q]
    hkl = hkl_xy_to_hkl(hklx, hkly)
    return [hkl[0], hkl[1], hkl[2], z, error]


# functions used by get_hkl_intensity_from_cursor()


def closest_mesh_point(x_mesh, y_mesh, x, y):
    cp = (np.add(np.square(x_mesh - x), np.square(y_mesh - y)).argmin())
    return cp


def hkl_to_hklx(hkl, q=None, d=None, x=None):
    if d is None:
        return [x * float(a) for a in hkl]
    return [q_to_hklxy(q, d) * float(a) for a in hkl]


def hkl_xy_to_hkl(hklx, hkly):
    return [hklx[0] + hkly[0], hklx[1] + hkly[1], hklx[2] + hkly[2]]


def hklxy_to_q(hkl, d):
    return hkl / d * 2.0 * np.pi


def stringrange_to_float(stringrange):
    if stringrange.count(':') != 1:
        return [None, None]
    try:
        mmin, mmax = [float(x) for x in stringrange.split(':')]
    except ValueError:
        return [None, None]
    return [mmin, mmax]


def q_to_hklxy(q, d):
    return q * d / 2.0 / np.pi


def get_projection(x, z):
    numpoints = min(int(np.sqrt(len(x))) + 1, len(np.unique(x)) + 1)
    projection, _dummy = np.histogram(x, numpoints, weights=z)
    counts, px = np.histogram(x, numpoints)
    px = (px[:-1] + px[1:]) / 2.
    py = projection / counts
    return px, py
