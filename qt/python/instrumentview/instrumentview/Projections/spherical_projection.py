# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Projections.projection import projection
import numpy as np


class spherical_projection(projection):
    def __init__(self, workspace, detector_indices, axis: np.ndarray):
        super().__init__(workspace, detector_indices)
        self._projection_axis = axis
        self._calculate_axes()
        self._calculate_detector_coordinates()
        self._find_and_correct_x_gap()

    def _calculate_detector_coordinates(self):
        x_values = []
        y_values = []
        for det_id in self._detector_indices:
            x, y = self._calculate_2d_coordinates(det_id)
            x_values.append(x)
            y_values.append(y)
        self._detector_x_coordinates = np.array(x_values)
        self._detector_y_coordinates = np.array(y_values)
        self._x_range = (self._detector_x_coordinates.min(), self._detector_x_coordinates.max())
        self._y_range = (self._detector_y_coordinates.min(), self._detector_y_coordinates.max())

    def _calculate_2d_coordinates(self, detector_index: int) -> tuple[float, float]:
        detector_relative_position = np.array(self._component_info.position(detector_index)) - self._sample_position
        v = detector_relative_position.dot(self._projection_axis)
        x = detector_relative_position.dot(self._x_axis)
        y = detector_relative_position.dot(self._y_axis)

        r = np.sqrt(x * x + y * y + v * v)
        # u_scale = 1. / np.sqrt(x * x + y * y)
        # v_scale = 1. / r

        u = -np.atan2(y, x)  # * u_scale
        v = -np.acos(v / r)  # * v_scale
        return (u, v)
