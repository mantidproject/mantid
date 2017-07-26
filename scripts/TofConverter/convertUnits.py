#pylint: disable=invalid-name,too-many-branches
from __future__ import (absolute_import, division, print_function)
import math
# Used by converter GUI to do unit conversions


def doConversion(inputval, inOption, outOption, theta, flightpath):
    stage1output = input2energy(float(inputval), inOption, theta, flightpath)
    stage2output = energy2output(stage1output, outOption, theta, flightpath)
    return stage2output

# Convert input value to intermediate energy


def input2energy(inputval, inOption, theta, flightpath):
    e2lam = 81.787 #using lambda=h/p  p: momentum, h: planck's const, lambda: wavelength
    e2nu = 4.139 # using h/(m*lambda^2) m: mass of neutron
    e2v = 0.0000052276 # using v = h/(m*lambda)
    e2k = 2.0717 #using k = 2*pi/(lambda) k:momentum
    e2t = 0.086165 #using t = (m*v^2)/(2kb) kb: Boltzmann const
    e2cm = 0.123975 #cm = 8.06554465*E E:energy
    iv2 = inputval ** 2

    if inOption == 'Wavelength (Angstroms)':
        Energy = e2lam / iv2

    elif inOption == 'Energy (meV)':
        Energy = inputval

    elif inOption == 'Nu (THz)':
        Energy = e2nu*inputval

    elif inOption == 'Velocity (m/s)':
        Energy = e2v*iv2

    elif inOption == 'Momentum (k Angstroms^-1)':
        Energy = e2k*iv2

    elif inOption == 'Temperature (K)':
        Energy = e2t*inputval

    elif inOption == 'Energy (cm^-1)':
        Energy = e2cm*inputval

    elif inOption == 'Momentum transfer (Q Angstroms^-1)':
        if theta >= 0.0:
            k = inputval*0.5/math.sin(theta)
            Energy = e2k*k*k
        else:
            raise RuntimeError("Theta > 0 is required for conversion from Q")

    elif inOption == 'd-spacing (Angstroms)':
        if theta >= 0.0:
            lam = 2 * inputval*math.sin(theta)
            Energy = e2lam / (lam * lam)
        else:
            raise RuntimeError("Theta > 0 is required for conversion from Q")

    elif  inOption == 'Time of flight (microseconds)':
        if flightpath >= 0.0:
            Energy = 1000000*flightpath
            Energy = e2v*Energy*Energy / iv2
        else:
            raise RuntimeError("Flight path >= 0 is required for conversion from TOF")

    return Energy


# Convert intermediate energy to output type
def energy2output(Energy, outOption, theta, flightpath):
    e2lam = 81.787 #using lambda=h/p  p: momentum, h: planck's const, lambda: wavelength
    e2nu = 4.139 # using h/(m*lambda^2) m: mass of neutron
    e2v = 0.0000052276 # using v = h/(m*lambda)
    e2k = 2.0717 #using k = 2*pi/(lambda) k:momentum
    e2t = 0.086165 #using t = (m*v^2)/(2kb) kb: Boltzmann const
    e2cm = 0.123975 #cm = 8.06554465*E E:energy

    if outOption == 'Wavelength (Angstroms)':
        OutputVal =  (e2lam/ Energy)**0.5

    elif outOption == 'Nu (Thz)':
        OutputVal = Energy / e2nu

    elif outOption == 'Velocity (m/s)':
        OutputVal = (Energy / e2v)**0.5

    elif outOption == 'Momentum (k Angstroms^-1)':
        OutputVal = (Energy / e2k)**0.5

    elif outOption == 'Temperature (K)':
        OutputVal = Energy / e2t

    elif outOption == 'Energy (cm^-1)':
        OutputVal = Energy / e2cm

    elif outOption == 'Momentum transfer (Q Angstroms^-1)':
        if theta >= 0.0:
            k = (Energy / e2k) ** 0.5
            OutputVal = 2 * k * math.sin(theta)
        else:
            raise RuntimeError("Theta > 0 is required for conversion to Q")

    elif outOption == 'd-spacing (Angstroms)':
        if theta >= 0.0:
            lam = (e2lam / Energy)**0.5
            OutputVal = lam * 0.5 / math.sin(theta)
        else:
            raise RuntimeError("Theta > 0 is required for conversion to d-Spacing")

    elif outOption == 'Time of flight (microseconds)':
        if flightpath >= 0.0:
            OutputVal = flightpath * 1000 * ((e2v * 1000000 / Energy) ** 0.5)
        else:
            raise RuntimeError("Flight path >= 0 is required for conversion to TOF")

    elif outOption == 'Energy (meV)':
        OutputVal = Energy

    return OutputVal
