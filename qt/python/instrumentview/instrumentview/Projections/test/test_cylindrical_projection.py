# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest.mock
from instrumentview.Projections.ProjectionType import ProjectionType
from instrumentview.Projections.Projection import CylindricalProjection
import unittest
import numpy as np


class TestCylindricalProjection(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.abs_tol = 1e-9
        cls.root_position = np.array([0, 0, 0])
        cls.sample_position = np.array([0, 0, 0])
        cls.detector_positions = np.array([[0, 1, 0], [3, 1, 0], [-3, 1, 0]])

    def test_calculate_2d_coordinates_x(self):
        # x-axis projection
        # No need to mock out the x adjustments because in this case there isn't any need for
        # the projection class to call it.
        self._run_test(
            projection_type=ProjectionType.CYLINDRICAL_X,
            expected_x_axis=[0, 1, 0],
            expected_y_axis=[0, 0, 1],
            expected_projections=[(0, 0), (0, 3 / np.sqrt(10)), (0, -3 / np.sqrt(10))],
        )

    def test_calculate_2d_coordinates_y(self):
        # y-axis projection
        self._run_test(
            projection_type=ProjectionType.CYLINDRICAL_Y,
            expected_x_axis=[0, 0, 1],
            expected_y_axis=[1, 0, 0],
            expected_projections=[(0, 1), (-np.pi / 2, 1 / np.sqrt(10)), (np.pi / 2, 1 / np.sqrt(10))],
            mock_apply_x_correction=None,
        )

    def test_calculate_2d_coordinates_z(self):
        # z-axis projection
        self._run_test(
            projection_type=ProjectionType.CYLINDRICAL_Z,
            expected_x_axis=[1, 0, 0],
            expected_y_axis=[0, 1, 0],
            expected_projections=[(-np.pi / 2, 0), (-np.atan2(1, 3), 0), (-np.atan2(1, -3), 0)],
            mock_apply_x_correction=None,
        )

    def _run_test(self, projection_type, expected_x_axis, expected_y_axis, expected_projections, mock_apply_x_correction=None):
        cylinder = CylindricalProjection(
            type=projection_type,
            sample_position=self.sample_position,
            root_position=self.root_position,
            detector_positions=self.detector_positions,
        )
        np.testing.assert_allclose(cylinder._x_axis, expected_x_axis, atol=self.abs_tol)
        np.testing.assert_allclose(cylinder._y_axis, expected_y_axis, atol=self.abs_tol)
        calculated = cylinder.positions()
        np.testing.assert_allclose(calculated, np.array(expected_projections))
        if mock_apply_x_correction is not None:
            mock_apply_x_correction.assert_called_once()
