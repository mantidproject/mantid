# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS unit converters.
"""

from numpy import cos, pi, radians


def lambda_to_energy(wavelength):
    m_neutron = 1.674927471 * 10**-27  # in kg
    h_planck = 6.626070040 * 10**-34  # in J*s
    energy = h_planck**2 / 2.0 / m_neutron / wavelength**2 * 10**20 \
             / 1.6021766208 / 10**-22
    return energy


def two_theta_to_q(two_theta, wavelength, delta_e):
    # delta_e in meV
    # wavelength in Angstrom
    # two_theta is not 2*theta here!
    delta_e = delta_e * 1.6021766208 * 10**-22  # converts meV to Joule
    h_bar = 6.626070040 * 10**-34 / pi / 2.0  # in J*s
    m_neutron = 1.674927471 * 10**-27  # in kg
    two_theta = radians(two_theta)
    k_i = 2.0 * pi / wavelength  # incoming wave vector
    # outgoing wave vector k_i for elastic
    # factor 10**-20 is for converting 1/m^2 to 1/Angstrom^2
    k_f = (k_i**2 - delta_e*2.0*m_neutron/h_bar**2*10**-20)**0.5
    q_abs = (k_i**2 + k_f**2 - 2.0*k_i*k_f*cos(two_theta))**0.5  # length of Q in inelastic case
    return q_abs
