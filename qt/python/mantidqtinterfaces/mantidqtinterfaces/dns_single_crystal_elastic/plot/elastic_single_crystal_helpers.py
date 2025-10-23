# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Helper functions for DNS single crystal elastic calculations.
"""

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
