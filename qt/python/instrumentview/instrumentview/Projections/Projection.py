# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABC, abstractmethod
import numpy as np
from instrumentview.Detectors import DetectorPosition


class Projection(ABC):
    """Base class for calculating a 2D projection with a specified axis"""

    def __init__(
        self,
        sample_position: np.ndarray,
        root_position: np.ndarray,
        detector_positions: list[DetectorPosition] | np.ndarray,
        axis: np.ndarray,
    ):
        """For the given workspace and detectors, calculate 2D points with specified projection axis"""

        self._sample_position = np.asarray(sample_position, dtype=np.float64)
        self._root_position = np.asarray(root_position, dtype=np.float64)
        self._detector_positions = np.asarray(detector_positions, dtype=np.float64)
        self._projection_axis = np.asarray(axis, dtype=np.float64)

        self._x_axis = np.zeros_like(self._projection_axis, dtype=np.float64)
        self._y_axis = np.zeros_like(self._projection_axis, dtype=np.float64)
        self._detector_x_coordinates = np.zeros(len(self._detector_positions))
        self._detector_y_coordinates = np.zeros(len(self._detector_positions))
        self._x_range = (0, 0)
        self._y_range = (0, 0)

        self._u_period = 2 * np.pi

        self._calculate_axes(self._root_position)
        self._calculate_detector_coordinates()
        self._find_and_correct_x_gap()

    def _calculate_axes(self, root_position: np.ndarray) -> None:
        """The projection axis is specified, we calculate a 3D coordinate system based on that"""
        z = root_position.dot(self._projection_axis)
        if z == 0 or np.abs(z) == np.linalg.norm(root_position):
            # Find the shortest projection of the projection axis and direct the x axis along it
            if np.abs(self._projection_axis[2]) < np.abs(self._projection_axis[1]):
                self._x_axis = np.array([0, 0, 1], dtype=np.float64)
            elif np.abs(self._projection_axis[1]) < np.abs(self._projection_axis[0]):
                self._x_axis = np.array([0, 1, 0], dtype=np.float64)
            else:
                self._x_axis = np.array([1, 0, 0], dtype=np.float64)
        else:
            x_axis = root_position - z * self._projection_axis
            self._x_axis = x_axis / np.linalg.norm(x_axis)

        self._y_axis = np.cross(self._projection_axis, self._x_axis)

    @abstractmethod
    def _calculate_2d_coordinates(self) -> tuple[np.ndarray, np.ndarray]:
        pass

    def _calculate_detector_coordinates(self) -> None:
        """Calculate 2D projection coordinates and store data"""

        self._detector_x_coordinates, self._detector_y_coordinates = self._calculate_2d_coordinates()

        self._x_range = (self._detector_x_coordinates.min(), self._detector_x_coordinates.max())
        self._y_range = (self._detector_y_coordinates.min(), self._detector_y_coordinates.max())

    def _find_and_correct_x_gap(self) -> None:
        """Shift points based on the specified period so that they appear within the correct x range when plotted"""
        if self._u_period == 0:
            return

        if self._x_range[1] == self._x_range[0]:
            return

        # Find biggest gap in x coordinates
        sorted_x_coordinates = np.sort(self._detector_x_coordinates)
        x_gap_idx = np.argmax(np.diff(sorted_x_coordinates))
        x_from = sorted_x_coordinates[x_gap_idx]
        x_to = sorted_x_coordinates[x_gap_idx + 1]

        if x_to - x_from <= self._u_period - (self._x_range[1] - self._x_range[0]):
            return

        # Update range that avoids gap entirely, wraps around gap
        self._x_range = (x_to, x_from + self._u_period)

        self._apply_x_correction()

    def _apply_x_correction(self) -> None:
        """
        Updates x coordinates outside of current x range to be corrected to fit inside range.
        Correction is applied by adding or subtracting a multiple of the period.
        For example:
            current range is (-2.1, np.pi)
            current point at -np.pi is updated to -np.pi + 2*np.pi = np.pi
        """
        if self._u_period == 0:
            return

        # Get view of x coordinates, can change in-place
        x = self._detector_x_coordinates
        x_min, x_max = self._x_range

        x[x < x_min] += np.floor((x_max - x[x < x_min]) / self._u_period) * self._u_period
        x[x > x_max] -= np.floor((x[x > x_max] - x_min) / self._u_period) * self._u_period

    def coordinate_for_detector(self, detector_index: int) -> tuple[float, float]:
        return (self._detector_x_coordinates[detector_index], self._detector_y_coordinates[detector_index])

    def positions(self) -> np.ndarray:
        return np.vstack([self._detector_x_coordinates, self._detector_y_coordinates]).transpose()
