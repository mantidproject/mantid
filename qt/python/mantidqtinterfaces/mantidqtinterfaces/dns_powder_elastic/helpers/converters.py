# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS unit converters.
"""

from numpy import deg2rad, pi, sin


def elastic_two_theta_to_d(two_theta, wavelength):
    return wavelength / (2.0 * sin(deg2rad(two_theta / 2.0)))


def elastic_two_theta_to_q(two_theta, wavelength):
    return pi * 4.0 * sin(deg2rad(two_theta / 2.0)) / wavelength


def convert_hkl_string_to_float(hkl):
    return [float(x) for x in hkl.split(",")]
