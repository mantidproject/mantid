# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS unit convertes
"""

from numpy import cos, radians, sin, sqrt


def d_spacing_from_lattice(a, b, c, alpha, beta, gamma, hkl):
    # pylint: disable=too-many-arguments
    # alpha beta gamma in deg
    h, k, l = hkl  # noqa: E741
    alpha = radians(alpha)
    beta = radians(beta)
    gamma = radians(gamma)
    calpha = cos(alpha)
    cbeta = cos(beta)
    cgamma = cos(gamma)
    salpha = sin(alpha)
    sbeta = sin(beta)
    sgamma = sin(gamma)
    vol = a * b * c * sqrt(1 - calpha ** 2
                           - cbeta ** 2
                           - cgamma ** 2
                           + 2 * calpha * cbeta * cgamma)
    s11 = b ** 2 * c ** 2 * salpha ** 2
    s22 = a ** 2 * c ** 2 * sbeta ** 2
    s33 = a ** 2 * b ** 2 * sgamma ** 2
    s12 = a * b * c ** 2 * (calpha * cbeta - cgamma)
    s23 = a ** 2 * b * c * (cbeta * cgamma - calpha)
    s13 = a * b ** 2 * c * (cgamma * calpha - cbeta)
    invd2 = 1 / vol ** 2 * (s11 * h ** 2
                            + s22 * k ** 2
                            + s33 * l ** 2
                            + 2 * s12 * h * k
                            + 2 * s23 * k * l
                            + 2 * s13 * h * l)
    d = sqrt(1 / invd2)
    return d
