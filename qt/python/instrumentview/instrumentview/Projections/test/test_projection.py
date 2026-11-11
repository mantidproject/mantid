# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest.mock
from instrumentview.Projections.ProjectionType import ProjectionType
from instrumentview.Projections.Projection import Projection

import numpy as np
import unittest


class TestProjection(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root_position = np.array([0, 0, 0])
        cls.sample_position = np.array([0, 0, 0])
        cls.detector_positions = np.array([[0, 1, 0], [2, 1, 0], [-2, 1, 0]])

    def test_project_points_wraps_points_more_than_a_period_away(self):
        proj = Projection(
            type=ProjectionType.CYLINDRICAL_Y,
            sample_position=self.sample_position,
            root_position=self.root_position,
            detector_positions=self.detector_positions,
        )
        seam = proj._seam
        wrapped = proj._wrap_x(np.array([seam - 3 * np.pi / 2, seam + 7 * np.pi / 2]))
        self.assertTrue(np.all(wrapped >= seam))
        self.assertTrue(np.all(wrapped < seam + proj.u_period))
        np.testing.assert_allclose(wrapped, [seam + np.pi / 2, seam + 3 * np.pi / 2])

    @unittest.mock.patch("instrumentview.Projections.CylindricalProjection.CylindricalProjection._calculate_2d_coordinates")
    def test_find_auto_seam_multiple_similar_gaps(self, mock_calc_2d_coords):
        mock_calc_2d_coords.return_value = np.array([0, np.pi, -np.pi]), np.array([0, 0, 0])
        proj = Projection(
            type=ProjectionType.CYLINDRICAL_Z,
            sample_position=self.sample_position,
            root_position=self.root_position,
            detector_positions=self.detector_positions,
        )
        np.testing.assert_allclose(proj._seam, 0, atol=1e-9)
        np.testing.assert_allclose(proj._detector_x_coordinates, [0, np.pi, np.pi], rtol=1e-3)

    @unittest.mock.patch("instrumentview.Projections.CylindricalProjection.CylindricalProjection._calculate_2d_coordinates")
    def test_find_auto_seam_one_big_gap(self, mock_calc_2d_coords):
        mock_calc_2d_coords.return_value = np.array([0, np.pi / 2, -np.pi]), np.array([0, 0, 0])
        proj = Projection(
            type=ProjectionType.CYLINDRICAL_Z,
            sample_position=self.sample_position,
            root_position=self.root_position,
            detector_positions=self.detector_positions,
        )
        np.testing.assert_allclose(proj._seam, 0, atol=1e-9)
        np.testing.assert_allclose(proj._detector_x_coordinates, [0, np.pi / 2, np.pi], rtol=1e-3)

    @unittest.mock.patch("instrumentview.Projections.CylindricalProjection.CylindricalProjection._calculate_2d_coordinates")
    def test_find_auto_seam_no_gap_wide_enough(self, mock_calc_2d_coords):
        mock_calc_2d_coords.return_value = np.array([0, -np.pi / 4, np.pi / 2]), np.array([0, 0, 0])
        proj = Projection(
            type=ProjectionType.CYLINDRICAL_Z,
            sample_position=self.sample_position,
            root_position=self.root_position,
            detector_positions=self.detector_positions,
        )
        np.testing.assert_allclose(proj._seam, -np.pi / 4, atol=1e-9)
        np.testing.assert_allclose(proj._detector_x_coordinates, [0, -np.pi / 4, np.pi / 2], rtol=1e-3)

    @unittest.mock.patch("instrumentview.Projections.Projection.Projection._find_auto_seam", return_value=None)
    @unittest.mock.patch("instrumentview.Projections.CylindricalProjection.CylindricalProjection._calculate_2d_coordinates")
    def test_calculate_detector_coordinates(self, mock_calc_2d_coords, mock_find_auto_seam):
        mock_calc_2d_coords.return_value = (np.arange(5).astype(float), np.arange(1, 6).astype(float))
        proj = Projection(
            type=ProjectionType.CYLINDRICAL_Z,
            sample_position=self.sample_position,
            root_position=self.root_position,
            detector_positions=self.detector_positions,
        )
        np.testing.assert_array_equal(proj._raw_x_coordinates, np.arange(5))
        np.testing.assert_array_equal(proj._y_range, np.array([1, 5]))

    @unittest.mock.patch("instrumentview.Projections.Projection.Projection._find_auto_seam", return_value=None)
    @unittest.mock.patch("instrumentview.Projections.CylindricalProjection.CylindricalProjection._calculate_2d_coordinates")
    def test_coordinate_for_detector(self, mock_calc_2d_coords, mock_find_auto_seam):
        mock_calc_2d_coords.return_value = (np.arange(5).astype(float), np.arange(1, 6).astype(float))
        proj = Projection(
            type=ProjectionType.CYLINDRICAL_Z,
            sample_position=self.sample_position,
            root_position=self.root_position,
            detector_positions=self.detector_positions,
        )
        np.testing.assert_array_equal(proj.coordinate_for_detector(0), [0, 1])
        np.testing.assert_array_equal(proj.coordinate_for_detector(4), [4, 5])

    @unittest.mock.patch("instrumentview.Projections.Projection.Projection._find_auto_seam", return_value=None)
    @unittest.mock.patch("instrumentview.Projections.CylindricalProjection.CylindricalProjection._calculate_2d_coordinates")
    def test_positions(self, mock_calc_2d_coords, mock_find_auto_seam):
        mock_calc_2d_coords.return_value = (np.arange(5).astype(float), np.arange(1, 6).astype(float))
        proj = Projection(
            type=ProjectionType.CYLINDRICAL_Z,
            sample_position=self.sample_position,
            root_position=self.root_position,
            detector_positions=self.detector_positions,
        )
        np.testing.assert_array_equal(proj.positions(), np.vstack([np.arange(5), np.arange(1, 6)]).T)

    def test_project_points_matches_positions_for_detector_centers(self):
        proj = Projection(
            type=ProjectionType.CYLINDRICAL_Z,
            sample_position=self.sample_position,
            root_position=self.root_position,
            detector_positions=self.detector_positions,
        )
        np.testing.assert_allclose(proj.project_points(self.detector_positions), proj.positions())

    def _projection_with_raw_x(self, raw_x):
        """Build a cylindrical projection whose raw x coordinates are exactly the given values."""
        patcher = unittest.mock.patch(
            "instrumentview.Projections.CylindricalProjection.CylindricalProjection._calculate_2d_coordinates",
            return_value=(np.asarray(raw_x, dtype=float), np.zeros(len(raw_x))),
        )
        patcher.start()
        self.addCleanup(patcher.stop)
        return Projection(
            type=ProjectionType.CYLINDRICAL_Z,
            sample_position=self.sample_position,
            root_position=self.root_position,
            detector_positions=self.detector_positions,
        )

    def test_u_offset_defaults_to_zero_and_keeps_automatic_range(self):
        proj = self._projection_with_raw_x([0, np.pi / 8, -np.pi / 4])
        self.assertEqual(proj.u_offset, 0.0)
        np.testing.assert_allclose(proj._seam, -np.pi / 4, rtol=1e-3)
        np.testing.assert_allclose(proj._detector_x_coordinates, [0, np.pi / 8, -np.pi / 4], rtol=1e-3)

    def test_set_u_offset_moves_seam(self):
        proj = self._projection_with_raw_x([0, np.pi / 8, -np.pi / 4])
        # Move the seam onto x = 0, so the detector below it wraps around to the far side
        proj.set_u_offset(np.pi / 4)
        self.assertEqual(proj.u_offset, np.pi / 4)
        np.testing.assert_allclose(proj._seam, 0, atol=1e-9)
        np.testing.assert_allclose(proj._detector_x_coordinates, [0, np.pi / 8, 2 * np.pi - np.pi / 4], rtol=1e-3)

    def test_set_u_offset_only_shifts_by_whole_periods(self):
        raw_x = [0, np.pi / 8, -np.pi / 4]
        proj = self._projection_with_raw_x(raw_x)
        proj.set_u_offset(np.pi / 4)
        shifts = (proj._detector_x_coordinates - np.array(raw_x)) / proj.u_period
        np.testing.assert_allclose(shifts, np.round(shifts), atol=1e-9)

    def test_set_u_offset_back_to_zero_restores_automatic_range(self):
        proj = self._projection_with_raw_x([0, np.pi / 8, -np.pi / 4])
        proj.set_u_offset(np.pi / 4)
        proj.set_u_offset(0)
        np.testing.assert_allclose(proj._seam, -np.pi / 4, rtol=1e-3)
        np.testing.assert_allclose(proj._detector_x_coordinates, [0, np.pi / 8, -np.pi / 4], rtol=1e-3)

    def test_set_u_offset_of_one_period_is_the_same_as_no_rotation(self):
        raw_x = [0, np.pi / 8, -np.pi / 4]
        proj = self._projection_with_raw_x(raw_x)
        proj.set_u_offset(proj.u_period)
        self.assertEqual(proj.u_offset, 0.0)
        np.testing.assert_allclose(proj._seam, -np.pi / 4, rtol=1e-3)
        np.testing.assert_allclose(proj._detector_x_coordinates, raw_x, rtol=1e-3)

    def test_set_u_offset_wraps_past_one_period(self):
        proj = self._projection_with_raw_x([0, np.pi / 8, -np.pi / 4])
        proj.set_u_offset(proj.u_period + np.pi / 4)
        quarter_turn = self._projection_with_raw_x([0, np.pi / 8, -np.pi / 4])
        quarter_turn.set_u_offset(np.pi / 4)
        self.assertEqual(proj.u_offset, quarter_turn.u_offset)
        np.testing.assert_allclose(proj._detector_x_coordinates, quarter_turn._detector_x_coordinates, rtol=1e-3)

    def test_project_points_follows_u_offset(self):
        proj = Projection(
            type=ProjectionType.CYLINDRICAL_Z,
            sample_position=self.sample_position,
            root_position=self.root_position,
            detector_positions=self.detector_positions,
        )
        proj.set_u_offset(np.pi / 4)
        np.testing.assert_allclose(proj.project_points(self.detector_positions), proj.positions())
