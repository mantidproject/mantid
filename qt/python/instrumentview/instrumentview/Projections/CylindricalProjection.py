# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Projections.Projection import Projection
import numpy as np


class CylindricalProjection(Projection):
    """2D projection with a cylindrical coordinate system, see https://en.wikipedia.org/wiki/Cylindrical_coordinate_system"""

    def _calculate_2d_coordinates(self, detector_positions: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
        detector_relative_positions = detector_positions - self._sample_position
        z = detector_relative_positions.dot(self._projection_axis)
        x = detector_relative_positions.dot(self._x_axis)
        y = detector_relative_positions.dot(self._y_axis)

        v_scale = 1.0 / np.sqrt(x * x + y * y + z * z)

        v = z * v_scale
        u = -np.atan2(y, x)

        # use equal area cylindrical projection with v = sin(latitude), u = longitude
        return (u, v)
