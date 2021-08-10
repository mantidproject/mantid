# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CropToComponent, logger, mtd
from enum import Enum
from math import fabs


class AcqMode(Enum):
    '''
    |        |  MONO  |  KINETIC  |  TOF  |  REVENT  |
    | X Unit |  Empty |   Empty   | Lambda|    TOF   |
    | X Size |    1   |    >1     |  >1   |    >1    |
    | IsHist?|    n   |     n     |   y   |     y    |
    '''
    MONO = 1 # standard monochromatic SANS
    KINETIC = 2 # kinetic monochromatic SANS
    TOF = 3 # TOF SANS [D33 only] (equidistant (LTOF) or non-equidistant (VTOF))
    REVENT = 4 # rebinned monochromatic event data


# Empty run rumber to act as a placeholder where a sample measurement is missing.
EMPTY_TOKEN = '000000'
# Quite often some samples are not measured at the some distances.
# Still, it is preferred to reduce them together in one table.
# Below is an example of why the blanks are needed:
#    | samples D1 | samples D2 | samples D3 | sample TR |
#    |     R1     |     R2     |     R3     |    R4     |
#    |     R10    |     0      |     R11    |    R12    |
# Here the sample 2 is not measured at distance 2.
# But in order to reduce it together in a vectorized way and apply the transmissions
# we will need to inject a blank measurement in the place of 0, with the same number of frames
# as the sample measurement (so 1 if mono, >1 if kinetic mono).


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
        logger.warning(f'Distance difference out of tolerance {r1}: {l2_1}, {r2}: {l2_2}')


def get_wavelength(ws):
    run = ws.getRun()
    if 'wavelength' in run:
        return run['wavelength'].value
    elif 'selector.wavelength' in run:
        return run['selector.wavelength']
    else:
        raise RuntimeError('Unable to find the wavelength in workspace ' + ws.name())


def check_wavelengths_match(ws1, ws2):
    """
        Checks if the wavelength difference between the data is close enough
        @param ws1 : workspace 1
        @param ws2 : workspace 2
    """
    tolerance = 0.01 # AA
    wavelength_1 = get_wavelength(ws1)
    wavelength_2 = get_wavelength(ws2)
    r1 = ws1.getRunNumber()
    r2 = ws2.getRunNumber()
    if fabs(wavelength_1 - wavelength_2) > tolerance:
        logger.warning(f'Wavelength difference out of tolerance {r1}: {wavelength_1}, {r2}: {wavelength_2}')


def check_processed_flag(ws, exp_value):
    """
        Returns true if the workspace is processed as expected, false otherwise
        @param ws : workspace
        @param exp_value : the expected value of the ProcessedAs log
    """
    if ws.getRun().getLogData('ProcessedAs').value != exp_value:
        logger.warning(f'{exp_value} workspace is not processed as such.')


def cylinder_xml(radius):
    """
        Returns XML for an infinite cylinder with axis of z (incident beam) and given radius [m]
        @param radius : the radius of the cylinder [m]
        @return : XML string for the geometry shape
    """
    return '<infinite-cylinder id="flux"><centre x="0.0" y="0.0" z="0.0"/><axis x="0.0" y="0.0" z="1.0"/>' \
           '<radius val="{0}"/></infinite-cylinder>'.format(radius)


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


def blank_monitor_ws_neg_index(instrument):
    '''Returns the negative index of the spectra corresponding to the empty monitor that will host acq times'''
    if instrument != 'D16':
        return -1
    else:
        return -2


def get_vertical_grouping_pattern(ws):
    """
    Provides vertical grouping pattern and crops to the main detector panel where counts from the beam are measured.
    Used for fitting the horizontal incident beam profile for q resolution calculation.
    TODO: These are static and can be turned to grouping files in instrument/Grouping folder
    :param ws: Empty beam workspace.
    """
    inst_name = mtd[ws].getInstrument().getName()
    min_id = 0
    if 'D11' in inst_name:
        if 'lr' in inst_name:
            step = 128
            max_id = 16384
        elif 'B' in inst_name:
            CropToComponent(InputWorkspace=ws, OutputWorkspace=ws, ComponentNames='detector_center')
            max_id = 49152
            step = 192
        else:
            step = 256
            max_id = 65536
    elif 'D22' in inst_name:
        max_id = 32768
        step = 256
        if 'lr' in inst_name:
            step = 128
            max_id = 16384
        elif 'B' in inst_name:
            CropToComponent(InputWorkspace=ws, OutputWorkspace=ws, ComponentNames='detector_back')
    elif 'D33' in inst_name:
        CropToComponent(InputWorkspace=ws, OutputWorkspace=ws, ComponentNames='back_detector')
        max_id = 32768
        step = 128
    else:
        logger.warning('Instruments other than D11, D22, and D33 are not yet supported for direct beam width fitting.')
        return
    return ','.join(["{}-{}".format(start, start + step - 1) for start in range(min_id, max_id, step)])
