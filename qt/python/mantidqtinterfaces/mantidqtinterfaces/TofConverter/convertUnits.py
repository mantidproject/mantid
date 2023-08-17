# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-branches
import math

ENERGY_MEV = "Energy (meV)"
WAVELENGTH = "Wavelength (Angstroms)"
NU = "Nu (THz)"
VELOCITY = "Velocity (m/s)"
MOMENTUM = "Momentum (k Angstroms^-1)"
TEMPERATURE = "Temperature (K)"
ENERGY_CMINV = "Energy (cm^-1)"
MOMENTUM_TRANSFER = "Momentum transfer (Q Angstroms^-1)"
D_SPACING = "d-spacing (Angstroms)"
TIME_OF_FLIGHT = "Time of flight (microseconds)"
UNIT_LIST = [ENERGY_MEV, WAVELENGTH, NU, VELOCITY, MOMENTUM, TEMPERATURE, ENERGY_CMINV, MOMENTUM_TRANSFER, D_SPACING, TIME_OF_FLIGHT]

# Used by converter GUI to do unit conversions


def doConversion(inputval, inOption, outOption, theta, flightpath):
    if is_momentum_dspacing_transform(inOption, outOption):
        return convert_momentum_transfer_to_dspacing_or_back(float(inputval))
    stage1output = input2energy(float(inputval), inOption, theta, flightpath)
    stage2output = energy2output(stage1output, outOption, theta, flightpath)
    return stage2output


# Convert input value to intermediate energy


def input2energy(inputval, inOption, theta, flightpath):
    e2lam = 81.787  # using lambda=h/p  p: momentum, h: planck's const, lambda: wavelength
    e2nu = 4.139  # using h/(m*lambda^2) m: mass of neutron
    e2v = 0.0000052276  # using v = h/(m*lambda)
    e2k = 2.0717  # using k = 2*pi/(lambda) k:momentum
    e2t = 0.086165  # using t = (m*v^2)/(2kb) kb: Boltzmann const
    e2cm = 0.123975  # cm = 8.06554465*E E:energy
    iv2 = inputval**2

    if inOption == WAVELENGTH:
        Energy = e2lam / iv2

    elif inOption == ENERGY_MEV:
        Energy = inputval

    elif inOption == NU:
        Energy = e2nu * inputval

    elif inOption == VELOCITY:
        Energy = e2v * iv2

    elif inOption == MOMENTUM:
        Energy = e2k * iv2

    elif inOption == TEMPERATURE:
        Energy = e2t * inputval

    elif inOption == ENERGY_CMINV:
        Energy = e2cm * inputval

    elif inOption == MOMENTUM_TRANSFER:
        if theta >= 0.0:
            k = inputval * 0.5 / math.sin(theta)
            Energy = e2k * k * k
        else:
            raise RuntimeError("Theta > 0 is required for conversion from Q")

    elif inOption == D_SPACING:
        if theta >= 0.0:
            lam = 2 * inputval * math.sin(theta)
            Energy = e2lam / (lam * lam)
        else:
            raise RuntimeError("Theta > 0 is required for conversion from Q")

    elif inOption == TIME_OF_FLIGHT:
        if flightpath >= 0.0:
            Energy = 1000000 * flightpath
            Energy = e2v * Energy * Energy / iv2
        else:
            raise RuntimeError("Flight path >= 0 is required for conversion from TOF")

    return Energy


# Convert intermediate energy to output type
def energy2output(Energy, outOption, theta, flightpath):
    e2lam = 81.787  # using lambda=h/p  p: momentum, h: planck's const, lambda: wavelength
    e2nu = 4.139  # using h/(m*lambda^2) m: mass of neutron
    e2v = 0.0000052276  # using v = h/(m*lambda)
    e2k = 2.0717  # using k = 2*pi/(lambda) k:momentum
    e2t = 0.086165  # using t = (m*v^2)/(2kb) kb: Boltzmann const
    e2cm = 0.123975  # cm = 8.06554465*E E:energy

    if outOption == WAVELENGTH:
        OutputVal = (e2lam / Energy) ** 0.5

    elif outOption == NU:
        OutputVal = Energy / e2nu

    elif outOption == VELOCITY:
        OutputVal = (Energy / e2v) ** 0.5

    elif outOption == MOMENTUM:
        OutputVal = (Energy / e2k) ** 0.5

    elif outOption == TEMPERATURE:
        OutputVal = Energy / e2t

    elif outOption == ENERGY_CMINV:
        OutputVal = Energy / e2cm

    elif outOption == MOMENTUM_TRANSFER:
        if theta >= 0.0:
            k = (Energy / e2k) ** 0.5
            OutputVal = 2 * k * math.sin(theta)
        else:
            raise RuntimeError("Theta > 0 is required for conversion to Q")

    elif outOption == D_SPACING:
        if theta >= 0.0:
            lam = (e2lam / Energy) ** 0.5
            OutputVal = lam * 0.5 / math.sin(theta)
        else:
            raise RuntimeError("Theta > 0 is required for conversion to d-Spacing")

    elif outOption == TIME_OF_FLIGHT:
        if flightpath >= 0.0:
            OutputVal = flightpath * 1000 * ((e2v * 1000000 / Energy) ** 0.5)
        else:
            raise RuntimeError("Flight path >= 0 is required for conversion to TOF")

    elif outOption == ENERGY_MEV:
        OutputVal = Energy

    return OutputVal


def convert_momentum_transfer_to_dspacing_or_back(q_or_d: float) -> float:
    return 2 * math.pi / q_or_d


def is_momentum_dspacing_transform(inOption: str, outOption: str) -> bool:
    return (inOption == MOMENTUM_TRANSFER and outOption == D_SPACING) or (inOption == D_SPACING and outOption == MOMENTUM_TRANSFER)
