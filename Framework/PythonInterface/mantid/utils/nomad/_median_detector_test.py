# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# 3rd-party imports
import numpy
import numpy as np
import yaml
from mantid.simpleapi import (LoadEmptyInstrument, mtd, MaskDetectors, ExtractMask, SaveMask, DeleteWorkspace)
# standard imports
import enum
from collections import namedtuple


__all__ = ['determine_tubes_threshold']


def determine_tubes_threshold(vec_intensity, mask_config: dict, instrument_config):
    """

    Refer to diagram
    https://code.ornl.gov/sns-hfir-scse/diffraction/powder/powder-diffraction/
    uploads/99f4b65655e05edd476999942ff6fb98/Step-1.png
    from box 'Select a tube' to 'All tubes finished'


    Parameters
    ----------
    vec_intensity: numpy.ndarray
        solid angle corrected counts. shape = (101378, )
    mask_config: ~dict
        configuration for masking
    instrument_config: ~namedtuple
        instrument configuration

    Returns
    -------

    """
    # Workflow: process full-collimated, half-collimated and not-collimated separately
    pixel_collimation_states = \
        _NOMADMedianDetectorTest.get_collimation_states(mask_config['collimation'],
                                                        instrument_config, InstrumentComponentLevel.Pixel)

    # reshape the input intensities to 2D matrix: each row is a tube, i.e., (num_tubes, num_pixel_per_tube)
    # or say (392, 256)
    tube_pixel_intensity_array = vec_intensity.flatten().reshape((instrument_config.num_tubes,
                                                                  instrument_config.num_pixels_per_tube))

    # Calculate tube medians: m, m1, m2
    half_tube_pixels = instrument_config.num_pixels_per_tube // 2
    # m
    tube_m_array = np.median(tube_pixel_intensity_array, axis=1)
    # m1
    tube_m1_array = np.median(tube_pixel_intensity_array[:, 0:half_tube_pixels], axis=1)
    # m2
    tube_m2_array = np.median(tube_pixel_intensity_array[:, half_tube_pixels:], axis=1)

    # Boundary constants
    low_pixel = mask_config['threshold']['low_pixel']
    high_pixel = mask_config['threshold']['high_pixel']

    # Calculate boundary for not collimated pixels
    # condition 2:  summed pixel intensity < low_pixel * m or summed pixel intensity > high_pixel * m,
    not_collimated_lowers = low_pixel * tube_m_array
    not_collimated_uppers = high_pixel * tube_m_array
    not_collimated_lowers = np.repeat(not_collimated_lowers, instrument_config.num_pixels_per_tube)
    not_collimated_uppers = np.repeat(not_collimated_uppers, instrument_config.num_pixels_per_tube)
    not_collimated_masks = (pixel_collimation_states == int(CollimationLevel.Empty)).astype(int)
    # sanity check
    assert not_collimated_lowers.shape == (instrument_config.num_pixels, )
    assert not_collimated_masks.shape == not_collimated_uppers.shape

    # Calculate boundary for full collimated pixels
    # condition 3:
    full_collimated_lowers = (1. + (low_pixel - 1) * 3.) * tube_m_array
    full_collimated_uppers = (1. + (high_pixel - 1) * 3.) * tube_m_array
    full_collimated_lowers = np.repeat(full_collimated_lowers, instrument_config.num_pixels_per_tube)
    full_collimated_uppers = np.repeat(full_collimated_uppers, instrument_config.num_pixels_per_tube)
    full_collimated_masks = (pixel_collimation_states == int(CollimationLevel.Full)).astype(int)

    # Calculate boundaries for half collimated pixels
    # condition 4: summed pixel intensity < (1+(low_pixel-1)*3) * m1 or
    # summed pixel intensity > (1+(high_pixel-1)*3) * m1,
    # condition 5:  summed pixel intensity < (1+(low_pixel-1)*3) * m2 or
    # summed pixel intensity > (1+(high_pixel-1)*3) * m2,
    half1_collimated_lowers = (1. + (low_pixel - 1) * 3.) * tube_m1_array
    half2_collimated_lowers = (1. + (low_pixel - 1) * 3.) * tube_m2_array
    half_collimated_lowers = np.repeat(half1_collimated_lowers, 2)
    half_collimated_lowers[1::2] = half2_collimated_lowers
    half_collimated_lowers = np.repeat(half_collimated_lowers, instrument_config.num_pixels_per_tube // 2)
    assert half_collimated_lowers.shape == (instrument_config.num_pixels, )

    half_collimated_uppers = (1. + (high_pixel - 1) * 3.) * tube_m1_array
    half2_collimated_uppers = (1. + (high_pixel - 1) * 3.) * tube_m2_array
    half_collimated_uppers = np.repeat(half_collimated_uppers, 2)
    half_collimated_uppers[1::2] = half2_collimated_uppers
    half_collimated_uppers = np.repeat(half_collimated_uppers, instrument_config.num_pixels_per_tube // 2)
    assert half_collimated_uppers.shape == (instrument_config.num_pixels, )
    half_collimated_masks = (pixel_collimation_states == int(CollimationLevel.Half)).astype(int)

    # Combine
    upper_boundaries = not_collimated_uppers * not_collimated_masks
    upper_boundaries += full_collimated_uppers * full_collimated_masks
    upper_boundaries += half_collimated_uppers * half_collimated_masks

    lower_boundaries = not_collimated_lowers * not_collimated_masks
    lower_boundaries += full_collimated_lowers * full_collimated_masks
    lower_boundaries += half_collimated_lowers * half_collimated_masks
    #

    # Return the to be masked pixels
    pixel_mask_states = vec_intensity < lower_boundaries
    print(pixel_mask_states)
    pixel_mask_states += vec_intensity > upper_boundaries

    return pixel_mask_states


def export_masks(pixel_mask_states: numpy.ndarray,
                 mask_file_name: str,
                 instrument_name: str = 'NOMAD'):
    """Export masks to XML file format

    Parameters
    ----------
    pixel_mask_states: numpy.ndarray
        boolean array with the number of pixels of NOMAD.  True for masking
    mask_file_name: str
        name of the output mask XML file
    instrument_name: str
        name of the instrument

    """
    # Load empty instrument
    empty_workspace_name = f'_{instrument_name}_empty'
    mask_workspace_name = f'_{instrument_name}_mask_empty'
    LoadEmptyInstrument(InstrumentName=instrument_name, OutputWorkspace=empty_workspace_name)

    # Get the workspace indexes to mask
    # first 2 spectra are monitors: add 2
    # Check
    if mtd[empty_workspace_name].getNumberHistograms() != pixel_mask_states.shape[0] + 2:
        raise RuntimeError(f'Spectra number of {instrument_name} workspace does not match mask state array')
    mask_ws_indexes = np.where(pixel_mask_states)[0] + 2
    print(f'DEBUG mask workspace indexes: {mask_ws_indexes}')

    # Mask detectors
    MaskDetectors(Workspace=empty_workspace_name, WorkspaceIndexList=mask_ws_indexes)

    # Extract and save
    ExtractMask(InputWorkspace=empty_workspace_name, OutputWorkspace=mask_workspace_name)
    SaveMask(InputWorkspace=mask_workspace_name, OutputFile=mask_file_name)

    # Clean
    for ws_name in [empty_workspace_name, mask_workspace_name]:
        DeleteWorkspace(ws_name)


class CollimationLevel(enum.IntEnum):
    r"""Collimation state of an eight-pack"""
    Empty = 0
    Half = 1
    Full = 2


class InstrumentComponentLevel(enum.IntEnum):
    """
    Instrument component level
    """
    Bank = 0
    EightPack = 1
    Tube = 2
    Pixel = 3


class _NOMADMedianDetectorTest:
    r"""Mixin providing methods to algorithm NOMADMedianDetectorTest"""

    @property
    def tube_medians(self) -> np.ndarray:
        r"""Median intensity of a whole tube
        :returns: 1D array of size number of tubes in the instrument
        """
        intensities_by_tube = self.intensities.reshape((self.tube_count, self.tube_length))  # shape=(99*8, 128)
        return np.median(intensities_by_tube, axis=1)

    @property
    def tube_thresholds(self) -> np.ndarray:
        r"""Lower and upper thresholds of each tube
        :returns: array of shape (number_of_tubes, 2) with mininum and maximum thresholds for each tube
        """

    @property
    def collimation_states(self) -> np.ndarray:
        states = np.full(self.eightpack_count, CollimationLevel.Empty, dtype=CollimationLevel)  # initialize as empty
        states[self.config['collimation']['half_col']] = CollimationLevel.Half
        states[self.config['collimation']['full_col']] = CollimationLevel.Full
        return states

    @staticmethod
    def parse_yaml(file_name: str) -> dict:
        """Parse configuration YAML file

        Parameters
        ----------
        file_name: str
            name of the YAML file that contains the configuration for NOMAD

        Returns
        -------
        dictionary
        """
        with open(file_name, 'r') as stream:
            config = yaml.safe_load(stream)
        return config

    @staticmethod
    def get_collimation_states(collimation_config_dict: dict, instrument_config,
                               level: InstrumentComponentLevel) -> numpy.ndarray:
        """Convert 8pack collimation states (in dictionary)

        Parameters
        ----------
        collimation_config_dict
        instrument_config
        level
        num_8packs: int
            number of 8 packs

        Returns
        -------
        numpy.ndarray
            component collimation states

        """
        # Compute the 8-pack level collimation
        collimation_state_array = np.zeros(shape=(instrument_config.num_8packs, ), dtype=int)

        # Read the configuration dict
        full_collimation_8packs = np.array(collimation_config_dict['full_col']) - 1
        half_collimation_8packs = np.array(collimation_config_dict['half_col']) - 1
        collimation_state_array[full_collimation_8packs] = 2
        collimation_state_array[half_collimation_8packs] = 1

        if level == InstrumentComponentLevel.Bank:
            raise RuntimeError(f'Level must be EightPack or up but not {level}')
        elif level == InstrumentComponentLevel.Tube:
            collimation_state_array = collimation_state_array.repeat(instrument_config.num_tubes_per_8pack)
        elif level == InstrumentComponentLevel.Pixel:
            num_repeats = instrument_config.num_tubes_per_8pack * instrument_config.num_pixels_per_tube
            collimation_state_array = collimation_state_array.repeat(num_repeats)

        return collimation_state_array

    @staticmethod
    def set_nomad_constants():
        """Set NOMAD geometry constants for numpy operation

        Returns
        -------
        namedtutple
            named tuple for NOMAD pixel, tube, 8 pack and bank constants

        """
        info_dict = dict()

        info_dict['num_banks'] = 6
        info_dict['num_8packs_per_bank'] = [0, 6, 15, 23, 30, 45, 49]  # [i, i+1) is the range of 8 packs for bank i
        info_dict['num_8packs'] = 49
        info_dict['num_pixels_per_tube'] = 256
        info_dict['num_tubes_per_8pack'] = 8
        info_dict['num_tubes'] = info_dict['num_8packs'] * info_dict['num_tubes_per_8pack']
        info_dict['num_pixels'] = info_dict['num_tubes'] * info_dict['num_pixels_per_tube']

        # convert to namedtuple and return
        instrument = namedtuple("nomad", info_dict)

        return instrument(**info_dict)
