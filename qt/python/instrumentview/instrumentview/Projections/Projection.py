# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import abstractmethod
import numpy as np
from instrumentview.Detectors import DetectorPosition


class Projection:
    """Base class for calculating a 2D projection with a specified axis"""

    _registry = {}
    _U_PERIOD = 2 * np.pi

    def __init_subclass__(cls, projection_types=None, **kwargs):
        super().__init_subclass__(**kwargs)
        if projection_types:
            for projection_type, defaults in projection_types.items():
                Projection._registry[projection_type] = (cls, defaults)

    def __new__(cls, type, **kwargs):
        if cls is Projection:
            entry = Projection._registry.get(type)
            if not entry:
                raise ValueError(f"Unknown projection type: '{type}'. Available: {list(Projection._registry)}")
            subclass, _ = entry
            return super().__new__(subclass)
        return super().__new__(cls)

    def __init__(
        self,
        type,
        sample_position: np.ndarray,
        root_position: np.ndarray,
        detector_positions: list[DetectorPosition] | np.ndarray,
        **kwargs,
    ):
        """For the given workspace and detectors, calculate 2D points with specified projection axis"""

        self.type = type
        _, defaults = Projection._registry[type]
        self._projection_axis = np.asarray(defaults["axis"], dtype=np.float64)

        self._sample_position = np.asarray(sample_position, dtype=np.float64)
        self._root_position = np.asarray(root_position, dtype=np.float64)
        self._detector_positions = np.asarray(detector_positions, dtype=np.float64)

        self._x_axis = np.zeros_like(self._projection_axis, dtype=np.float64)
        self._y_axis = np.zeros_like(self._projection_axis, dtype=np.float64)
        self._detector_x_coordinates = np.zeros(len(self._detector_positions))
        self._detector_y_coordinates = np.zeros(len(self._detector_positions))
        self._raw_x_coordinates = np.zeros(len(self._detector_positions))
        self._y_range = (0, 0)

        self._u_offset = 0.0

        self._calculate_axes(self._root_position)
        self._calculate_detector_coordinates()
        # Where the projection is cut open when it is not rotated, or None if it is left as calculated
        self._auto_seam = self._find_auto_seam()
        self._apply_u_offset()

    @property
    def u_period(self) -> float:
        """The width in x of one full turn about the projection axis."""
        return self._U_PERIOD

    @property
    def u_offset(self) -> float:
        """The angle the projection is rotated by about its axis, measured from the automatically chosen seam."""
        return self._u_offset

    def set_u_offset(self, offset: float) -> None:
        """Rotate the projection about its axis, moving the seam at which the instrument is cut open.

        The automatically chosen seam is used when the offset is zero.
        """
        offset = float(np.mod(offset, self._U_PERIOD))

        if offset == self._u_offset:
            return

        self._u_offset = offset
        self._apply_u_offset()

    @property
    def _seam(self) -> float | None:
        """The x coordinate at which the projection is cut open, or None if it is not wrapped."""
        if self._auto_seam is None:
            return None
        return self._auto_seam + self._u_offset

    def _apply_u_offset(self) -> None:
        """Wrap the detector x coordinates into the period starting at the current seam."""
        if self._auto_seam is None:
            return
        self._detector_x_coordinates = self._wrap_x(self._raw_x_coordinates)

    def _wrap_x(self, x_values: np.ndarray) -> np.ndarray:
        """Shift x values by whole periods into [seam, seam + period). The range is half open, so a
        point sitting exactly on the seam stays at the near edge."""
        seam = self._seam
        return seam + np.mod(x_values - seam, self._U_PERIOD)

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

    @abstractmethod
    def _calculate_2d_coordinates_from_relative_positions(self, detector_relative_positions: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
        """Project detector-relative positions to 2D coordinates."""
        pass

    def _calculate_detector_coordinates(self) -> None:
        """Calculate 2D projection coordinates and store data"""

        self._detector_x_coordinates, self._detector_y_coordinates = self._calculate_2d_coordinates()
        self._raw_x_coordinates = self._detector_x_coordinates.copy()

        self._y_range = (self._detector_y_coordinates.min(), self._detector_y_coordinates.max())

    def _find_auto_seam(self) -> float | None:
        """Choose where to cut the projection open so that the widest gap between detectors falls at the edges.

        Returns the far side of the widest gap if it is wider than the gap already left across the ends of the
        calculated range, otherwise the lowest x coordinate. Returns None if all the detectors share one x coordinate.
        """
        sorted_x_coordinates = np.sort(self._raw_x_coordinates)
        x_span = sorted_x_coordinates[-1] - sorted_x_coordinates[0]
        if x_span == 0:
            return None

        x_gaps = np.diff(sorted_x_coordinates)
        x_gap_idx = np.argmax(x_gaps)
        if x_gaps[x_gap_idx] <= self._U_PERIOD - x_span:
            return sorted_x_coordinates[0]
        return sorted_x_coordinates[x_gap_idx + 1]

    def project_points(self, points_3d: np.ndarray, apply_x_correction: bool = True) -> np.ndarray:
        """Project world-space points to this projection's 2D coordinates."""
        points = np.asarray(points_3d, dtype=np.float64)
        if points.ndim == 1:
            points = points[np.newaxis, :]

        relative_positions = points - self._sample_position
        x_coordinates, y_coordinates = self._calculate_2d_coordinates_from_relative_positions(relative_positions)

        if apply_x_correction and self._auto_seam is not None:
            x_coordinates = self._wrap_x(x_coordinates)

        return np.column_stack([x_coordinates, y_coordinates])

    def coordinate_for_detector(self, detector_index: int) -> tuple[float, float]:
        return (self._detector_x_coordinates[detector_index], self._detector_y_coordinates[detector_index])

    def positions(self) -> np.ndarray:
        return np.vstack([self._detector_x_coordinates, self._detector_y_coordinates]).transpose()

    # Overwritten in side-by-side
    def get_bank_groups_by_detector_id(self) -> list[tuple[list[int], str]]:
        return []


# Import subclasses at the BOTTOM to avoid circular imports,
# but ensure they're always registered when Projection is imported
from instrumentview.Projections.CylindricalProjection import CylindricalProjection  # noqa: F401 E402
from instrumentview.Projections.SphericalProjection import SphericalProjection  # noqa: F401 E402
from instrumentview.Projections.SideBySide import SideBySide  # noqa: F401 E402
