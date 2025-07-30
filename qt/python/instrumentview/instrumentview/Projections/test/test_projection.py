# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest.mock
from instrumentview.Projections.CylindricalProjection import CylindricalProjection

import numpy as np
import unittest


class TestProjection(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root_position = np.array([0, 0, 0])
        cls.sample_position = np.array([0, 0, 0])
        cls.detector_positions = np.array([[0, 1, 0], [2, 1, 0], [-2, 1, 0]])

    def test_apply_x_correction_below_min(self):
        proj = CylindricalProjection(self.sample_position, self.root_position, self.detector_positions, np.array([0, 1, 0]))
        x_min = proj._x_range[0]
        x_max = proj._x_range[1]
        proj._detector_x_coordinates[0] = x_min - 3 * np.pi / 2
        self.assertLess(proj._detector_x_coordinates[0], x_min)
        proj._apply_x_correction()
        self.assertGreaterEqual(proj._detector_x_coordinates[0], x_min)
        self.assertLessEqual(proj._detector_x_coordinates[0], x_max)

    def test_apply_x_correction_above_max(self):
        proj = CylindricalProjection(self.sample_position, self.root_position, self.detector_positions, np.array([0, 1, 0]))
        x_min = proj._x_range[0]
        x_max = proj._x_range[1]
        proj._detector_x_coordinates[0] = x_max + 3 * np.pi / 2
        self.assertGreater(proj._detector_x_coordinates[0], x_max)
        proj._apply_x_correction()
        self.assertGreaterEqual(proj._detector_x_coordinates[0], x_min)
        self.assertLessEqual(proj._detector_x_coordinates[0], x_max)

    @unittest.mock.patch("instrumentview.Projections.CylindricalProjection.CylindricalProjection._calculate_2d_coordinates")
    def test_find_and_correct_x_gap_multiple_similar_gaps(self, mock_calc_2d_coords):
        mock_calc_2d_coords.return_value = np.array([0, np.pi / 4, -np.pi / 4]), np.array([0, 0, 0])
        proj = CylindricalProjection(self.sample_position, self.root_position, self.detector_positions, np.array([0, 0, 1]))
        np.testing.assert_allclose(proj._detector_x_coordinates, [0, np.pi / 4, -np.pi / 4], rtol=1e-3)
        np.testing.assert_allclose(proj._x_range, [-np.pi / 4, np.pi / 4], rtol=1e-3)
        proj._u_period = np.pi / 2
        proj._find_and_correct_x_gap()
        np.testing.assert_allclose(proj._detector_x_coordinates, [0, np.pi / 4, np.pi / 4], rtol=1e-3)
        np.testing.assert_allclose(proj._x_range, [0, np.pi / 4], rtol=1e-3)

    @unittest.mock.patch("instrumentview.Projections.CylindricalProjection.CylindricalProjection._calculate_2d_coordinates")
    def test_find_and_correct_x_gap_big_gap_first(self, mock_calc_2d_coords):
        mock_calc_2d_coords.return_value = np.array([0, np.pi / 8, -np.pi / 4]), np.array([0, 0, 0])
        proj = CylindricalProjection(self.sample_position, self.root_position, self.detector_positions, np.array([0, 0, 1]))
        np.testing.assert_allclose(proj._detector_x_coordinates, [0, np.pi / 8, -np.pi / 4], rtol=1e-3)
        np.testing.assert_allclose(proj._x_range, [-np.pi / 4, np.pi / 8], rtol=1e-3)
        proj._u_period = np.pi / 2
        proj._find_and_correct_x_gap()
        np.testing.assert_allclose(proj._x_range, [np.pi / 4, np.pi / 2], rtol=1e-3)
        np.testing.assert_allclose(proj._detector_x_coordinates, [np.pi / 2, np.pi / 8, np.pi / 4], rtol=1e-3)

    @unittest.mock.patch("instrumentview.Projections.CylindricalProjection.CylindricalProjection._calculate_2d_coordinates")
    def test_find_and_correct_x_gap_big_gap_last(self, mock_calc_2d_coords):
        mock_calc_2d_coords.return_value = np.array([0, -np.pi / 8, np.pi / 4]), np.array([0, 0, 0])
        proj = CylindricalProjection(self.sample_position, self.root_position, self.detector_positions, np.array([0, 0, 1]))
        np.testing.assert_allclose(proj._detector_x_coordinates, [0, np.pi / 8, -np.pi / 4], rtol=1e-3)
        np.testing.assert_allclose(proj._x_range, [-np.pi / 4, np.pi / 8], rtol=1e-3)
        proj._u_period = np.pi / 2
        proj._find_and_correct_x_gap()
        np.testing.assert_allclose(proj._x_range, [np.pi / 4, np.pi / 2], rtol=1e-3)
        np.testing.assert_allclose(proj._detector_x_coordinates, [np.pi / 2, 3 * np.pi / 8, np.pi / 4], rtol=1e-3)

    @unittest.mock.patch("instrumentview.Projections.Projection.Projection._find_and_correct_x_gap")
    @unittest.mock.patch("instrumentview.Projections.CylindricalProjection.CylindricalProjection._calculate_2d_coordinates")
    def test_calculate_detector_coordinates(self, mock_calc_2d_coords, mock_find_and_correct_x_gap):
        mock_calc_2d_coords.return_value = (np.arange(5).astype(float), np.arange(1, 6).astype(float))
        proj = CylindricalProjection(self.sample_position, self.root_position, self.detector_positions, np.array([0, 0, 1]))
        np.testing.assert_array_equal(proj._x_range, np.array([0, 4]))
        np.testing.assert_array_equal(proj._y_range, np.array([1, 5]))

    @unittest.mock.patch("instrumentview.Projections.Projection.Projection._find_and_correct_x_gap")
    @unittest.mock.patch("instrumentview.Projections.CylindricalProjection.CylindricalProjection._calculate_2d_coordinates")
    def test_coordinate_for_detector(self, mock_calc_2d_coords, mock_find_and_correct_x_gap):
        mock_calc_2d_coords.return_value = (np.arange(5).astype(float), np.arange(1, 6).astype(float))
        proj = CylindricalProjection(self.sample_position, self.root_position, self.detector_positions, np.array([0, 0, 1]))
        np.testing.assert_array_equal(proj.coordinate_for_detector(0), [0, 1])
        np.testing.assert_array_equal(proj.coordinate_for_detector(4), [4, 5])

    @unittest.mock.patch("instrumentview.Projections.Projection.Projection._find_and_correct_x_gap")
    @unittest.mock.patch("instrumentview.Projections.CylindricalProjection.CylindricalProjection._calculate_2d_coordinates")
    def test_positions(self, mock_calc_2d_coords, mock_find_and_correct_x_gap):
        mock_calc_2d_coords.return_value = (np.arange(5).astype(float), np.arange(1, 6).astype(float))
        proj = CylindricalProjection(self.sample_position, self.root_position, self.detector_positions, np.array([0, 0, 1]))
        np.testing.assert_array_equal(proj.positions(), np.vstack([np.arange(5), np.arange(1, 6)]).T)
