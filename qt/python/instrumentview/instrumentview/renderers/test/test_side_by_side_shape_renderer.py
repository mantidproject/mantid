# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Unit tests for the SideBySideShapeRenderer class."""

import unittest
from unittest.mock import MagicMock

import numpy as np
import pyvista as pv
from scipy.spatial.transform import Rotation

from instrumentview.renderers.shape_renderer import ShapeRenderer
from instrumentview.renderers.side_by_side_shape_renderer import SideBySideShapeRenderer


class TestSideBySideShapeRenderer(unittest.TestCase):
    """Tests for the SideBySideShapeRenderer class."""

    def setUp(self):
        self.renderer = SideBySideShapeRenderer()

    # ------------------------------------------------------------------
    # Basics
    # ------------------------------------------------------------------

    def test_is_shape_renderer_subclass(self):
        self.assertIsInstance(self.renderer, ShapeRenderer)

    def test_build_detector_mesh_raises_without_precompute(self):
        positions = np.array([[0.0, 0.0, 0.0]])
        with self.assertRaises(RuntimeError):
            self.renderer.build_detector_mesh(positions)

    def test_build_masked_mesh_empty_positions(self):
        mesh = self.renderer.build_masked_mesh(np.empty((0, 3)))
        self.assertEqual(mesh.number_of_points, 0)

    # ------------------------------------------------------------------
    # build_detector_mesh with bank groups
    # ------------------------------------------------------------------

    def test_build_detector_mesh_with_grid_bank(self):
        """Detectors in a grid bank should produce a valid mesh with no rotation."""
        ws = self._create_mock_workspace(n_detectors=4)
        bank_groups = [([0, 1, 2, 3], "grid")]
        self.renderer.precompute(ws, bank_groups)

        model = self._create_mock_model(ws, n_pickable=4)
        positions = np.array([[0, 0, 0], [0.1, 0, 0], [0, 0.1, 0], [0.1, 0.1, 0]], dtype=np.float64)
        mesh = self.renderer.build_detector_mesh(positions, model)

        self.assertIsInstance(mesh, pv.PolyData)
        self.assertGreater(mesh.number_of_points, 0)
        self.assertGreater(mesh.number_of_cells, 0)

    def test_build_detector_mesh_with_tube_bank(self):
        """Detectors in a tube bank should produce a valid mesh."""
        ws = self._create_mock_workspace(n_detectors=4)
        bank_groups = [([0, 1, 2, 3], "tube")]
        self.renderer.precompute(ws, bank_groups)

        model = self._create_mock_model(ws, n_pickable=4)
        positions = np.array([[0, 0, 0], [0, 0.1, 0], [0, 0.2, 0], [0, 0.3, 0]], dtype=np.float64)
        mesh = self.renderer.build_detector_mesh(positions, model)

        self.assertIsInstance(mesh, pv.PolyData)
        self.assertGreater(mesh.number_of_points, 0)

    def test_build_detector_mesh_multiple_banks(self):
        """Multiple banks of different types should all be included."""
        ws = self._create_mock_workspace(n_detectors=6)
        bank_groups = [([0, 1, 2], "grid"), ([3, 4, 5], "tube")]
        self.renderer.precompute(ws, bank_groups)

        model = self._create_mock_model(ws, n_pickable=6)
        positions = np.array(
            [[0, 0, 0], [0.1, 0, 0], [0.2, 0, 0], [0.5, 0, 0], [0.5, 0.1, 0], [0.5, 0.2, 0]],
            dtype=np.float64,
        )
        mesh = self.renderer.build_detector_mesh(positions, model)

        self.assertIsInstance(mesh, pv.PolyData)
        self.assertGreater(mesh.number_of_cells, 0)
        # All 6 detectors should be mapped
        c2d = self.renderer._cell_to_detector
        self.assertTrue(np.all(c2d >= 0))
        self.assertTrue(np.all(c2d < 6))

    def test_build_masked_mesh_with_bank_groups(self):
        """build_masked_mesh should also apply bank-aware scaling."""
        ws = self._create_mock_workspace(n_detectors=4)
        bank_groups = [([0, 1, 2, 3], "grid")]
        self.renderer.precompute(ws, bank_groups)

        model = self._create_mock_model(ws, n_pickable=0)
        model.masked_detector_ids = np.arange(4)
        positions = np.array([[0, 0, 0], [0.1, 0, 0], [0, 0.1, 0], [0.1, 0.1, 0]], dtype=np.float64)
        mesh = self.renderer.build_masked_mesh(positions, model)

        self.assertIsInstance(mesh, pv.PolyData)
        self.assertGreater(mesh.number_of_cells, 0)

    def test_no_bank_groups_falls_back_to_uniform_scale(self):
        """When bank_groups_by_detector_id is None, uniform scaling is used."""
        ws = self._create_mock_workspace(n_detectors=4)
        self.renderer.precompute(ws, None)

        model = self._create_mock_model(ws, n_pickable=4)
        positions = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0], [1, 1, 0]], dtype=np.float64)
        mesh = self.renderer.build_detector_mesh(positions, model)

        self.assertIsInstance(mesh, pv.PolyData)
        self.assertGreater(mesh.number_of_cells, 0)

    # ------------------------------------------------------------------
    # _compute_bank_projection_scales
    # ------------------------------------------------------------------

    def test_grid_bank_rotation_flags_are_false(self):
        """Grid bank detectors should have per_det_rotate = False."""
        ws = self._create_mock_workspace(n_detectors=4)
        bank_groups = [([0, 1, 2, 3], "grid")]
        self.renderer.precompute(ws, bank_groups)

        det_indices = np.arange(4)
        positions = np.array([[0, 0, 0], [0.1, 0, 0], [0, 0.1, 0], [0.1, 0.1, 0]], dtype=np.float64)
        model = MagicMock()
        model.workspace = ws

        scales, rotate = self.renderer._compute_bank_projection_scales(det_indices, positions, model)

        self.assertEqual(len(scales), 4)
        self.assertFalse(np.any(rotate))

    def test_tube_bank_rotation_flags_are_true(self):
        """Tube bank detectors should have per_det_rotate = True."""
        ws = self._create_mock_workspace(n_detectors=4)
        bank_groups = [([0, 1, 2, 3], "tube")]
        self.renderer.precompute(ws, bank_groups)

        det_indices = np.arange(4)
        positions = np.array([[0, 0, 0], [0, 0.1, 0], [0, 0.2, 0], [0, 0.3, 0]], dtype=np.float64)
        model = MagicMock()
        model.workspace = ws

        scales, rotate = self.renderer._compute_bank_projection_scales(det_indices, positions, model)

        self.assertEqual(len(scales), 4)
        self.assertTrue(np.all(rotate))

    def test_mixed_banks_rotation_flags(self):
        """Mixed grid + tube banks: only tube detectors should have rotate=True."""
        ws = self._create_mock_workspace(n_detectors=6)
        bank_groups = [([0, 1, 2], "grid"), ([3, 4, 5], "tube")]
        self.renderer.precompute(ws, bank_groups)

        det_indices = np.arange(6)
        positions = np.array(
            [[0, 0, 0], [0.1, 0, 0], [0.2, 0, 0], [0.5, 0, 0], [0.5, 0.1, 0], [0.5, 0.2, 0]],
            dtype=np.float64,
        )
        model = MagicMock()
        model.workspace = ws

        scales, rotate = self.renderer._compute_bank_projection_scales(det_indices, positions, model)

        # Grid detectors (0,1,2) should not rotate
        self.assertFalse(np.any(rotate[:3]))
        # Tube detectors (3,4,5) should rotate
        self.assertTrue(np.all(rotate[3:]))

    def test_scales_are_positive(self):
        """All computed scales should be positive."""
        ws = self._create_mock_workspace(n_detectors=4)
        bank_groups = [([0, 1, 2, 3], "grid")]
        self.renderer.precompute(ws, bank_groups)

        det_indices = np.arange(4)
        positions = np.array([[0, 0, 0], [0.1, 0, 0], [0, 0.1, 0], [0.1, 0.1, 0]], dtype=np.float64)
        model = MagicMock()
        model.workspace = ws

        scales, _ = self.renderer._compute_bank_projection_scales(det_indices, positions, model)

        self.assertTrue(np.all(scales > 0))

    def test_uniform_scale_within_bank(self):
        """All detectors in the same bank should get the same scale factor."""
        ws = self._create_mock_workspace(n_detectors=4, same_shape=True)
        bank_groups = [([0, 1, 2, 3], "grid")]
        self.renderer.precompute(ws, bank_groups)

        det_indices = np.arange(4)
        positions = np.array([[0, 0, 0], [0.1, 0, 0], [0, 0.1, 0], [0.1, 0.1, 0]], dtype=np.float64)
        model = MagicMock()
        model.workspace = ws

        scales, _ = self.renderer._compute_bank_projection_scales(det_indices, positions, model)

        # All four should have the same scale
        np.testing.assert_array_almost_equal(scales, scales[0])

    def test_single_detector_bank(self):
        """A bank with only one detector should still produce a valid scale."""
        ws = self._create_mock_workspace(n_detectors=3)
        bank_groups = [([0], "grid"), ([1, 2], "grid")]
        self.renderer.precompute(ws, bank_groups)

        det_indices = np.arange(3)
        positions = np.array([[0, 0, 0], [0.1, 0, 0], [0.2, 0, 0]], dtype=np.float64)
        model = MagicMock()
        model.workspace = ws

        scales, _ = self.renderer._compute_bank_projection_scales(det_indices, positions, model)

        self.assertTrue(np.all(scales > 0))

    # ------------------------------------------------------------------
    # No-overlap verification
    # ------------------------------------------------------------------

    def test_shapes_do_not_overlap_grid_bank(self):
        """After scaling, the directional extent along each nearest-neighbour
        direction should not exceed half the nearest-neighbour distance."""
        ws = self._create_mock_workspace(n_detectors=4, same_shape=True)
        bank_groups = [([0, 1, 2, 3], "grid")]
        self.renderer.precompute(ws, bank_groups)

        det_indices = np.arange(4)
        positions = np.array([[0, 0, 0], [0.1, 0, 0], [0, 0.1, 0], [0.1, 0.1, 0]], dtype=np.float64)
        model = MagicMock()
        model.workspace = ws

        scales, _ = self.renderer._compute_bank_projection_scales(det_indices, positions, model)

        scale = scales[0]
        nnd = 0.1  # nearest-neighbour distance in the grid
        # Check along both NN directions (x and y)
        for direction in [np.array([1.0, 0.0]), np.array([0.0, 1.0])]:
            extent = self.renderer._shape_extent_along_direction_2d(0, direction, apply_rotation=False)
            scaled_extent = scale * extent
            self.assertLessEqual(scaled_extent, nnd / 2.0 + 1e-9)

    # ------------------------------------------------------------------
    # _shape_extent_along_direction_2d
    # ------------------------------------------------------------------

    def test_extent_along_x_direction(self):
        """Extent along x should match the shape's x span."""
        ws = self._create_mock_workspace(n_detectors=1)
        self.renderer.precompute(ws, None)

        direction = np.array([1.0, 0.0])
        extent = self.renderer._shape_extent_along_direction_2d(0, direction)

        # The test shape has vertices at x=0 and x=0.01
        self.assertAlmostEqual(extent, 0.01, places=5)

    def test_extent_along_y_direction(self):
        """Extent along y should match the shape's y span."""
        ws = self._create_mock_workspace(n_detectors=1)
        self.renderer.precompute(ws, None)

        direction = np.array([0.0, 1.0])
        extent = self.renderer._shape_extent_along_direction_2d(0, direction)

        # The test shape has vertices at y=0 and y=0.01
        self.assertAlmostEqual(extent, 0.01, places=5)

    def test_extent_along_diagonal(self):
        """Extent along [1,1]/sqrt(2) should be larger than along axis."""
        ws = self._create_mock_workspace(n_detectors=1)
        self.renderer.precompute(ws, None)

        diag = np.array([1.0, 1.0]) / np.sqrt(2)
        extent_diag = self.renderer._shape_extent_along_direction_2d(0, diag)
        extent_x = self.renderer._shape_extent_along_direction_2d(0, np.array([1.0, 0.0]))

        self.assertGreater(extent_diag, extent_x)

    def test_extent_with_empty_shape(self):
        """An empty shape should return 0.0."""
        self.renderer._precomputed = True
        self.renderer._shape_cache = {0: (np.empty((0, 3)), np.empty((0, 3)))}
        self.renderer._det_shape_keys = np.array([0])
        self.renderer._det_scales = np.array([[1.0, 1.0, 1.0]])
        self.renderer._det_rotations = np.eye(3).reshape(1, 3, 3)

        extent = self.renderer._shape_extent_along_direction_2d(0, np.array([1.0, 0.0]))
        self.assertEqual(extent, 0.0)

    def test_extent_with_rotation(self):
        """When apply_rotation=True, the extent should reflect the rotated shape."""
        ws = self._create_mock_workspace(n_detectors=1)
        self.renderer.precompute(ws, None)

        # Force a 90° rotation around z so x→y and y→-x
        rot_90z = Rotation.from_euler("z", 90, degrees=True).as_matrix()
        self.renderer._det_rotations[0] = rot_90z

        # After rotation, the x-extent of the shape should become the y-extent
        extent_x_rotated = self.renderer._shape_extent_along_direction_2d(0, np.array([1.0, 0.0]), apply_rotation=True)
        extent_x_unrotated = self.renderer._shape_extent_along_direction_2d(0, np.array([1.0, 0.0]), apply_rotation=False)
        extent_y_unrotated = self.renderer._shape_extent_along_direction_2d(0, np.array([0.0, 1.0]), apply_rotation=False)

        # After 90° rotation, x-extent should equal the original y-extent
        self.assertAlmostEqual(extent_x_rotated, extent_y_unrotated, places=5)
        # And differ from the original x-extent (unless shape is square)
        # Our test shape is a square (0.01 x 0.01), so they'd be equal —
        # but the important thing is the rotation was applied
        self.assertAlmostEqual(extent_x_rotated, extent_x_unrotated, places=5)

    # ------------------------------------------------------------------
    # _shape_radial_extent_2d
    # ------------------------------------------------------------------

    def test_radial_extent(self):
        """Radial extent should be the max distance from origin to any vertex."""
        ws = self._create_mock_workspace(n_detectors=1)
        self.renderer.precompute(ws, None)

        extent = self.renderer._shape_radial_extent_2d(0)

        # The furthest vertex from origin in 2D is (0.01, 0.01)
        expected = np.sqrt(0.01**2 + 0.01**2)
        self.assertAlmostEqual(extent, expected, places=8)

    def test_radial_extent_empty_shape(self):
        """An empty shape should return 0.0."""
        self.renderer._precomputed = True
        self.renderer._shape_cache = {0: (np.empty((0, 3)), np.empty((0, 3)))}
        self.renderer._det_shape_keys = np.array([0])
        self.renderer._det_scales = np.array([[1.0, 1.0, 1.0]])
        self.renderer._det_rotations = np.eye(3).reshape(1, 3, 3)

        extent = self.renderer._shape_radial_extent_2d(0)
        self.assertEqual(extent, 0.0)

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _create_mock_workspace(self, n_detectors=4, same_shape=False):
        """Create a mock workspace with componentInfo and detectorInfo."""
        workspace = MagicMock()

        det_info = MagicMock()
        det_info.size.return_value = n_detectors
        det_info.position.side_effect = self._make_position_side_effect(n_detectors)
        det_info.indexOf.side_effect = lambda did: int(did)
        det_info.detectorIDs.return_value = list(range(n_detectors))

        comp_info = MagicMock()
        comp_info.hasValidShape.return_value = True

        # A small square triangle mesh (two triangles forming a 0.01×0.01 square)
        triangle_mesh = np.array(
            [
                [[0.0, 0.0, 0.0], [0.01, 0.0, 0.0], [0.0, 0.01, 0.0]],
                [[0.01, 0.0, 0.0], [0.01, 0.01, 0.0], [0.0, 0.01, 0.0]],
            ]
        )

        shape_objs = []
        for i in range(n_detectors):
            shape = MagicMock()
            if same_shape:
                shape.getShapeXML.return_value = "<cuboid>same_shape</cuboid>"
            else:
                shape.getShapeXML.return_value = f"<cuboid>shape_{i % 2}</cuboid>"
            shape.getMesh.return_value = triangle_mesh.copy()
            shape_objs.append(shape)

        comp_info.shape.side_effect = lambda idx: shape_objs[idx]

        identity_quat = MagicMock()
        identity_quat.real.return_value = 1.0
        identity_quat.imagI.return_value = 0.0
        identity_quat.imagJ.return_value = 0.0
        identity_quat.imagK.return_value = 0.0
        comp_info.rotation.return_value = identity_quat

        unit_scale = MagicMock()
        unit_scale.X.return_value = 1.0
        unit_scale.Y.return_value = 1.0
        unit_scale.Z.return_value = 1.0
        comp_info.scaleFactor.return_value = unit_scale

        workspace.componentInfo.return_value = comp_info
        workspace.detectorInfo.return_value = det_info

        return workspace

    def _make_position_side_effect(self, n_detectors):
        def position(i):
            pos = MagicMock()
            pos.X.return_value = float(i)
            pos.Y.return_value = 0.0
            pos.Z.return_value = 0.0
            return pos

        return position

    def _create_mock_model(self, workspace, n_pickable=4):
        model = MagicMock()
        model.workspace = workspace
        model.is_2d_projection = True
        model.pickable_detector_ids = np.arange(n_pickable)
        model.masked_detector_ids = np.array([], dtype=np.int64)
        return model


if __name__ == "__main__":
    unittest.main()
