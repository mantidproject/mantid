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

from instrumentview.Renderers.base_renderer import InstrumentRenderer
from instrumentview.Renderers.point_cloud_renderer import PointCloudRenderer
from instrumentview.Renderers.shape_renderer import ShapeRenderer, _triangles_to_verts_faces, _make_fallback_shape


class TestPointCloudRenderer(unittest.TestCase):
    """Tests for the PointCloudRenderer (original rendering mode)."""

    def setUp(self):
        self.renderer = PointCloudRenderer()
        self.positions = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0], [7.0, 8.0, 9.0]])

    def test_is_instrument_renderer(self):
        self.assertIsInstance(self.renderer, InstrumentRenderer)

    def test_build_detector_mesh_returns_polydata(self):
        mesh = self.renderer.build_detector_mesh(self.positions)
        self.assertIsInstance(mesh, pv.PolyData)
        self.assertEqual(mesh.number_of_points, 3)

    def test_build_pickable_mesh_returns_polydata(self):
        mesh = self.renderer.build_pickable_mesh(self.positions)
        self.assertIsInstance(mesh, pv.PolyData)
        self.assertEqual(mesh.number_of_points, 3)

    def test_build_masked_mesh_returns_polydata(self):
        mesh = self.renderer.build_masked_mesh(self.positions)
        self.assertIsInstance(mesh, pv.PolyData)
        self.assertEqual(mesh.number_of_points, 3)

    def test_set_detector_scalars_sets_point_data(self):
        mesh = self.renderer.build_detector_mesh(self.positions)
        counts = np.array([10, 20, 30])
        self.renderer.set_detector_scalars(mesh, counts, "Counts")
        np.testing.assert_array_equal(mesh.point_data["Counts"], counts)

    def test_set_pickable_scalars_sets_point_data(self):
        mesh = self.renderer.build_pickable_mesh(self.positions)
        vis = np.array([0, 1, 0])
        self.renderer.set_pickable_scalars(mesh, vis, "Visible")
        np.testing.assert_array_equal(mesh.point_data["Visible"], vis)

    def test_add_detector_mesh_to_plotter_calls_add_mesh(self):
        plotter = MagicMock()
        plotter.off_screen = True
        mesh = self.renderer.build_detector_mesh(self.positions)
        self.renderer.add_detector_mesh_to_plotter(plotter, mesh, is_projection=False, scalars="Counts")
        plotter.add_mesh.assert_called_once()
        call_kwargs = plotter.add_mesh.call_args[1]
        self.assertTrue(call_kwargs["render_points_as_spheres"])
        self.assertEqual(call_kwargs["point_size"], 15)

    def test_add_pickable_mesh_to_plotter_calls_add_mesh(self):
        plotter = MagicMock()
        mesh = self.renderer.build_pickable_mesh(self.positions)
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
        mesh = self.renderer.build_masked_mesh(self.positions)
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
        self.renderer = ShapeRenderer()

    def test_is_instrument_renderer(self):
        self.assertIsInstance(self.renderer, InstrumentRenderer)

    def test_build_detector_mesh_raises_without_precompute(self):
        positions = np.array([[0.0, 0.0, 0.0]])
        with self.assertRaises(RuntimeError):
            self.renderer.build_detector_mesh(positions)

    def test_precompute_and_build_mesh(self):
        """Integration test: create a mock workspace with detectors that have shapes
        and verify mesh assembly produces valid geometry."""
        workspace = self._create_mock_workspace(n_detectors=4)
        self.renderer.precompute(workspace)

        self.assertTrue(self.renderer._precomputed)
        self.assertGreater(len(self.renderer._shape_cache), 0)

        model = self._create_mock_model(workspace, n_pickable=4)
        positions = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0], [1, 1, 0]], dtype=np.float64)
        mesh = self.renderer.build_detector_mesh(positions, model)

        self.assertIsInstance(mesh, pv.PolyData)
        self.assertGreater(mesh.number_of_points, 0)
        self.assertGreater(mesh.number_of_cells, 0)

    def test_cell_to_detector_mapping(self):
        """Verify that each cell in the assembled mesh maps to a valid detector index."""
        workspace = self._create_mock_workspace(n_detectors=3)
        self.renderer.precompute(workspace)

        model = self._create_mock_model(workspace, n_pickable=3)
        positions = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0]], dtype=np.float64)
        mesh = self.renderer.build_detector_mesh(positions, model)

        c2d = self.renderer._cell_to_detector
        self.assertIsNotNone(c2d)
        self.assertEqual(len(c2d), mesh.number_of_cells)
        # All mapped indices must be in {0, 1, 2}
        self.assertTrue(np.all(c2d >= 0))
        self.assertTrue(np.all(c2d < 3))

    def test_set_detector_scalars_cell_data(self):
        """Scalars should be repeated per-cell for each detector."""
        workspace = self._create_mock_workspace(n_detectors=2)
        self.renderer.precompute(workspace)

        model = self._create_mock_model(workspace, n_pickable=2)
        positions = np.array([[0, 0, 0], [1, 0, 0]], dtype=np.float64)
        mesh = self.renderer.build_detector_mesh(positions, model)

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
        workspace = self._create_mock_workspace(n_detectors=2)
        self.renderer.precompute(workspace)
        mesh = self.renderer.build_masked_mesh(np.empty((0, 3)), model=None)
        self.assertEqual(mesh.number_of_points, 0)

    def test_shape_deduplication(self):
        """Detectors sharing the same shape XML should reuse the cached template."""
        workspace = self._create_mock_workspace(n_detectors=10, same_shape=True)
        self.renderer.precompute(workspace)
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

    # ------------------------------------------------------------------
    # Helper methods to create mock objects
    # ------------------------------------------------------------------
    def _create_mock_workspace(self, n_detectors=4, same_shape=False):
        """Create a mock workspace with mock componentInfo and detectorInfo."""
        workspace = MagicMock()

        det_info = MagicMock()
        det_info.size.return_value = n_detectors
        # Each detector has a unique position
        for i in range(n_detectors):
            pos = MagicMock()
            pos.X.return_value = float(i)
            pos.Y.return_value = 0.0
            pos.Z.return_value = 0.0
            det_info.position.side_effect = self._make_position_side_effect(n_detectors)
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
        model.pickable_detector_ids = np.arange(n_pickable)
        model.masked_detector_ids = np.array([], dtype=np.int64)
        return model


if __name__ == "__main__":
    unittest.main()
