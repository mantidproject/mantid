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

# standard imports
from collections import namedtuple
import enum
import random
import string
from typing import List, NamedTuple

from mantid.api import MatrixWorkspace


class CollimationLevel(enum.Enum):
    r"""Collimation state of an eight-pack"""

    Empty = 0
    Half = 1
    Full = 2


class InstrumentComponentLevel(enum.IntEnum):
    """
    Instrument component level
    """

    Panel = 0  # a.k.a "Panel"
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
    MONITOR_COUNT = 2  # number of upstream monitors
    PANEL_COUNT = 6
    PANEL_FLAT = [False, False, False, False, True, True]  # last two panels are flat
    EIGHTPACK_COUNT = 99
    TUBES_IN_EIGHTPACK = 8
    TUBE_COUNT = EIGHTPACK_COUNT * TUBES_IN_EIGHTPACK
    PIXELS_IN_TUBE = 128
    PIXELS_IN_EIGHTPACK = TUBES_IN_EIGHTPACK * PIXELS_IN_TUBE
    PIXEL_COUNT = TUBE_COUNT * PIXELS_IN_TUBE

    @staticmethod
    def _random_string(prefix: str = "_", n: int = 9) -> str:
        r"""
        String of random characters

        Parameters
        ----------
        prefix
            string to prefix to the random string
        n
            length of the random string
        """
        letters = string.ascii_lowercase
        return prefix + "".join([random.choice(letters) for i in range(n)])

    @staticmethod
    def parse_yaml(file_name: str) -> dict:
        """Parse configuration YAML file and convert the Panel, 8pack and tube parameters

        Parameters
        ----------
        file_name: str
            name of the YAML file that contains the configuration for NOMAD

        Returns
        -------
        dictionary
        """
        # Import YMAL to dict
        with open(file_name, "r") as stream:
            yaml_config = yaml.safe_load(stream)

        # Parse: convert 8pack, tubes indexes from starting-1 to staring-0
        config = dict()

        # Used 8 packs: index from 0
        config["eight_packs"] = np.array(yaml_config["eight_packs"]).astype(int)

        # Collimator states
        config["collimation"] = dict()
        for col_name in ["full_col", "half_col"]:
            # Value given is the index of the used 8 packs given in 'eight_packs'
            # Now it is converted to the real 8 pack indexes, which starts from 0
            config["collimation"][col_name] = config["eight_packs"][yaml_config["collimation"][col_name]]

        # Threshold
        config["threshold"] = yaml_config["threshold"]

        return config

    @staticmethod
    def get_collimation_states(
        collimation_config_dict: dict, instrument_config: NamedTuple, level: InstrumentComponentLevel
    ) -> numpy.ndarray:
        """
        Convert 8pack collimation states (in dictionary)
        State values are 0 (no collimation), 1 (half collimation), and 2 (full collimation)

        Parameters
        ----------
        collimation_config_dict
        instrument_config
        level

        Returns
        -------
        numpy.ndarray
            component collimation states as integers

        """
        # Compute the 8-pack level collimation
        collimation_state_array = np.zeros(shape=(instrument_config.num_8packs,), dtype=int)

        # Read the configuration dict
        full_collimation_8packs = collimation_config_dict["full_col"]
        half_collimation_8packs = collimation_config_dict["half_col"]
        collimation_state_array[full_collimation_8packs] = 2
        collimation_state_array[half_collimation_8packs] = 1

        if level == InstrumentComponentLevel.Panel:
            raise RuntimeError(f"Level must be EightPack or up but not {level}")
        elif level == InstrumentComponentLevel.Tube:
            collimation_state_array = collimation_state_array.repeat(instrument_config.num_tubes_per_8pack)
        elif level == InstrumentComponentLevel.Pixel:
            num_repeats = instrument_config.num_tubes_per_8pack * instrument_config.num_pixels_per_tube
            collimation_state_array = collimation_state_array.repeat(num_repeats)

        return collimation_state_array

    @classmethod
    def set_nomad_constants(cls):
        """Set NOMAD geometry constants for numpy operation

        Returns
        -------
        namedtutple
            named tuple for NOMAD pixel, tube, 8 pack and Panel constants

        """
        info_dict = dict()

        info_dict["num_Panels"] = cls.PANEL_COUNT
        info_dict["num_8packs"] = cls.EIGHTPACK_COUNT
        info_dict["num_pixels_per_tube"] = cls.PIXELS_IN_TUBE
        info_dict["num_tubes_per_8pack"] = cls.TUBES_IN_EIGHTPACK

        info_dict["num_tubes"] = info_dict["num_8packs"] * info_dict["num_tubes_per_8pack"]
        info_dict["num_pixels"] = info_dict["num_tubes"] * info_dict["num_pixels_per_tube"]

        # convert to namedtuple and return
        instrument = namedtuple("nomad", info_dict)

        return instrument(**info_dict)

    @classmethod
    def determine_tubes_threshold(cls, vec_intensity, mask_config: dict, instrument_config):
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
        # If vec_intensity is a masked array, we set the intensities of the masked indexes to zero. Later we'll set
        # these indexes to have a mask of True.
        intensities = vec_intensity.flatten()  # intensities is a copy of vec_intensities, even for 1D input arrays
        try:
            intensities[intensities.mask] = 0
            intensities = intensities.data  # cast from numpy.ma.core.MaskedArray to numpy.ndarray
        except AttributeError:  # vec_intensity is not a masked array
            pass

        # Workflow: process full-collimated, half-collimated and not-collimated separately
        pixel_collimation_states = cls.get_collimation_states(mask_config["collimation"], instrument_config, InstrumentComponentLevel.Pixel)

        # reshape the input intensities to 2D matrix: each row is a tube, i.e., (num_tubes, num_pixel_per_tube)
        # or say (392, 256)
        tube_pixel_intensity_array = intensities.reshape((instrument_config.num_tubes, instrument_config.num_pixels_per_tube))

        # Calculate tube medians: m, m1, m2
        half_tube_pixels = instrument_config.num_pixels_per_tube // 2
        # m
        tube_m_array = np.median(tube_pixel_intensity_array, axis=1)
        # m1
        tube_m1_array = np.median(tube_pixel_intensity_array[:, :half_tube_pixels], axis=1)
        # m2
        tube_m2_array = np.median(tube_pixel_intensity_array[:, half_tube_pixels:], axis=1)

        # Boundary constants
        low_pixel = mask_config["threshold"]["low_pixel"]
        high_pixel = mask_config["threshold"]["high_pixel"]

        # Calculate boundary for not collimated pixels
        # condition 2:  summed pixel intensity < low_pixel * m or summed pixel intensity > high_pixel * m,
        not_collimated_lowers = low_pixel * tube_m_array
        not_collimated_uppers = high_pixel * tube_m_array
        not_collimated_lowers = np.repeat(not_collimated_lowers, instrument_config.num_pixels_per_tube)
        not_collimated_uppers = np.repeat(not_collimated_uppers, instrument_config.num_pixels_per_tube)
        not_collimated_masks = (pixel_collimation_states == CollimationLevel.Empty.value).astype(int)
        # sanity check
        assert not_collimated_lowers.shape == (instrument_config.num_pixels,)
        assert not_collimated_masks.shape == not_collimated_uppers.shape

        # Calculate boundary for full collimated pixels
        # condition 3:
        full_collimated_lowers = (1.0 + (low_pixel - 1) * 3.0) * tube_m_array
        full_collimated_uppers = (1.0 + (high_pixel - 1) * 3.0) * tube_m_array
        full_collimated_lowers = np.repeat(full_collimated_lowers, instrument_config.num_pixels_per_tube)
        full_collimated_uppers = np.repeat(full_collimated_uppers, instrument_config.num_pixels_per_tube)
        full_collimated_masks = (pixel_collimation_states == CollimationLevel.Full.value).astype(int)

        # Calculate boundaries for half collimated pixels
        # condition 4: summed pixel intensity < (1+(low_pixel-1)*3) * m1 or
        # summed pixel intensity > (1+(high_pixel-1)*3) * m1,
        # condition 5:  summed pixel intensity < (1+(low_pixel-1)*3) * m2 or
        # summed pixel intensity > (1+(high_pixel-1)*3) * m2,
        half1_collimated_lowers = (1.0 + (low_pixel - 1) * 3.0) * tube_m1_array
        half2_collimated_lowers = (1.0 + (low_pixel - 1) * 3.0) * tube_m2_array
        half_collimated_lowers = np.repeat(half1_collimated_lowers, 2)
        half_collimated_lowers[1::2] = half2_collimated_lowers
        half_collimated_lowers = np.repeat(half_collimated_lowers, instrument_config.num_pixels_per_tube // 2)
        assert half_collimated_lowers.shape == (instrument_config.num_pixels,)

        half_collimated_uppers = (1.0 + (high_pixel - 1) * 3.0) * tube_m1_array
        half2_collimated_uppers = (1.0 + (high_pixel - 1) * 3.0) * tube_m2_array
        half_collimated_uppers = np.repeat(half_collimated_uppers, 2)
        half_collimated_uppers[1::2] = half2_collimated_uppers
        half_collimated_uppers = np.repeat(half_collimated_uppers, instrument_config.num_pixels_per_tube // 2)
        assert half_collimated_uppers.shape == (instrument_config.num_pixels,)
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
        pixel_mask_states += vec_intensity > upper_boundaries

        # If vec_intensity is a masked array, set the masked indexes as True.
        try:
            pixel_mask_states[vec_intensity.mask] = True
        except AttributeError:  # vec_intensity is not a masked array
            pass
        return pixel_mask_states

    def _get_intensities(self, intensities_workspace: MatrixWorkspace) -> np.ma.core.MaskedArray:  # F821
        r"""
        Integrated intensity of each pixel for pixels in use. Pixels of unused eightpacks are masked

        Parameters
        ----------
        input_workspace: workspace containing pixel intensities

        Returns
        -------
        1D array of size number-of-pixels
        """
        intensities_workspace
        # Do not count initial spectra which may be related to monitors, not pixel-detectors
        spectrum_info = intensities_workspace.spectrumInfo()
        spectrum_index = 0
        while spectrum_info.isMonitor(spectrum_index):
            spectrum_index += 1
        # sum counts for each detector histogram
        intensities = np.sum(intensities_workspace.extractY()[spectrum_index:], axis=1)
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
    def tube_in_flat_panel(self) -> np.ndarray:
        r"""Boolean list indicating whether each tube belong to flat panel"""
        checks = list()  # will contains True or False for each tube index
        # iterate over the range of tubes for each panel
        for (begin, end), state in zip(self.tube_range, self.PANEL_FLAT):
            checks.extend([state] * (end - begin))
        return np.array(checks)

    @property
    def pixel_in_flat_panel(self) -> np.ndarray:
        r"""Boolean list indicating whether each pixel belong to flat panel"""
        return np.repeat(self.tube_in_flat_panel, self.PIXELS_IN_TUBE)

    @property
    def pixel_in_use(self) -> np.ndarray:
        r"""
        Boolean flag, True for pixels in use

        Returns
        -------
        1D array of size PIXEL_COUNT
        """
        flag = np.full(self.EIGHTPACK_COUNT, False)  # initialize all eightpacks as not in use
        flag[self.config["eight_packs"]] = True
        return np.repeat(flag, self.PIXELS_IN_EIGHTPACK)  # assign the flag to all pixels in the eight-pack

    @property
    def tube_intensity(self) -> np.ndarray:
        r"""Sum the pixel intensities for each tube
        @return 1D array of size number of tubes in the instrument"""
        intensities_by_tube = self.intensities.reshape((self.TUBE_COUNT, self.PIXELS_IN_TUBE))  # shape=(99*8, 128)
        return np.sum(intensities_by_tube, axis=1)

    @property
    def tube_collevel(self) -> np.ndarray:
        r"""Collimation level of each tube
        @return 1D array of size number of tubes. Each item is a CollimationLevel enumeration"""
        # Find the collimation level of each eightpack
        levels = np.full(self.EIGHTPACK_COUNT, CollimationLevel.Empty, dtype=CollimationLevel)  # initialize as empty
        levels[self.config["collimation"]["half_col"]] = CollimationLevel.Half
        levels[self.config["collimation"]["full_col"]] = CollimationLevel.Full
        return np.repeat(levels, self.TUBES_IN_EIGHTPACK)  # extend the collimation levels to each tube

    @property
    def pixel_collevel(self) -> np.ndarray:
        r"""
        Collimation level of each pixel

        Returns
        -------
        1D array of size PIXEL_COUNT. Each item is a CollimationLevel enumeration"""
        return np.repeat(self.tube_collevel, self.PIXELS_IN_TUBE)

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
        index_is_collimated[begin:end] = False  # not collimated
        # Mask tube intensities having some level of collimation
        intensities = np.ma.masked_array(self.tube_intensity, mask=index_is_collimated)
        # Calculate the median on each panel using only the non-masked intensities
        medians = list()
        for begin, end in self.tube_range:
            tube_intensities = intensities[begin:end]
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
        # find lower and upper multiplicative factors according to whether the tube belongs in a flat panel
        lowers = np.where(self.tube_in_flat_panel, 0.1, self.config["threshold"]["low_tube"])
        uppers = np.where(self.tube_in_flat_panel, np.inf, self.config["threshold"]["high_tube"])
        # start all medians as those from the eightpack calculation
        medians = np.copy(self.eightpack_median)
        # if the tube is in a non-flat panel and is non-collimated, select the median from the panel calculation
        not_collimated = self.tube_collevel != CollimationLevel.Full
        medians = np.where(~self.tube_in_flat_panel & not_collimated, self.panel_median, medians)
        # np.where returns the index value for indexes where both eightpack_medians and panel_medians are masked
        # thus, we impose the mask of tube_intensity to the medians
        medians = np.ma.masked_array(medians, mask=self.tube_intensity.mask)
        deficient = self.tube_intensity < lowers * medians  # tube exhibits insufficient intensity
        excessive = self.tube_intensity > uppers * medians  # tube exhibits excessive intensity
        tube_mask = deficient | excessive  # masked tubes denoted with a True value
        # turn the numpy mask into True values. Masked indexes correspond to unused tubes, hence
        # should be flagged as mask == True
        tube_mask[self.tube_intensity.mask] = True
        tube_mask = tube_mask.data  # cast numpy.ma.core.MaskedArray to numpy.ndarray
        return np.repeat(tube_mask, self.PIXELS_IN_TUBE)  # extend the tube mask states to all pixels in the tubes

    @property
    def mask_by_pixel_intensity(self):
        return self.determine_tubes_threshold(self.intensities, self.config, self.set_nomad_constants())
