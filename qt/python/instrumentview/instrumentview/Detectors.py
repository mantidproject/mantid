# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np


class DetectorPosition(np.ndarray):
    """A numpy array that uses a tolerance on the data to check for equality"""

    def __new__(cls, input_array):
        return np.asarray(input_array).view(cls)

    def __eq__(self, other):
        return np.allclose(self, other)


class DetectorInfo:
    """Class for wrapping up information relating to a detector. Used for transferring this info to the View to be displayed."""

    def __init__(
        self,
        name: str,
        detector_id: int,
        workspace_index: int,
        xyz_position: np.ndarray,
        spherical_position: np.ndarray,
        component_path: str,
        pixel_counts: int,
    ):
        self.name = name
        self.detector_id = detector_id
        self.workspace_index = workspace_index
        self.xyz_position = xyz_position
        self.spherical_position = spherical_position
        self.component_path = component_path
        self.pixel_counts = pixel_counts


class Detector:
    """Class for wrapping up information relating to a detector. We need the detector ID,
    position, whether it is a monitor, and the index in the whole list of detectors."""

    def __init__(self, index: int, id: int, detector_info):
        self.index_in_whole_list = index
        self.id = id
        position = detector_info.position(index)
        self.position = DetectorPosition(position)
        self.spherical_position = position.getSpherical()
        self.is_monitor = detector_info.isMonitor(index)
