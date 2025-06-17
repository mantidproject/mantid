# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest.mock
from instrumentview.Projections.cylindrical_projection import cylindrical_projection

import numpy as np
import unittest


class TestProjection(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root_position = np.array([0, 0, 0])
        cls.sample_position = np.array([0, 0, 0])
        cls.detector_positions = np.array([[0, 1, 0], [2, 1, 0], [-2, 1, 0]])

    def test_apply_x_correction_below_min(self):
        proj = cylindrical_projection(self.sample_position, self.root_position, self.detector_positions, np.array([0, 1, 0]))
        x_min = proj._x_range[0]
        x_max = proj._x_range[1]
        proj._detector_x_coordinates[0] = x_min - np.pi / 2
        self.assertLess(proj._detector_x_coordinates[0], x_min)
        proj._apply_x_correction(0)
        self.assertGreaterEqual(proj._detector_x_coordinates[0], x_min)
        self.assertLessEqual(proj._detector_x_coordinates[0], x_max)

    def test_apply_x_correction_above_max(self):
        proj = cylindrical_projection(self.sample_position, self.root_position, self.detector_positions, np.array([0, 1, 0]))
        x_min = proj._x_range[0]
        x_max = proj._x_range[1]
        proj._detector_x_coordinates[0] = x_max + np.pi / 2
        self.assertGreater(proj._detector_x_coordinates[0], x_max)
        proj._apply_x_correction(0)
        self.assertGreaterEqual(proj._detector_x_coordinates[0], x_min)
        self.assertLessEqual(proj._detector_x_coordinates[0], x_max)

    def test_find_and_correct_x_gap(self):
        def mock_calculate_2d_coordinates(positions):
            return np.array([0, np.pi / 4, -np.pi / 4]), np.array([0, 0, 0])

        with unittest.mock.patch(
            "instrumentview.Projections.cylindrical_projection.cylindrical_projection._calculate_2d_coordinates",
            side_effect=mock_calculate_2d_coordinates,
        ):
            proj = cylindrical_projection(self.sample_position, self.root_position, self.detector_positions, np.array([0, 0, 1]))
            np.testing.assert_allclose(proj._detector_x_coordinates, [0, np.pi / 4, -np.pi / 4], rtol=1e-3)
            np.testing.assert_allclose(proj._x_range, [-np.pi / 4, np.pi / 4], rtol=1e-3)
            proj._u_period = np.pi / 2
            proj._find_and_correct_x_gap()
            np.testing.assert_allclose(proj._detector_x_coordinates, [np.pi / 2, np.pi / 4, np.pi / 4], rtol=1e-3)
            np.testing.assert_allclose(proj._x_range, [np.pi / 4, np.pi / 2], rtol=1e-3)
