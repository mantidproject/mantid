# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest.mock
from instrumentview.Projections.cylindrical_projection import cylindrical_projection
import unittest
from unittest.mock import MagicMock
import numpy as np


class TestCylindricalProjection(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mock_workspace = MagicMock()
        cls.mock_componentInfo = MagicMock()
        cls.mock_componentInfo.samplePosition.return_value = np.array([0, 0, 0])
        cls.mock_componentInfo.position = cls.mock_position
        cls.mock_componentInfo.hasValidShape.return_value = True
        cls.mock_workspace.componentInfo.return_value = cls.mock_componentInfo
        cls.detector_indices = [0, 1, 2]
        cls.abs_tol = 1e-9

    def mock_position(index: int):
        if index == 0:
            return [0, 1, 0]
        if index == 1:
            return [3, 1, 0]
        if index == 2:
            return [-3, 1, 0]

        raise ValueError(f"Unexpected component index: {index}")

    def test_calculate_2d_coordinates_x(self):
        # x-axis projection
        # No need to mock out the x adjustments because in this case there isn't any need for
        # the projection class to call it.
        self._run_test(
            projection_axis=[1, 0, 0],
            expected_x_axis=[0, 1, 0],
            expected_y_axis=[0, 0, 1],
            expected_projections=[(0, 0), (0, 3 / np.sqrt(10)), (0, -3 / np.sqrt(10))],
        )

    @unittest.mock.patch("instrumentview.Projections.cylindrical_projection.cylindrical_projection._apply_x_correction")
    def test_calculate_2d_coordinates_y(self, mock_apply_x_correction):
        # y-axis projection
        self._run_test(
            projection_axis=[0, 1, 0],
            expected_x_axis=[0, 0, 1],
            expected_y_axis=[1, 0, 0],
            expected_projections=[(0, 1), (-np.pi / 2, 1 / np.sqrt(10)), (np.pi / 2, 1 / np.sqrt(10))],
            mock_apply_x_correction=mock_apply_x_correction,
        )

    @unittest.mock.patch("instrumentview.Projections.cylindrical_projection.cylindrical_projection._apply_x_correction")
    def test_calculate_2d_coordinates_z(self, mock_apply_x_correction):
        # z-axis projection
        self._run_test(
            projection_axis=[0, 0, 1],
            expected_x_axis=[1, 0, 0],
            expected_y_axis=[0, 1, 0],
            expected_projections=[(-np.pi / 2, 0), (-np.atan2(1, 3), 0), (-np.atan2(1, -3), 0)],
            mock_apply_x_correction=mock_apply_x_correction,
        )

    def _run_test(self, projection_axis, expected_x_axis, expected_y_axis, expected_projections, mock_apply_x_correction=None):
        cylinder = cylindrical_projection(self.mock_workspace, self.detector_indices, projection_axis)
        np.testing.assert_allclose(cylinder._projection_axis, projection_axis, atol=self.abs_tol)
        np.testing.assert_allclose(cylinder._x_axis, expected_x_axis, atol=self.abs_tol)
        np.testing.assert_allclose(cylinder._y_axis, expected_y_axis, atol=self.abs_tol)
        for i in self.detector_indices:
            expected = expected_projections[i]
            calculated = cylinder.coordinate_for_detector(i)
            np.testing.assert_allclose(calculated, expected, atol=self.abs_tol)
        if mock_apply_x_correction is not None:
            self.assertEqual(mock_apply_x_correction.call_count, len(self.detector_indices))
