# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS unit convertes
"""

from numpy import cos, pi, radians


def lambda_to_energy(wavelength):
    mneutron = 1.674927471 * 10**-27  # in kg
    hquer = 6.626070040 * 10**-34  # in J*s
    energy = (hquer**2 / 2 / mneutron / wavelength**2 * 10**20 / 1.6021766208 /
              10**-22)
    return energy


def twotheta_to_q(twotheta, wavelength, delta_e):
    # deltaE in meV
    # wavelength in Angstroem
    # twotheta is not 2*theta here!
    delta_e = delta_e * 1.6021766208 * 10**-22  # converts meV to Joule
    hquer = 6.626070040 * 10**-34 / pi / 2  # in J*s
    mneutron = 1.674927471 * 10**-27  # in kg
    twotheta = radians(twotheta)
    ki = pi * 2 / wavelength  # incoming wavevector
    # outgoing wavevector # = ki for elastic
    # factor 10**-20 is for converting 1/m^2 to 1/Angstroem^2
    kf = (ki**2 - delta_e * 2 * mneutron / hquer**2 * 10**-20)**0.5
    qabs = 2 * pi * (ki**2 + kf**2 - 2 * ki * kf * cos(twotheta)
                     )**0.5 / 2.0 / pi  # length of Q in inelastic case
    return qabs
