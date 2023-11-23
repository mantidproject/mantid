# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS unit converters.
"""

from numpy import cos, radians, sin, sqrt


def d_spacing_from_lattice(a, b, c, alpha, beta, gamma, hkl):
    # pylint: disable=too-many-arguments
    # alpha beta gamma in deg
    h, k, l = hkl
    alpha = radians(alpha)
    beta = radians(beta)
    gamma = radians(gamma)
    cos_alpha = cos(alpha)
    cos_beta = cos(beta)
    cos_gamma = cos(gamma)
    sin_alpha = sin(alpha)
    sin_beta = sin(beta)
    sin_gamma = sin(gamma)
    vol = a * b * c * sqrt(1.0 - cos_alpha**2 - cos_beta**2 - cos_gamma**2 + 2.0 * cos_alpha * cos_beta * cos_gamma)
    s11 = b**2 * c**2 * sin_alpha**2
    s22 = a**2 * c**2 * sin_beta**2
    s33 = a**2 * b**2 * sin_gamma**2
    s12 = a * b * c**2 * (cos_alpha * cos_beta - cos_gamma)
    s23 = a**2 * b * c * (cos_beta * cos_gamma - cos_alpha)
    s13 = a * b**2 * c * (cos_gamma * cos_alpha - cos_beta)
    inverse_d_square = 1.0 / vol**2 * (s11 * h**2 + s22 * k**2 + s33 * l**2 + 2.0 * s12 * h * k + 2 * s23 * k * l + 2 * s13 * h * l)
    d = sqrt(1 / inverse_d_square)
    return d
