# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np


class DetectorInfo:
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
