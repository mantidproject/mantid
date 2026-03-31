# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Unit tests for the PointCloudRenderer and ShapeRenderer classes."""

import unittest
from unittest.mock import MagicMock

import numpy as np
import pyvista as pv

from instrumentview.renderers.base_renderer import InstrumentRenderer
from instrumentview.renderers.point_cloud_renderer import PointCloudRenderer
from instrumentview.renderers.shape_renderer import ShapeRenderer, _triangles_to_verts_faces, _make_fallback_shape
from instrumentview.Projections.ProjectionType import ProjectionType


class TestPointCloudRenderer(unittest.TestCase):
    """Tests for the PointCloudRenderer (original rendering mode)."""

    def setUp(self):
        self.renderer = PointCloudRenderer()
        self.positions = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0], [7.0, 8.0, 9.0]])

    def test_is_instrument_renderer(self):
        self.assertIsInstance(self.renderer, InstrumentRenderer)

    def test_build_detector_mesh_returns_polydata(self):
        mesh = self.renderer.build_detector_mesh(self.positions, False)
        self.assertIsInstance(mesh, pv.PolyData)
        self.assertEqual(mesh.number_of_points, 3)

    def test_build_pickable_mesh_returns_polydata(self):
        mesh = self.renderer.build_pickable_mesh(self.positions, False)
        self.assertIsInstance(mesh, pv.PolyData)
        self.assertEqual(mesh.number_of_points, 3)

    def test_build_masked_mesh_returns_polydata(self):
        mesh = self.renderer.build_masked_mesh(self.positions, False)
        self.assertIsInstance(mesh, pv.PolyData)
        self.assertEqual(mesh.number_of_points, 3)

    def test_set_detector_scalars_sets_point_data(self):
        mesh = self.renderer.build_detector_mesh(self.positions, False)
        counts = np.array([10, 20, 30])
        self.renderer.set_detector_scalars(mesh, counts, "Counts")
        np.testing.assert_array_equal(mesh.point_data["Counts"], counts)

    def test_set_pickable_scalars_sets_point_data(self):
        mesh = self.renderer.build_pickable_mesh(self.positions, False)
        vis = np.array([0, 1, 0])
        self.renderer.set_pickable_scalars(mesh, vis, "Visible")
        np.testing.assert_array_equal(mesh.point_data["Visible"], vis)

    def test_add_detector_mesh_to_plotter_calls_add_mesh(self):
        plotter = MagicMock()
        plotter.off_screen = True
        mesh = self.renderer.build_detector_mesh(self.positions, False)
        self.renderer.add_detector_mesh_to_plotter(plotter, mesh, is_projection=False, scalars="Counts")
        plotter.add_mesh.assert_called_once()
        call_kwargs = plotter.add_mesh.call_args[1]
        self.assertTrue(call_kwargs["render_points_as_spheres"])
        self.assertEqual(call_kwargs["point_size"], 15)

    def test_add_pickable_mesh_to_plotter_calls_add_mesh(self):
        plotter = MagicMock()
        mesh = self.renderer.build_pickable_mesh(self.positions, False)
        self.renderer.add_pickable_mesh_to_plotter(plotter, mesh, scalars="Vis")
        plotter.add_mesh.assert_called_once()
        call_kwargs = plotter.add_mesh.call_args[1]
        self.assertTrue(call_kwargs["render_points_as_spheres"])
        self.assertTrue(call_kwargs["pickable"])

    def test_add_masked_mesh_empty_does_not_add(self):
        plotter = MagicMock()
        empty = pv.PolyData()
        self.renderer.add_masked_mesh_to_plotter(plotter, empty)
        plotter.add_mesh.assert_not_called()

    def test_add_masked_mesh_nonempty_adds(self):
        plotter = MagicMock()
        mesh = self.renderer.build_masked_mesh(self.positions, False)
        self.renderer.add_masked_mesh_to_plotter(plotter, mesh)
        plotter.add_mesh.assert_called_once()


class TestTrianglesToVertsFaces(unittest.TestCase):
    """Tests for the _triangles_to_verts_faces helper."""

    def test_single_triangle(self):
        raw = np.array([[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]]])
        verts, faces = _triangles_to_verts_faces(raw)
        self.assertEqual(verts.shape[0], 3)
        self.assertEqual(faces.shape, (1, 3))

    def test_shared_vertices_are_deduplicated(self):
        # Two triangles sharing an edge (2 shared vertices)
        raw = np.array(
            [
                [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
                [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [1.0, 1.0, 0.0]],
            ]
        )
        verts, faces = _triangles_to_verts_faces(raw)
        self.assertEqual(verts.shape[0], 4)  # 4 unique vertices
        self.assertEqual(faces.shape, (2, 3))

    def test_face_indices_valid(self):
        raw = np.array([[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]]])
        verts, faces = _triangles_to_verts_faces(raw)
        # Face indices must be in range [0, n_verts)
        self.assertTrue(np.all(faces >= 0))
        self.assertTrue(np.all(faces < len(verts)))

    def test_reconstructed_triangles_match_original(self):
        """Verify that verts[faces] reproduces the original triangle coordinates."""
        raw = np.array(
            [
                [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
                [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [1.0, 1.0, 0.0]],
            ]
        )
        verts, faces = _triangles_to_verts_faces(raw)
        reconstructed = verts[faces]  # (n_tri, 3, 3)
        np.testing.assert_allclose(reconstructed, raw)


class TestFallbackShape(unittest.TestCase):
    def test_returns_tetrahedron(self):
        verts, faces = _make_fallback_shape()
        self.assertEqual(verts.shape, (4, 3))
        self.assertEqual(faces.shape, (4, 3))


class TestShapeRenderer(unittest.TestCase):
    """Tests for the ShapeRenderer class."""

    def setUp(self):
        self._workspace = self._refresh_render_with_mock_workspace(n_detectors=4)

    def test_is_instrument_renderer(self):
        self.assertIsInstance(self.renderer, InstrumentRenderer)

    def test_build_detector_mesh_auto_precomputes(self):
        """build_detector_mesh should auto-precompute if not already done."""
        model = self._create_mock_model(self._workspace, n_pickable=1)
        positions = np.array([[0.0, 0.0, 0.0]])
        mesh = self.renderer.build_detector_mesh(positions, False, model)
        self.assertTrue(self.renderer._precomputed)
        self.assertIsInstance(mesh, pv.PolyData)

    def test_precompute_and_build_mesh(self):
        """Integration test: create a mock workspace with detectors that have shapes
        and verify mesh assembly produces valid geometry."""

        # Test precomputing before building a mesh
        self.assertTrue(self.renderer._precomputed)
        self.assertGreater(len(self.renderer._shape_cache), 0)

        model = self._create_mock_model(self._workspace, n_pickable=4)
        positions = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0], [1, 1, 0]], dtype=np.float64)
        mesh = self.renderer.build_detector_mesh(positions, False, model)

        self.assertIsInstance(mesh, pv.PolyData)
        self.assertGreater(mesh.number_of_points, 0)
        self.assertGreater(mesh.number_of_cells, 0)

    def test_build_pickable_mesh_returns_shape_copy(self):
        """build_pickable_mesh should return a shape mesh copy (not a point cloud)
        so that cell picking works on the full detector surface."""

        model = self._create_mock_model(self._workspace, n_pickable=4)
        positions = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0], [1, 1, 0]], dtype=np.float64)
        self.renderer.build_detector_mesh(positions, False, model)

        pickable = self.renderer.build_pickable_mesh(positions, False)
        self.assertIsInstance(pickable, pv.PolyData)
        # Must have cells (shape faces), not just points
        self.assertGreater(pickable.number_of_cells, 0)
        # Same cell count as the detector mesh
        self.assertEqual(pickable.number_of_cells, self.renderer._detector_mesh_ref.number_of_cells)

    def test_set_pickable_scalars_sets_cell_data(self):
        """Visibility should be set as cell data via _cell_to_detector."""
        workspace = self._refresh_render_with_mock_workspace(n_detectors=2)

        model = self._create_mock_model(workspace, n_pickable=2)
        positions = np.array([[0, 0, 0], [1, 0, 0]], dtype=np.float64)
        self.renderer.build_detector_mesh(positions, False, model)
        pickable = self.renderer.build_pickable_mesh(positions, False)

        vis = np.array([0, 1])
        self.renderer.set_pickable_scalars(pickable, vis, "Visible")

        cell_vis = pickable.cell_data["Visible"]
        c2d = self.renderer._cell_to_detector
        # Cells for detector 0 should have value 0, detector 1 should have value 1
        for det_idx, expected in enumerate(vis):
            np.testing.assert_array_equal(cell_vis[c2d == det_idx], expected)

    def test_cell_to_detector_mapping(self):
        """Verify that each cell in the assembled mesh maps to a valid detector index."""
        workspace = self._refresh_render_with_mock_workspace(n_detectors=3)

        model = self._create_mock_model(workspace, n_pickable=3)
        positions = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0]], dtype=np.float64)
        mesh = self.renderer.build_detector_mesh(positions, False, model)

        c2d = self.renderer._cell_to_detector
        self.assertIsNotNone(c2d)
        self.assertEqual(len(c2d), mesh.number_of_cells)
        # All mapped indices must be in {0, 1, 2}
        self.assertTrue(np.all(c2d >= 0))
        self.assertTrue(np.all(c2d < 3))

    def test_set_detector_scalars_cell_data(self):
        """Scalars should be repeated per-cell for each detector."""
        workspace = self._refresh_render_with_mock_workspace(n_detectors=2)

        model = self._create_mock_model(workspace, n_pickable=2)
        positions = np.array([[0, 0, 0], [1, 0, 0]], dtype=np.float64)
        mesh = self.renderer.build_detector_mesh(positions, False, model)

        counts = np.array([100, 200])
        self.renderer.set_detector_scalars(mesh, counts, "Counts")

        # Each detector has the same shape → same num faces. All cells belonging
        # to detector 0 should have value 100, all for detector 1 should have 200.
        cell_scalars = mesh.cell_data["Counts"]
        c2d = self.renderer._cell_to_detector
        for det_idx, expected in enumerate(counts):
            mask = c2d == det_idx
            np.testing.assert_array_equal(cell_scalars[mask], expected)

    def test_empty_positions_returns_empty_mesh(self):
        self._refresh_render_with_mock_workspace(n_detectors=2)
        mesh = self.renderer.build_masked_mesh(np.empty((0, 3)), False, model=None)
        self.assertEqual(mesh.number_of_points, 0)

    def test_shape_deduplication(self):
        """Detectors sharing the same shape XML should reuse the cached template."""
        self._refresh_render_with_mock_workspace(n_detectors=10, same_shape=True)
        # Only 1 unique shape (plus potentially the fallback)
        # Since all share the same XML, cache should have exactly 1 entry
        self.assertEqual(len(self.renderer._shape_cache), 1)

    def test_add_detector_mesh_to_plotter(self):
        """Verify that add_detector_mesh_to_plotter adds a surface mesh without point rendering."""
        plotter = MagicMock()
        plotter.off_screen = True
        mesh = pv.Sphere()  # A valid surface mesh
        self.renderer._faces_per_detector = np.array([96])  # sphere has ~96 faces
        self.renderer.add_detector_mesh_to_plotter(plotter, mesh, is_projection=False, scalars="Counts")
        plotter.add_mesh.assert_called_once()
        call_kwargs = plotter.add_mesh.call_args[1]
        # Shape renderer should NOT use render_points_as_spheres
        self.assertNotIn("render_points_as_spheres", call_kwargs)

    def test_set_pickable_scalars_after_rebuild_with_fewer_detectors(self):
        """Rebuilding the detector mesh with fewer detectors should not cause
        an array shape mismatch in set_pickable_scalars.
        """
        workspace = self._refresh_render_with_mock_workspace(n_detectors=6, same_shape=True)

        # First render: 6 detectors
        model_6 = self._create_mock_model(workspace, n_pickable=6)
        positions_6 = np.array([[i, 0, 0] for i in range(6)], dtype=np.float64)
        self.renderer.build_detector_mesh(positions_6, False, model_6)
        pickable_6 = self.renderer.build_pickable_mesh(positions_6, False)
        vis_6 = np.zeros(6)
        self.renderer.set_pickable_scalars(pickable_6, vis_6, "Visible Picked")

        # Second render: only 4 pickable detectors (e.g. 2 were masked)
        model_4 = self._create_mock_model(workspace, n_pickable=4)
        positions_4 = np.array([[i, 0, 0] for i in range(4)], dtype=np.float64)
        self.renderer.build_detector_mesh(positions_4, False, model_4)

        # build_pickable_mesh returns a shape copy matching the 4-detector mesh
        pickable_4 = self.renderer.build_pickable_mesh(positions_4, False)
        vis_4 = np.zeros(4)
        # This must not raise despite the mesh having fewer cells than before
        self.renderer.set_pickable_scalars(pickable_4, vis_4, "Visible Picked")
        self.assertEqual(len(pickable_4.cell_data["Visible Picked"]), pickable_4.number_of_cells)

    def test_build_detector_mesh_projects_shape_vertices_for_cylindrical_projection(self):
        workspace = self._refresh_render_with_mock_workspace(n_detectors=1)

        projection = MagicMock()
        projection.project_points.side_effect = lambda points, apply_x_correction=True: np.column_stack(
            [points[:, 0] + 5.0, points[:, 1] - 3.0]
        )
        projection.u_period = 0

        model = self._create_mock_model(workspace, n_pickable=1)
        model.is_2d_projection = True
        model.projection_type = ProjectionType.CYLINDRICAL_Z
        model.active_projection = projection

        projected_centres = np.array([[100.0, 200.0, 0.0]], dtype=np.float64)
        mesh = self.renderer.build_detector_mesh(projected_centres, False, model)

        self.assertTrue(projection.project_points.called)
        self.assertLess(np.max(mesh.points[:, 0]), 6.0)
        self.assertGreater(np.min(mesh.points[:, 0]), 4.9)
        self.assertLess(np.max(mesh.points[:, 1]), -2.9)

    def test_build_detector_mesh_keeps_projected_shape_contiguous_across_seam(self):
        workspace = self._refresh_render_with_mock_workspace(n_detectors=1)

        projection = MagicMock()
        projection.u_period = 2 * np.pi

        # Alternate near -pi/+pi to emulate seam crossing while matching input vertex count.
        def seam_crossing_projection(points, apply_x_correction=True):
            n = len(points)
            xs = np.where(np.arange(n) % 2 == 0, -np.pi + 0.01, np.pi - 0.01)
            ys = np.linspace(0.0, 0.01, n)
            return np.column_stack([xs, ys]).astype(np.float64)

        projection.project_points.side_effect = seam_crossing_projection

        model = self._create_mock_model(workspace, n_pickable=1)
        model.is_2d_projection = True
        model.projection_type = ProjectionType.CYLINDRICAL_Z
        model.active_projection = projection

        # Centre close to +pi branch.
        projected_centres = np.array([[np.pi - 0.015, 0.0, 0.0]], dtype=np.float64)
        mesh = self.renderer.build_detector_mesh(projected_centres, False, model)

        x_span = float(np.max(mesh.points[:, 0]) - np.min(mesh.points[:, 0]))
        self.assertLess(x_span, 0.1)

    def test_build_pickable_mesh_flip_z_negates_z_in_point_cloud_fallback(self):
        """When no detector mesh ref exists, build_pickable_mesh should negate
        z-coordinates when flip_z=True (falls back to a plain point cloud)."""
        positions = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, -6.0]])
        mesh = self.renderer.build_pickable_mesh(positions, flip_z=True)
        np.testing.assert_allclose(mesh.points[:, 2], [-3.0, 6.0])

    def test_build_pickable_mesh_no_flip_z_unchanged_in_point_cloud_fallback(self):
        """When no detector mesh ref exists, z-coordinates should be unchanged
        when flip_z=False."""
        positions = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, -6.0]])
        mesh = self.renderer.build_pickable_mesh(positions, flip_z=False)
        np.testing.assert_allclose(mesh.points[:, 2], [3.0, -6.0])

    def test_build_pickable_mesh_flip_z_ignored_when_detector_mesh_ref_exists(self):
        """When _detector_mesh_ref has been built, build_pickable_mesh should
        return a shape mesh copy regardless of flip_z — the flip is already
        baked into the detector mesh vertices."""
        workspace = self._refresh_render_with_mock_workspace(n_detectors=2)
        model = self._create_mock_model(workspace, n_pickable=2)
        positions = np.array([[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]], dtype=np.float64)
        ref_mesh = self.renderer.build_detector_mesh(positions, False, model)

        # flip_z=True should still return the shape mesh copy (not a point cloud)
        pickable = self.renderer.build_pickable_mesh(positions, flip_z=True)
        self.assertGreater(pickable.number_of_cells, 0)
        self.assertEqual(pickable.number_of_cells, ref_mesh.number_of_cells)

    def test_build_detector_mesh_flip_z_negates_world_z_before_projection(self):
        """In cylindrical projections, flip_z=True must negate the world-space
        z-coordinate of each shape vertex before it is passed to project_points."""
        workspace = self._refresh_render_with_mock_workspace(n_detectors=1)
        # Override the stored 3D position so the flip is observable (z ≠ 0)
        self.renderer._all_positions_3d = np.array([[0.0, 0.0, 1.5]])

        model = self._create_mock_model(workspace, n_pickable=1)
        model.is_2d_projection = True
        model.projection_type = ProjectionType.CYLINDRICAL_Z
        positions = np.array([[0.0, 0.0, 0.0]], dtype=np.float64)

        def make_capturing_projection():
            proj = MagicMock()
            proj.u_period = 0
            calls = []

            def record(pts, apply_x_correction=True, _buf=calls):
                _buf.append(pts[:, 2].copy())
                return np.zeros((len(pts), 2))

            proj.project_points.side_effect = record
            return proj, calls

        proj_no_flip, z_no_flip_calls = make_capturing_projection()
        model.active_projection = proj_no_flip
        self.renderer.build_detector_mesh(positions, False, model)

        proj_flip, z_flip_calls = make_capturing_projection()
        model.active_projection = proj_flip
        self.renderer.build_detector_mesh(positions, True, model)

        z_no_flip = np.concatenate(z_no_flip_calls)
        z_flip = np.concatenate(z_flip_calls)

        # Without flip: world z comes from _all_positions_3d, so all values > 0
        self.assertTrue(np.all(z_no_flip > 0), f"Expected positive z without flip, got {z_no_flip}")
        # With flip: all world z values should be negated
        self.assertTrue(np.all(z_flip < 0), f"Expected negative z with flip, got {z_flip}")
        np.testing.assert_allclose(z_flip, -z_no_flip)

    def test_build_masked_mesh_flip_z_negates_world_z_before_projection(self):
        """flip_z=True should also negate world-space z for masked detector vertices
        before projection (same _assemble_mesh path as build_detector_mesh)."""

        workspace = self._refresh_render_with_mock_workspace(n_detectors=1)
        self.renderer._all_positions_3d = np.array([[0.0, 0.0, 2.0]])

        model = self._create_mock_model(workspace, n_pickable=0)
        model.is_2d_projection = True
        model.projection_type = ProjectionType.CYLINDRICAL_Z
        model.masked_detector_ids = np.array([0])
        positions = np.array([[0.0, 0.0, 0.0]], dtype=np.float64)

        def make_capturing_projection():
            proj = MagicMock()
            proj.u_period = 0
            calls = []

            def record(pts, apply_x_correction=True, _buf=calls):
                _buf.append(pts[:, 2].copy())
                return np.zeros((len(pts), 2))

            proj.project_points.side_effect = record
            return proj, calls

        proj_no_flip, z_no_flip_calls = make_capturing_projection()
        model.active_projection = proj_no_flip
        self.renderer.build_masked_mesh(positions, False, model)

        proj_flip, z_flip_calls = make_capturing_projection()
        model.active_projection = proj_flip
        self.renderer.build_masked_mesh(positions, True, model)

        z_no_flip = np.concatenate(z_no_flip_calls)
        z_flip = np.concatenate(z_flip_calls)

        self.assertTrue(np.all(z_no_flip > 0), f"Expected positive z without flip, got {z_no_flip}")
        self.assertTrue(np.all(z_flip < 0), f"Expected negative z with flip, got {z_flip}")
        np.testing.assert_allclose(z_flip, -z_no_flip)

    # ------------------------------------------------------------------
    # Helper methods to create mock objects
    # ------------------------------------------------------------------

    def _refresh_render_with_mock_workspace(self, **kwargs):
        mock_ws = self._create_mock_workspace(**kwargs)
        self.renderer = ShapeRenderer(mock_ws)
        self.renderer.precompute()
        return mock_ws

    def _create_mock_workspace(self, n_detectors=4, same_shape=False):
        """Create a mock workspace with mock componentInfo and detectorInfo."""
        workspace = MagicMock()

        det_info = MagicMock()
        det_info.size.return_value = n_detectors
        # Each detector has a unique position

        def position(i):
            pos = MagicMock()
            pos.X.return_value = float(i)
            pos.Y.return_value = 0.0
            pos.Z.return_value = 0.0
            return pos

        for i in range(n_detectors):
            pos = MagicMock()
            pos.X.return_value = float(i)
            pos.Y.return_value = 0.0
            pos.Z.return_value = 0.0
            det_info.position.side_effect = lambda idx: position(idx)
            det_info.indexOf.side_effect = lambda did: int(did)
        det_info.detectorIDs.return_value = list(range(n_detectors))

        comp_info = MagicMock()
        comp_info.hasValidShape.return_value = True

        # Create a simple triangle mesh for each shape
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

        # Rotation: identity quaternion
        identity_quat = MagicMock()
        identity_quat.real.return_value = 1.0
        identity_quat.imagI.return_value = 0.0
        identity_quat.imagJ.return_value = 0.0
        identity_quat.imagK.return_value = 0.0
        comp_info.rotation.return_value = identity_quat

        # Scale: unit scale
        unit_scale = MagicMock()
        unit_scale.X.return_value = 1.0
        unit_scale.Y.return_value = 1.0
        unit_scale.Z.return_value = 1.0
        comp_info.scaleFactor.return_value = unit_scale

        workspace.componentInfo.return_value = comp_info
        workspace.detectorInfo.return_value = det_info

        return workspace

    def _create_mock_model(self, workspace, n_pickable=4):
        model = MagicMock()
        model.workspace = workspace
        model.is_2d_projection = False
        model.projection_type = ProjectionType.THREE_D
        model.active_projection = None
        model.flip_z = False
        model.pickable_detector_ids = np.arange(n_pickable)
        model.masked_detector_ids = np.array([], dtype=np.int64)
        return model


if __name__ == "__main__":
    unittest.main()
