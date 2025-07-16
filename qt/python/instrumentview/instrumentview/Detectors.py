# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from dataclasses import dataclass


class DetectorPosition(np.ndarray):
    """A numpy array that uses a tolerance on the data to check for equality"""

    def __new__(cls, input_array):
        return np.asarray(input_array).view(cls)

    def __eq__(self, other):
        return np.allclose(self, other)


@dataclass
class DetectorInfo:
    """
    Class for wrapping up information relating to a detector.
    Used for transferring detector info from Model to View to be displayed.
    """

    name: str
    detector_id: int
    workspace_index: int
    xyz_position: np.ndarray
    spherical_position: np.ndarray
    component_path: str
    pixel_counts: int
