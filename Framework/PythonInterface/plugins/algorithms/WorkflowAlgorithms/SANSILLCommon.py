# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import logger
from enum import Enum
from math import fabs


class AcqMode(Enum):
    MONO = 1 # standard monochromatic SANS
    KINETIC = 2 # kinetic monochromatic SANS
    TOF = 3 # TOF SANS (D33 only)


def check_distances_match(ws1, ws2):
    """
        Checks if the detector distance between two workspaces are close enough
        @param ws1 : workspace 1
        @param ws2 : workspace 2
    """
    tolerance = 0.01 #m
    l2_1 = ws1.getRun().getLogData('L2').value
    l2_2 = ws2.getRun().getLogData('L2').value
    r1 = ws1.getRunNumber()
    r2 = ws2.getRunNumber()
    if fabs(l2_1 - l2_2) > tolerance:
        logger.warning('Distance difference out of tolerance {0}: {1}, {2}: {3}'.format(r1, l2_1, r2, l2_2))


def check_wavelengths_match(ws1, ws2):
    """
        Checks if the wavelength difference between the data is close enough
        @param ws1 : workspace 1
        @param ws2 : workspace 2
    """
    tolerance = 0.01 # AA
    wavelength_1 = ws1.getRun().getLogData('wavelength').value
    wavelength_2 = ws2.getRun().getLogData('wavelength').value
    r1 = ws1.getRunNumber()
    r2 = ws2.getRunNumber()
    if fabs(wavelength_1 - wavelength_2) > tolerance:
        logger.warning('Wavelength difference out of tolerance {0}: {1}, {2}: {3}'.format(r1, wavelength_1, r2, wavelength_2))


def check_processed_flag(ws, exp_value):
    """
        Returns true if the workspace is processed as expected, false otherwise
        @param ws : workspace
        @param exp_value : the expected value of the ProcessedAs log
    """
    return ws.getRun().getLogData('ProcessedAs').value == exp_value


def cylinder_xml(radius):
    """
        Returns XML for an infinite cylinder with axis of z (incident beam) and given radius [m]
        @param radius : the radius of the cylinder [m]
        @return : XML string for the geometry shape
    """
    return f'<infinite-cylinder id="flux"><centre x="0.0" y="0.0" z="0.0"/><axis x="0.0" y="0.0" z="1.0"/>' \
           '<radius val="{radius}"/></infinite-cylinder>'


def monitor_id(instrument):
    """
        Returns the pair of the real monitor ID and the other, blank monitor ID for the given instrument
        TODO: These could rather be defined in the IPFs
    """
    if instrument == 'D33':
        return [500000, 500001]
    elif instrument == 'D16':
        return [500001, 500000]
    else:
        return [100000, 100001]
