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


__all__ = ['determine_tubes_threshold']


def determine_tubes_threshold():
    return


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

