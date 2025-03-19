# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest.mock
from instrumentview.Projections.spherical_projection import spherical_projection
import unittest
from unittest.mock import MagicMock
import numpy as np


class TestSphericalProjection(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mock_workspace = MagicMock()
        cls.mock_componentInfo = MagicMock()
        cls.mock_componentInfo.samplePosition.return_value = np.array([0, 0, 0])
        cls.radius = 2.0
        cls.mock_componentInfo.position = lambda x: cls.mock_position(x, cls.radius)
        cls.mock_componentInfo.hasValidShape.return_value = True
        cls.mock_workspace.componentInfo.return_value = cls.mock_componentInfo
        cls.detector_indices = [0, 1, 2]
        cls.abs_tol = 1e-9

    def mock_position(index: int, radius: float):
        # All points on a sphere
        if index == 0:
            return [0, radius, 0]
        if index == 1:
            return [0, 0, radius]
        if index == 2:
            return [radius, 0, 0]

        raise ValueError(f"Unexpected component index: {index}")

    def test_calculate_2d_coordinates_x(self):
        # x-axis projection
        self._run_test(
            projection_axis=[1, 0, 0],
            expected_x_axis=[0, 1, 0],
            expected_y_axis=[0, 0, 1],
            expected_projections=[(0, -np.pi / 2), (-np.pi / 2, -np.pi / 2), (0, 0)],
        )

    def test_calculate_2d_coordinates_y(self):
        # y-axis projection
        self._run_test(
            projection_axis=[0, 1, 0],
            expected_x_axis=[0, 0, 1],
            expected_y_axis=[1, 0, 0],
            expected_projections=[(0, 0), (0, -np.pi / 2), (-np.pi / 2, -np.pi / 2)],
        )

    def test_calculate_2d_coordinates_z(self):
        # z-axis projection
        self._run_test(
            projection_axis=[0, 0, 1],
            expected_x_axis=[1, 0, 0],
            expected_y_axis=[0, 1, 0],
            expected_projections=[(-np.pi / 2, -np.pi / 2), (0, 0), (0, -np.pi / 2)],
        )

    def _run_test(self, projection_axis, expected_x_axis, expected_y_axis, expected_projections):
        sphere = spherical_projection(self.mock_workspace, self.detector_indices, projection_axis)
        np.testing.assert_allclose(sphere._projection_axis, projection_axis, atol=self.abs_tol)
        np.testing.assert_allclose(sphere._x_axis, expected_x_axis, atol=self.abs_tol)
        np.testing.assert_allclose(sphere._y_axis, expected_y_axis, atol=self.abs_tol)
        for i in self.detector_indices:
            expected = expected_projections[i]
            calculated = sphere.coordinate_for_detector(i)
            np.testing.assert_allclose(calculated, expected, atol=self.abs_tol)
