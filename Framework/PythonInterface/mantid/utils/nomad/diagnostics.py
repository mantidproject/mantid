# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.simpleapi import (LoadEmptyInstrument, mtd, MaskDetectors, ExtractMask, SaveMask, DeleteWorkspace)

# 3rd-party imports
import numpy
import numpy as np
import yaml

# standard imports
from collections import namedtuple
import enum
from typing import List


__all__ = ['determine_tubes_threshold']


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
    not_collimated_masks = (pixel_collimation_states == CollimationLevel.Empty.value).astype(int)
    # sanity check
    assert not_collimated_lowers.shape == (instrument_config.num_pixels, )
    assert not_collimated_masks.shape == not_collimated_uppers.shape

    # Calculate boundary for full collimated pixels
    # condition 3:
    full_collimated_lowers = (1. + (low_pixel - 1) * 3.) * tube_m_array
    full_collimated_uppers = (1. + (high_pixel - 1) * 3.) * tube_m_array
    full_collimated_lowers = np.repeat(full_collimated_lowers, instrument_config.num_pixels_per_tube)
    full_collimated_uppers = np.repeat(full_collimated_uppers, instrument_config.num_pixels_per_tube)
    full_collimated_masks = (pixel_collimation_states == CollimationLevel.Full.value).astype(int)

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
    half_collimated_masks = (pixel_collimation_states == CollimationLevel.Half.value).astype(int)

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


class CollimationLevel(enum.Enum):
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
    r"""
    Mixin providing methods to algorithm NOMADMedianDetectorTest

    The following attributes should've been defined and initialized in NOMADMedianDetectorTest:
        intensities: numpy.ma.core.MaskedArray
        config: dict
    """

    # Instrument geomtry
    # TODO these quantities should be derived from the instrument object
    PANEL_COUNT = 6
    EIGHTPACK_COUNT = 99
    TUBES_IN_EIGHTPACK = 8
    TUBE_COUNT = EIGHTPACK_COUNT * TUBES_IN_EIGHTPACK
    PIXELS_IN_TUBE = 128
    PIXELS_IN_EIGHTPACK = TUBES_IN_EIGHTPACK * PIXELS_IN_TUBE
    PIXEL_COUNT = TUBE_COUNT * PIXELS_IN_TUBE

    @staticmethod
    def parse_yaml(file_name: str) -> dict:
        """Parse configuration YAML file and convert the bank, 8pack and tube parameters

        Parameters
        ----------
        file_name: str
            name of the YAML file that contains the configuration for NOMAD

        Returns
        -------
        dictionary
        """
        # Import YMAL to dict
        with open(file_name, 'r') as stream:
            yaml_config = yaml.safe_load(stream)

        # Parse: convert 8pack, tubes indexes from starting-1 to staring-0
        config = dict()

        # Used 8 packs
        config['eight_packs'] = np.array(yaml_config['eight_packs']).astype(int)

        # Collimator states
        config['collimation'] = dict()
        for col_name in ['full_col', 'half_col']:
            config['collimation'][col_name] = config['eight_packs'][yaml_config['collimation'][col_name]]

        # Threshold
        config['threshold'] = yaml_config['threshold']

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

    def _get_intensities(self, input_workspace: 'mantid.api.MatrixWorkspace') -> np.ma.core.MaskedArray:  # noqa F821
        r"""
        Integrated intensity of each pixel for pixels in use. Pixels of unused eightpacks are masked

        Returns
        -------
        1D array of size number-of-pixels
        """
        # Do not count initial spectra which may be related to monitors, not pixel-detectors
        spectrum_info = input_workspace.spectrumInfo()
        spectrum_index = 0
        while spectrum_info.isMonitor(spectrum_index):
            spectrum_index += 1
        # sum counts for each detector histogram
        intensities = np.sum(input_workspace.extractY()[spectrum_index:], axis=1)
        assert len(intensities) == self.PIXEL_COUNT
        return np.ma.masked_array(intensities, mask=~self.pixel_in_use)  # mask unused pixels

    @property
    def tube_range(self) -> List[List[int]]:
        r"""Begin and (1 + end) tube index for each panel.
        @return nested list of shape (number-of-panels, 2)
        """
        # TODO this should be implemented looking at the ComponentInfo object
        eightpack_range = [[0, 14], [14, 37], [37, 51], [51, 63], [63, 81], [81, 99]]
        # extend eightpack indexes to tube indexes
        return (self.TUBES_IN_EIGHTPACK * np.array(eightpack_range)).tolist()

    @property
    def pixel_in_use(self) -> np.ndarray:
        r"""
        Boolean flag, True for pixels in use

        Returns
        -------
        1D array of size PIXEL_COUNT
        """
        flag = np.full(self.EIGHTPACK_COUNT, False)  # initialize all eightpacks as not in use
        flag[self.config['eight_packs']] = True
        return np.repeat(flag, )

    @property
    def tube_intensity(self) -> np.ndarray:
        r"""Sum the pixel intensities for each tube
        @return 1D array of size number of tubes in the instrument"""
        intensities_by_tube = self.intensities.reshape((self.TUBE_COUNT, self.PIXELS_IN_TUBE))  # shape=(99*8, 128)
        return np.sum(intensities_by_tube, axis=1)

    @property
    def tube_collevel(self) -> np.ndarray:
        r"""Collimation level of each tube
        @return 1D array of size number of tubes. Each item is a CollimationLevel"""
        # Find the collimation level of each eightpack
        levels = np.full(self.EIGHTPACK_COUNT, CollimationLevel.Empty, dtype=CollimationLevel)  # initialize as empty
        levels[self.config['collimation']['half_col']] = CollimationLevel.Half
        levels[self.config['collimation']['full_col']] = CollimationLevel.Full
        return np.repeat(levels, self.TUBES_IN_EIGHTPACK)  # extend the collimation levels to each tube

    @property
    def panel_median(self) -> np.ndarray:
        r"""
        @brief Median intensity of a panel, assigned to all tubes in the panel

        @details For the first panel, use all tubes in calculating the median. For other panels,
        use only non-collimated tubes (Half collimated tubes count here as collimated).

        @return 1D array of size number-of-tubes in the instrument. The array will have the same
        mask as that of self.tube_intensity
        """
        # Find tube indexes corresponding to tubes having some level of collimation (a.k.a not Empty)
        index_is_collimated = self.tube_collevel != CollimationLevel.Empty
        # Mark all tubes in the first panel as not collimated
        begin, end = self.tube_range[0]
        index_is_collimated[begin: end] = False  # not collimated
        # Mask tube intensities having some level of collimation
        intensities = np.ma.masked_array(self.tube_intensity, mask=index_is_collimated)
        # Calculate the median on each panel using only the non-masked intensities
        medians = list()
        for begin, end in self.tube_range:
            tube_intensities = intensities[begin: end]
            # extricate the masked tubes before calculating the median
            median = np.median(tube_intensities[~tube_intensities.mask])  # intensity for this panel
            medians.extend([median] * (end - begin))  # extend to all tubes in this panel
        # preserve the same mask as that of the tube intensities
        return np.ma.masked_array(np.array(medians), mask=self.tube_intensity.mask)

    @property
    def eightpack_median(self) -> np.ndarray:
        r"""Median of the set of summed tube intensities in a given eightpack, for all eightpacks, extended to the
        tube level

        @details  Tubes belonging to eightpacks not in use are masked.

        @return 1D array of length numer-of-tubes in the instrument. The array will have the same
        mask as that of self.tube_intensity"""
        intensities = self.tube_intensity.reshape((self.EIGHTPACK_COUNT, self.TUBES_IN_EIGHTPACK))
        medians_eightpack = np.median(intensities, axis=1)  # one median per eightpack
        medians = np.repeat(medians_eightpack, self.TUBES_IN_EIGHTPACK)  # median assigned to the tube
        # preserve the same mask as that of the tube intensities
        return np.ma.masked_array(medians, mask=self.tube_intensity.mask)

    @property
    def mask_by_tube_intensity(self) -> np.ndarray:
        r"""Pixel mask according to total count of pixel intensity in the tube

        @details the summed tube intensity is compared to either the median intensity of its eightpack or the
        median intensity of its panel, depending on whether its eightpack is collimated

        @return boolean 1D array of shape number of pixels. A value of `True` corresponds to a masked pixel
        """
        lower, upper = self.config['threshold']['low_tube'], self.config['threshold']['high_tube']
        tube_is_collimated = self.tube_collevel == CollimationLevel.Full
        # if the tube is collimated, select the median from the eightpack calculation, otherwise select from the panel
        medians = np.where(tube_is_collimated, self.eightpack_median, self.panel_median)
        # np.where returns the index value for indexes where both eightpack_medians and panel_medians are masked
        # thus, we impose the mask of tube_intensity to the medians
        medians = np.ma.masked_array(medians, mask=self.tube_intensity.mask)
        deficient = self.tube_intensity < lower * medians  # tube exhibits insufficient intensity
        excessive = self.tube_intensity > upper * medians  # tube exhibits excessive intensity
        tube_mask = deficient | excessive  # masked tubes denoted with a True value
        # turn the numpy mask into True values. Masked indexes correspond to unused tubes, hence
        # should be flagged as mask == True
        tube_mask[self.tube_intensity.mask] = True
        tube_mask = tube_mask.data  # cast numpy.ma.core.MaskedArray to numpy.ndarray
        return np.repeat(tube_mask, self.PIXELS_IN_TUBE)  # extend the tube mask states to all pixels in the tubes

    @property
    def mask_by_pixel_intensity(self) -> np.ndarray:
        # TODO move function "determine_tubes_threshold"() here
        pass

    def export_mask(self):
        # TODO move function "export_masks()" here
        pass
