# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Projections.projection import projection
import numpy as np
from instrumentview.Detectors import DetectorPosition


class spherical_projection(projection):
    """2D projection with a spherical coordinate system, see https://en.wikipedia.org/wiki/Spherical_coordinate_system"""

    def __init__(
        self, sample_position: np.ndarray, root_position: np.ndarray, detector_positions: list[DetectorPosition], axis: np.ndarray
    ):
        super().__init__(sample_position, root_position, detector_positions, axis)

    def _calculate_2d_coordinates(self, detector_position: np.ndarray) -> tuple[float, float]:
        detector_relative_position = detector_position - self._sample_position
        v = detector_relative_position.dot(self._projection_axis)
        x = detector_relative_position.dot(self._x_axis)
        y = detector_relative_position.dot(self._y_axis)

        r = np.sqrt(x * x + y * y + v * v)
        # u_scale = 1. / np.sqrt(x * x + y * y)
        # v_scale = 1. / r

        u = -np.atan2(y, x)  # * u_scale
        v = -np.acos(v / r)  # * v_scale
        return (u, v)
