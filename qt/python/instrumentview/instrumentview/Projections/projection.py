# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABC, abstractmethod
import numpy as np
from instrumentview.Detectors import DetectorPosition


class projection(ABC):
    """Base class for calculating a 2D projection with a specified axis"""

    def __init__(
        self,
        sample_position: np.ndarray,
        root_position: np.ndarray,
        detector_positions: list[DetectorPosition] | np.ndarray,
        axis: np.ndarray,
    ):
        """For the given workspace and detectors, calculate 2D points with specified projection axis"""

        self._sample_position = sample_position
        self._root_position = root_position
        self._detector_positions = detector_positions
        self._projection_axis = np.array(axis)

        self._x_axis = np.zeros_like(self._projection_axis)
        self._y_axis = np.zeros_like(self._projection_axis)
        self._detector_x_coordinates = np.zeros(len(self._detector_positions))
        self._detector_y_coordinates = np.zeros(len(self._detector_positions))
        self._x_range = (0, 0)
        self._y_range = (0, 0)

        self._u_period = np.pi

        self._calculate_axes(root_position)
        self._calculate_detector_coordinates()
        self._find_and_correct_x_gap()

    def _calculate_axes(self, root_position: np.ndarray) -> None:
        """The projection axis is specified, we calculate a 3D coordinate system based on that"""
        z = root_position.dot(self._projection_axis)
        if z == 0 or np.abs(z) == np.linalg.norm(root_position):
            # Find the shortest projection of the projection axis and direct the x axis along it
            if np.abs(self._projection_axis[2]) < np.abs(self._projection_axis[1]):
                self._x_axis = np.array([0, 0, 1])
            elif np.abs(self._projection_axis[1]) < np.abs(self._projection_axis[0]):
                self._x_axis = np.array([0, 1, 0])
            else:
                self._x_axis = np.array([1, 0, 0])
        else:
            x_axis = root_position - z * self._projection_axis
            self._x_axis = x_axis / np.linalg.norm(x_axis)

        self._y_axis = np.cross(self._projection_axis, self._x_axis)

    @abstractmethod
    def _calculate_2d_coordinates(self, detector_position: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
        pass

    def _calculate_detector_coordinates(self):
        """Calculate 2D projection coordinates and store data"""

        self._detector_x_coordinates, self._detector_y_coordinates = self._calculate_2d_coordinates(np.array(self._detector_positions))

        self._x_range = (self._detector_x_coordinates.min(), self._detector_x_coordinates.max())
        self._y_range = (self._detector_y_coordinates.min(), self._detector_y_coordinates.max())

    def _find_and_correct_x_gap(self):
        """Shift points based on the specified period so that they appear within the correct x range when plotted"""
        if self._u_period == 0:
            return

        if self._x_range[1] == self._x_range[0]:
            return

        number_of_bins = 1000
        bin_width = np.abs(self._x_range[1] - self._x_range[0]) / (number_of_bins - 1)
        x_bins = np.full(number_of_bins, False)
        bin_indices = ((self._detector_x_coordinates - self._x_range[0]) / bin_width).astype(int)
        x_bins[bin_indices] = True

        i_from = 0
        i_to = 0
        i0 = 0
        in_gap = False

        for i in range(number_of_bins):
            if not x_bins[i]:
                if not in_gap:
                    i0 = i
                in_gap = True
            else:
                if in_gap and i_to - i_from < i - i0:
                    i_from = i0  # First bin in the gap
                    i_to = i  # First bin after the gap
                in_gap = False

        x_from = self._x_range[0] + i_from * bin_width
        x_to = self._x_range[0] + i_to * bin_width
        if x_to - x_from > self._u_period - (self._x_range[1] - self._x_range[0]):
            self._x_range = (x_to, x_from)
            if self._x_range[0] > self._x_range[1]:
                self._x_range = (self._x_range[0], self._x_range[1] + self._u_period)

            self._apply_x_correction()

    def _apply_x_correction(self) -> None:
        """Set x coordinate of specified point to be within the correct range, with the period used as the modulus"""
        x = self._detector_x_coordinates
        x_min = self._x_range[0]
        x_max = self._x_range[1]
        if self._u_period == 0:
            return

        x[x < x_min] += np.floor((x_max - x[x < x_min]) / self._u_period) * self._u_period
        x[x > x_max] -= np.floor((x[x > x_max] - x_min) / self._u_period) * self._u_period

    def coordinate_for_detector(self, detector_index: int) -> tuple[float, float]:
        return (self._detector_x_coordinates[detector_index], self._detector_y_coordinates[detector_index])

    def positions(self) -> np.ndarray:
        return np.vstack([self._detector_x_coordinates, self._detector_y_coordinates]).transpose()
