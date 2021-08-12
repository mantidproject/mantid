# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


# 3rd-party imports
import numpy as np

# standard imports
import enum
from typing import List


class CollimationLevel(enum.Enum):
    r"""Collimation state of an eight-pack"""
    Empty = 0
    Half = 1
    Full = 2


class _NOMADMedianDetectorTest:
    r"""
    Mixin providing methods to algorithm NOMADMedianDetectorTest
    The following attributes should've been defined in NOMADMedianDetectorTest:
        intensities: numpy.ma.core.MaskedArray
    """

    # Instrument geomtry
    # TODO these quantities should be derived from the instrument object
    PANEL_COUNT = 6
    EIGHTPACK_COUNT = 99
    TUBES_IN_EIGHTPACK = 8
    TUBE_COUNT = EIGHTPACK_COUNT * TUBES_IN_EIGHTPACK
    PIXELS_IN_TUBE = 128
    PIXEL_COUNT = TUBE_COUNT * PIXELS_IN_TUBE

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
        medians = np.where(tube_is_collimated, self.eightpack_median, self.panel_median)
        # np.where returns the index value for indexes where both eightpack_medians and panel_medians are masked
        # we impose the mask of tube_intensity to the medians
        medians = np.ma.masked_array(medians, mask=self.tube_intensity.mask)
        deficient = self.tube_intensity < lower * medians
        excessive = self.tube_intensity > upper * medians
        tube_mask = deficient | excessive  # masked tubes denoted with a True value
        # turn the numpy mask into True values. These are unused tubes which should be flagged as masked
        tube_mask[self.tube_intensity.mask] = True
        tube_mask = tube_mask.data  # drop the mask feature of this numpy array. Not useful anymore
        return np.repeat(tube_mask, self.PIXELS_IN_TUBE)  # extend to all pixels in the tube
