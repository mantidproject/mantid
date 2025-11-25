# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Projections.Projection import Projection
import numpy as np


class SphericalProjection(Projection):
    """2D projection with a spherical coordinate system, see https://en.wikipedia.org/wiki/Spherical_coordinate_system"""

    def _calculate_2d_coordinates(self) -> tuple[np.ndarray, np.ndarray]:
        detector_relative_positions = self._detector_positions - self._sample_position
        v = detector_relative_positions.dot(self._projection_axis)
        x = detector_relative_positions.dot(self._x_axis)
        y = detector_relative_positions.dot(self._y_axis)

        r = np.sqrt(x * x + y * y + v * v)

        u = -np.atan2(y, x)
        vr = np.divide(v, r, out=np.zeros_like(v), where=r > 0)
        v = -np.acos(np.clip(vr, -1.0, 1.0))
        return (u, v)
