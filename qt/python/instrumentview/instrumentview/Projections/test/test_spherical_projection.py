# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest.mock
from instrumentview.Projections.SphericalProjection import SphericalProjection
import unittest
import numpy as np


class TestSphericalProjection(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.abs_tol = 1e-9
        r = 2.0
        cls.root_position = np.array([0, 0, 0])
        cls.sample_position = np.array([0, 0, 0])
        cls.detector_positions = np.array([[0, r, 0], [0, 0, r], [r, 0, 0]])

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
        sphere = SphericalProjection(self.sample_position, self.root_position, self.detector_positions, np.array(projection_axis))
        np.testing.assert_allclose(sphere._projection_axis, projection_axis, atol=self.abs_tol)
        np.testing.assert_allclose(sphere._x_axis, expected_x_axis, atol=self.abs_tol)
        np.testing.assert_allclose(sphere._y_axis, expected_y_axis, atol=self.abs_tol)
        calculated = sphere.positions()
        np.testing.assert_allclose(calculated, np.array(expected_projections))
