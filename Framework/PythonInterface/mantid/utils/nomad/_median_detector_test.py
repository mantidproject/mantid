# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# 3rd-party imports
import numpy as np
import yaml

# standard imports
import enum


__all__ = ['determine_tubes_threshold']


# In-progress Task 335
def determine_tubes_threshold(vec_intensity, nomad_info, config):
    """

    Refer to diagram
    https://code.ornl.gov/sns-hfir-scse/diffraction/powder/powder-diffraction/
    uploads/99f4b65655e05edd476999942ff6fb98/Step-1.png
    from box 'Select a tube' to 'All tubes finished'


    Parameters
    ----------
    vec_intensity: numpy.ndarray
        solid angle corrected counts. shape = (101378, )

    Returns
    -------

    """
    # Workflow: process full-collimated, half-collimated and not-collimated separately

    # reshape the input intensities to 2D matrix: each row is a tube, i.e., (num_tubes, num_pixel_per_tube)
    # or say (392, 256)
    tube_pixel_intensity_array = vec_intensity.flatten().reshape((nomad_info.num_tubes,
                                                                  nomad_info.num_pixels_per_tube))

    _determine_full_collimated_tubes_thresholds(tube_pixel_intensity_array, nomad_info, config)

    # _determine_half_collimated_tubes_thresholds(tube_pixel_intensity_array, nomad_info, config)

    # _determine_none_collimated_tubes_thresholds(tube_pixel_intensity_array, nomad_info, config)


def _determine_full_collimated_tubes_thresholds(self, vec_intensity, nomad_info, config):
    """Determine the full-collimated tubes' thresholds

    Parameters
    ----------
    vec_intensity
    nomad_info
    config

    Returns
    -------

    """
    # Get the boolean array for full-collimated tubes: shape = (392, )
    full_collimated_tubes = self.get_tubes_by_collimation_status(config)
    full_collimated_pixels = np.repeat(full_collimated_tubes, nomad_info.num_tubes)

    # Calculate median value the tubes
    tube_median_array = np.median(vec_intensity, axis=1)
    tube_median_array = np.repeat(tube_median_array, nomad_info.num_tubes)

    # Condition 3:
    # summed pixel intensity < (1+(low_pixel-1)*3) * m or
    # summed pixel intensity > (1+(high_pixel-1)*3) * m
    # where low_pixel and high_pixel are parameters defined in the provided YAML configuration file above.
    lower_boundaries = (1. + (config.low_pixel - 1) * 3.) * tube_median_array
    upper_boundaries = (1. + (config.high_pixel - 1) * 3.) * tube_median_array

    # Check pixels meets conditions 3 for pixels in full_collimated_tubes
    masked_pixels = np.where((tube_median_array < lower_boundaries or tube_median_array > upper_boundaries)
                             and full_collimated_pixels)

    return masked_pixels


class CollimationLevel(enum.Enum):
    r"""Collimation state of an eight-pack"""
    Empty = 0
    Half = 1
    Full = 2


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
