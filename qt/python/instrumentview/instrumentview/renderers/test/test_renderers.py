# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Unit tests for the PointCloudRenderer and ShapeRenderer classes."""

import unittest
from unittest.mock import MagicMock, patch, ANY

import numpy as np
import pyvista as pv

from instrumentview.renderers.base_renderer import InstrumentRenderer
from instrumentview.renderers.point_cloud_renderer import PointCloudRenderer
from instrumentview.renderers.shape_renderer import (
    ShapeRenderer,
    _triangles_to_verts_faces,
    _make_fallback_shape,
    _extract_quad_from_cylinder_shapeinfo,
    _extract_quad_from_cuboid_shapeinfo,
)
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
        self.renderer.add_detector_mesh_to_plotter(plotter, mesh, scalars="Counts")
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

    # --------------------------------------------------------------- picking

    def _make_mock_plotter(self, off_screen=False):
        plotter = MagicMock()
        plotter.off_screen = off_screen
        plotter.iren.get_event_position.return_value = (100, 200)
        return plotter

    @patch("instrumentview.renderers.point_cloud_renderer.vtkPointPicker")
    def test_enable_picking_off_screen_skips_observer(self, mock_picker_cls):
        plotter = self._make_mock_plotter(off_screen=True)
        self.renderer.enable_picking(plotter, MagicMock())
        plotter.disable_picking.assert_called_once()
        plotter.iren.style.AddObserver.assert_not_called()

    @patch("instrumentview.renderers.point_cloud_renderer.vtkPointPicker")
    def test_enable_picking_click_registers_left_button_observer(self, mock_picker_cls):
        plotter = self._make_mock_plotter()
        self.renderer.enable_picking(plotter, MagicMock())
        plotter.iren.style.AddObserver.assert_called_once_with("LeftButtonPressEvent", ANY)
        self.assertIsNotNone(self.renderer._left_button_observer_id)
        self.assertIsNone(self.renderer._mouse_move_observer_id)

    @patch("instrumentview.renderers.point_cloud_renderer.vtkPointPicker")
    def test_enable_picking_hover_registers_mouse_move_observer(self, mock_picker_cls):
        plotter = self._make_mock_plotter()
        self.renderer.enable_picking(plotter, MagicMock(), hover=True)
        plotter.iren.style.AddObserver.assert_called_once_with("MouseMoveEvent", ANY)
        self.assertIsNotNone(self.renderer._mouse_move_observer_id)
        self.assertIsNone(self.renderer._left_button_observer_id)

    @patch("instrumentview.renderers.point_cloud_renderer.vtkPointPicker")
    def test_enable_picking_reinit_removes_previous_observer(self, mock_picker_cls):
        plotter = self._make_mock_plotter()
        self.renderer.enable_picking(plotter, MagicMock())
        old_id = self.renderer._left_button_observer_id
        self.renderer.enable_picking(plotter, MagicMock(), hover=True)
        plotter.iren.style.RemoveObserver.assert_called_with(old_id)

    @patch("instrumentview.renderers.point_cloud_renderer.vtkPointPicker")
    def test_enable_picking_click_fires_callback_on_hit(self, mock_picker_cls):
        mock_picker = MagicMock()
        mock_picker_cls.return_value = mock_picker
        mock_picker.Pick.return_value = 1
        mock_picker.GetPointId.return_value = 5
        plotter = self._make_mock_plotter()
        callback = MagicMock()
        self.renderer.enable_picking(plotter, callback)
        observer_fn = plotter.iren.style.AddObserver.call_args[0][1]
        observer_fn(None, None)
        callback.assert_called_once_with(5)

    @patch("instrumentview.renderers.point_cloud_renderer.vtkPointPicker")
    def test_enable_picking_click_does_not_fire_on_miss(self, mock_picker_cls):
        mock_picker = MagicMock()
        mock_picker_cls.return_value = mock_picker
        mock_picker.Pick.return_value = 0
        plotter = self._make_mock_plotter()
        callback = MagicMock()
        self.renderer.enable_picking(plotter, callback)
        observer_fn = plotter.iren.style.AddObserver.call_args[0][1]
        observer_fn(None, None)
        callback.assert_not_called()

    @patch("instrumentview.renderers.point_cloud_renderer.vtkPointPicker")
    def test_enable_picking_hover_fires_callback_when_point_found(self, mock_picker_cls):
        mock_picker = MagicMock()
        mock_picker_cls.return_value = mock_picker
        mock_picker.GetPointId.return_value = 3
        mock_picker.Pick.return_value = 3
        plotter = self._make_mock_plotter()
        callback = MagicMock()
        self.renderer.enable_picking(plotter, callback, hover=True)
        observer_fn = plotter.iren.style.AddObserver.call_args[0][1]
        observer_fn(None, None)
        callback.assert_called_once_with(3)

    @patch("instrumentview.renderers.point_cloud_renderer.vtkPointPicker")
    def test_enable_picking_hover_does_not_fire_when_no_point(self, mock_picker_cls):
        mock_picker = MagicMock()
        mock_picker_cls.return_value = mock_picker
        mock_picker.GetPointId.return_value = -1
        mock_picker.Pick.return_value = -1
        plotter = self._make_mock_plotter()
        callback = MagicMock()
        self.renderer.enable_picking(plotter, callback, hover=True)
        observer_fn = plotter.iren.style.AddObserver.call_args[0][1]
        observer_fn(None, None)
        callback.assert_not_called()


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
        verts, faces, face_size = _make_fallback_shape()
        self.assertEqual(verts.shape, (4, 3))
        self.assertEqual(faces.shape, (4, 3))
        self.assertEqual(face_size, 3)


class TestExtractQuadFromCuboidShapeinfo(unittest.TestCase):
    """Tests for _extract_quad_from_cuboid_shapeinfo."""

    def _make_v3d(self, x, y, z):
        v = MagicMock()
        v.X.return_value = float(x)
        v.Y.return_value = float(y)
        v.Z.return_value = float(z)
        return v

    def _make_cuboid_si(self, x_min=-0.005, x_max=0.005, y_min=-0.005, y_max=0.005, z_front=-0.0005, z_back=0.0005):
        si = MagicMock()
        si.cuboidGeometry.return_value = {
            "leftFrontBottom": self._make_v3d(x_min, y_min, z_front),
            "leftFrontTop": self._make_v3d(x_min, y_max, z_front),
            "leftBackBottom": self._make_v3d(x_min, y_min, z_back),
            "rightFrontBottom": self._make_v3d(x_max, y_min, z_front),
        }
        return si

    def test_rectangular_cuboid_returns_quad(self):
        si = self._make_cuboid_si()
        result = _extract_quad_from_cuboid_shapeinfo(si)
        self.assertIsNotNone(result)
        quad_verts, quad_faces = result
        self.assertEqual(quad_verts.shape, (4, 3))
        self.assertEqual(quad_faces.shape, (1, 4))

    def test_quad_corners_derived_from_shapeinfo(self):
        """Quad X/Y extents should come directly from the ShapeInfo corner points."""
        si = self._make_cuboid_si(x_min=-0.01, x_max=0.01, y_min=-0.005, y_max=0.005)
        result = _extract_quad_from_cuboid_shapeinfo(si)
        self.assertIsNotNone(result)
        quad_verts, _ = result
        np.testing.assert_allclose(np.sort(np.unique(quad_verts[:, 0])), [-0.01, 0.01], atol=1e-10)
        np.testing.assert_allclose(np.sort(np.unique(quad_verts[:, 1])), [-0.005, 0.005], atol=1e-10)

    def test_quad_z_at_mid_depth(self):
        """Quad Z should be the midpoint of z_front and z_back."""
        si = self._make_cuboid_si(z_front=-0.002, z_back=0.006)
        result = _extract_quad_from_cuboid_shapeinfo(si)
        self.assertIsNotNone(result)
        quad_verts, _ = result
        np.testing.assert_allclose(quad_verts[:, 2], 0.002, atol=1e-10)

    def test_cube_shaped_detector_uses_xy_face(self):
        """Equal extents on all axes: quad must span X/Y at mid-Z."""
        s = 0.0127
        si = self._make_cuboid_si(x_min=-s, x_max=s, y_min=-s, y_max=s, z_front=-s, z_back=s)
        result = _extract_quad_from_cuboid_shapeinfo(si)
        self.assertIsNotNone(result)
        quad_verts, _ = result
        np.testing.assert_allclose(quad_verts[:, 2], 0.0, atol=1e-10)
        np.testing.assert_allclose(np.sort(np.unique(quad_verts[:, 0])), [-s, s], atol=1e-10)
        np.testing.assert_allclose(np.sort(np.unique(quad_verts[:, 1])), [-s, s], atol=1e-10)

    def test_zero_xy_extent_returns_none(self):
        """A degenerate cuboid with no XY extent should return None."""
        si = self._make_cuboid_si(x_min=0.0, x_max=0.0, y_min=0.0, y_max=0.0, z_front=-0.01, z_back=0.01)
        result = _extract_quad_from_cuboid_shapeinfo(si)
        self.assertIsNone(result)

    def test_exception_in_cuboid_geometry_returns_none(self):
        si = MagicMock()
        si.cuboidGeometry.side_effect = RuntimeError("bad shape")
        self.assertIsNone(_extract_quad_from_cuboid_shapeinfo(si))


class TestExtractQuadFromCylinderShapeinfo(unittest.TestCase):
    """Tests for _extract_quad_from_cylinder_shapeinfo."""

    def _make_v3d(self, x, y, z):
        v = MagicMock()
        v.X.return_value = float(x)
        v.Y.return_value = float(y)
        v.Z.return_value = float(z)
        return v

    def _make_cylinder_si(self, bottom_base=(0.0, 0.0, 0.0), axis=(0.0, 1.0, 0.0), radius=0.004, height=0.00203320):
        """Return a mock ShapeInfo whose cylinderGeometry() describes a cylinder."""
        si = MagicMock()
        si.cylinderGeometry.return_value = {
            "centreOfBottomBase": self._make_v3d(*bottom_base),
            "axis": self._make_v3d(*axis),
            "radius": radius,
            "height": height,
        }
        return si

    # --- WISH-like Y-axis cylinder (axis along local Y, bottom base at origin) ---

    def test_wish_cylinder_returns_quad(self):
        si = self._make_cylinder_si()
        result = _extract_quad_from_cylinder_shapeinfo(si)
        self.assertIsNotNone(result)
        quad_verts, quad_faces = result
        self.assertEqual(quad_verts.shape, (4, 3))
        self.assertEqual(quad_faces.shape, (1, 4))

    def test_wish_cylinder_quad_spans_correct_height(self):
        """Quad height (along Y axis) should equal cylinder height."""
        height = 0.00203320
        si = self._make_cylinder_si(axis=(0.0, 1.0, 0.0), height=height)
        quad_verts, _ = _extract_quad_from_cylinder_shapeinfo(si)
        y_min, y_max = quad_verts[:, 1].min(), quad_verts[:, 1].max()
        self.assertAlmostEqual(y_max - y_min, height, places=8)

    def test_wish_cylinder_quad_spans_correct_width(self):
        """Quad width (along the s_hat direction) should equal 2*radius."""
        radius = 0.004
        si = self._make_cylinder_si(axis=(0.0, 1.0, 0.0), radius=radius)
        quad_verts, _ = _extract_quad_from_cylinder_shapeinfo(si)
        x_min, x_max = quad_verts[:, 0].min(), quad_verts[:, 0].max()
        self.assertAlmostEqual(x_max - x_min, 2 * radius, places=8)

    def test_wish_cylinder_quad_lies_in_xy_plane(self):
        """For Y-axis cylinder, quad should lie in the local XY plane (z=0)."""
        si = self._make_cylinder_si(axis=(0.0, 1.0, 0.0))
        quad_verts, _ = _extract_quad_from_cylinder_shapeinfo(si)
        np.testing.assert_allclose(quad_verts[:, 2], 0.0, atol=1e-12)

    def test_wish_cylinder_normal_faces_negative_z(self):
        """Normal = e1 x e2 should point in the (0, 0, ±1) direction for Y-axis cylinder."""
        si = self._make_cylinder_si(axis=(0.0, 1.0, 0.0))
        quad_verts, _ = _extract_quad_from_cylinder_shapeinfo(si)
        e1 = quad_verts[1] - quad_verts[0]
        e2 = quad_verts[3] - quad_verts[0]
        normal = np.cross(e1, e2)
        normal /= np.linalg.norm(normal)
        self.assertAlmostEqual(abs(normal[2]), 1.0, places=6)

    # --- Z-axis cylinder (axis parallel to sample direction) ---

    def test_z_axis_cylinder_falls_back_gracefully(self):
        """Cylinder with axis parallel to sample dir (0,0,1) should still return a quad."""
        si = self._make_cylinder_si(axis=(0.0, 0.0, 1.0), bottom_base=(0.0, 0.0, -0.05), height=0.1)
        result = _extract_quad_from_cylinder_shapeinfo(si)
        self.assertIsNotNone(result)
        quad_verts, _ = result
        self.assertEqual(quad_verts.shape, (4, 3))

    # --- Error / degenerate cases ---

    def test_zero_axis_returns_none(self):
        si = self._make_cylinder_si(axis=(0.0, 0.0, 0.0))
        self.assertIsNone(_extract_quad_from_cylinder_shapeinfo(si))

    def test_exception_in_cylinder_geometry_returns_none(self):
        si = MagicMock()
        si.cylinderGeometry.side_effect = RuntimeError("bad shape")
        self.assertIsNone(_extract_quad_from_cylinder_shapeinfo(si))

    def test_quad_faces_correct_vertex_indices(self):
        si = self._make_cylinder_si()
        _, quad_faces = _extract_quad_from_cylinder_shapeinfo(si)
        np.testing.assert_array_equal(quad_faces[0], [0, 1, 2, 3])


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
        self.renderer.add_detector_mesh_to_plotter(plotter, mesh, scalars="Counts")
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

    def test_build_pickable_mesh_flip_beam_negates_z_in_point_cloud_fallback(self):
        """When no detector mesh ref exists, build_pickable_mesh should negate
        z-coordinates when flip_beam=True (falls back to a plain point cloud)."""
        positions = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, -6.0]])
        mesh = self.renderer.build_pickable_mesh(positions, flip_beam=True)
        np.testing.assert_allclose(mesh.points[:, 2], [-3.0, 6.0])

    def test_build_pickable_mesh_no_flip_beam_unchanged_in_point_cloud_fallback(self):
        """When no detector mesh ref exists, z-coordinates should be unchanged
        when flip_beam=False."""
        positions = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, -6.0]])
        mesh = self.renderer.build_pickable_mesh(positions, flip_beam=False)
        np.testing.assert_allclose(mesh.points[:, 2], [3.0, -6.0])

    def test_build_pickable_mesh_flip_beam_ignored_when_detector_mesh_ref_exists(self):
        """When _detector_mesh_ref has been built, build_pickable_mesh should
        return a shape mesh copy regardless of flip_beam — the flip is already
        baked into the detector mesh vertices."""
        workspace = self._refresh_render_with_mock_workspace(n_detectors=2)
        model = self._create_mock_model(workspace, n_pickable=2)
        positions = np.array([[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]], dtype=np.float64)
        ref_mesh = self.renderer.build_detector_mesh(positions, False, model)

        # flip_beam=True should still return the shape mesh copy (not a point cloud)
        pickable = self.renderer.build_pickable_mesh(positions, flip_beam=True)
        self.assertGreater(pickable.number_of_cells, 0)
        self.assertEqual(pickable.number_of_cells, ref_mesh.number_of_cells)

    def test_build_detector_mesh_flip_beam_negates_world_z_before_projection(self):
        """In cylindrical projections, flip_beam=True must negate the world-space
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

    def test_build_masked_mesh_flip_beam_negates_world_z_before_projection(self):
        """flip_beam=True should also negate world-space z for masked detector vertices
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

    def _make_shape_mock_plotter(self, off_screen=False):
        plotter = MagicMock()
        plotter.off_screen = off_screen
        plotter.iren.get_event_position.return_value = (100, 200)
        return plotter

    @patch("instrumentview.renderers.shape_renderer.vtkCellPicker")
    def test_enable_picking_off_screen_skips_observer(self, mock_picker_cls):
        plotter = self._make_shape_mock_plotter(off_screen=True)
        self.renderer.enable_picking(plotter, MagicMock())
        plotter.disable_picking.assert_called_once()
        plotter.iren.style.AddObserver.assert_not_called()

    @patch("instrumentview.renderers.shape_renderer.vtkCellPicker")
    def test_enable_picking_click_registers_left_button_observer(self, mock_picker_cls):
        plotter = self._make_shape_mock_plotter()
        self.renderer.enable_picking(plotter, MagicMock())
        plotter.iren.style.AddObserver.assert_called_once_with("LeftButtonPressEvent", ANY)
        self.assertIsNotNone(self.renderer._left_button_observer_id)
        self.assertIsNone(self.renderer._mouse_move_observer_id)

    @patch("instrumentview.renderers.shape_renderer.vtkCellPicker")
    def test_enable_picking_hover_registers_mouse_move_observer(self, mock_picker_cls):
        plotter = self._make_shape_mock_plotter()
        self.renderer.enable_picking(plotter, MagicMock(), hover=True)
        plotter.iren.style.AddObserver.assert_called_once_with("MouseMoveEvent", ANY)
        self.assertIsNotNone(self.renderer._mouse_move_observer_id)
        self.assertIsNone(self.renderer._left_button_observer_id)

    @patch("instrumentview.renderers.shape_renderer.vtkCellPicker")
    def test_enable_picking_reinit_clears_and_removes_previous_observer(self, mock_picker_cls):
        plotter = self._make_shape_mock_plotter()
        self.renderer.enable_picking(plotter, MagicMock())
        old_id = self.renderer._left_button_observer_id
        self.renderer.enable_picking(plotter, MagicMock(), hover=True)
        plotter.iren.style.RemoveObserver.assert_called_with(old_id)
        # _clear_observers resets both IDs; only the new hover ID should be set
        self.assertIsNotNone(self.renderer._mouse_move_observer_id)
        self.assertIsNone(self.renderer._left_button_observer_id)

    @patch("instrumentview.renderers.shape_renderer.vtkCellPicker")
    def test_enable_picking_click_fires_callback_via_cell_to_detector(self, mock_picker_cls):
        mock_picker = MagicMock()
        mock_picker_cls.return_value = mock_picker
        mock_picker.Pick.return_value = 1
        mock_picker.GetCellId.return_value = 1
        self.renderer._cell_to_detector = np.array([0, 7, 2])
        plotter = self._make_shape_mock_plotter()
        callback = MagicMock()
        self.renderer.enable_picking(plotter, callback)
        observer_fn = plotter.iren.style.AddObserver.call_args[0][1]
        observer_fn(None, None)
        callback.assert_called_once_with(7)

    @patch("instrumentview.renderers.shape_renderer.vtkCellPicker")
    def test_enable_picking_click_does_not_fire_on_miss(self, mock_picker_cls):
        mock_picker = MagicMock()
        mock_picker_cls.return_value = mock_picker
        mock_picker.Pick.return_value = 0
        self.renderer._cell_to_detector = np.array([0, 1, 2])
        plotter = self._make_shape_mock_plotter()
        callback = MagicMock()
        self.renderer.enable_picking(plotter, callback)
        observer_fn = plotter.iren.style.AddObserver.call_args[0][1]
        observer_fn(None, None)
        callback.assert_not_called()

    @patch("instrumentview.renderers.shape_renderer.vtkCellPicker")
    def test_enable_picking_does_not_fire_when_c2d_is_none(self, mock_picker_cls):
        mock_picker = MagicMock()
        mock_picker_cls.return_value = mock_picker
        mock_picker.Pick.return_value = 1
        mock_picker.GetCellId.return_value = 0
        self.renderer._cell_to_detector = None
        plotter = self._make_shape_mock_plotter()
        callback = MagicMock()
        self.renderer.enable_picking(plotter, callback)
        observer_fn = plotter.iren.style.AddObserver.call_args[0][1]
        observer_fn(None, None)
        callback.assert_not_called()

    @patch("instrumentview.renderers.shape_renderer.vtkCellPicker")
    def test_enable_picking_hover_fires_callback_when_cell_found(self, mock_picker_cls):
        mock_picker = MagicMock()
        mock_picker_cls.return_value = mock_picker
        mock_picker.Pick.return_value = 1
        mock_picker.GetCellId.return_value = 2
        self.renderer._cell_to_detector = np.array([4, 5, 9])
        plotter = self._make_shape_mock_plotter()
        callback = MagicMock()
        self.renderer.enable_picking(plotter, callback, hover=True)
        observer_fn = plotter.iren.style.AddObserver.call_args[0][1]
        observer_fn(None, None)
        callback.assert_called_once_with(9)

    @patch("instrumentview.renderers.shape_renderer.vtkCellPicker")
    def test_enable_picking_hover_does_not_fire_when_no_cell(self, mock_picker_cls):
        mock_picker = MagicMock()
        mock_picker_cls.return_value = mock_picker
        mock_picker.Pick.return_value = 0
        self.renderer._cell_to_detector = np.array([0, 1, 2])
        plotter = self._make_shape_mock_plotter()
        callback = MagicMock()
        self.renderer.enable_picking(plotter, callback, hover=True)
        observer_fn = plotter.iren.style.AddObserver.call_args[0][1]
        observer_fn(None, None)
        callback.assert_not_called()

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

        # Bulk methods required by precompute() — identity quaternions (scipy [x,y,z,w]) and unit scales
        det_info.allRotations.return_value = np.tile([0.0, 0.0, 0.0, 1.0], (n_detectors, 1))
        det_info.allScaleFactors.return_value = np.ones((n_detectors, 3))
        det_info.allPositions.return_value = np.column_stack([np.arange(n_detectors, dtype=float), np.zeros((n_detectors, 2))])

        comp_info = MagicMock()
        comp_info.hasValidShape.return_value = True

        # Create a simple triangle mesh for each shape (non-cuboid/cylinder so getMesh path is used)
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
                shape.getShapeXML.return_value = "<sphere>same_shape</sphere>"
            else:
                shape.getShapeXML.return_value = f"<sphere>shape_{i % 2}</sphere>"
            shape.getMesh.return_value = triangle_mesh.copy()
            # shapeInfo().shape() returns a MagicMock — does not match GeometryShape.CYLINDER/CUBOID,
            # so precompute() falls through to the getMesh() path for these mock shapes.
            shape_objs.append(shape)

        comp_info.shape.side_effect = lambda idx: shape_objs[idx]

        workspace.componentInfo.return_value = comp_info
        workspace.detectorInfo.return_value = det_info
        workspace.getInstrument().getReferenceFrame().vecPointingAlongBeam.return_value = [0, 0, 1]

        return workspace

    def _create_mock_model(self, workspace, n_pickable=4):
        model = MagicMock()
        model.workspace = workspace
        model.is_2d_projection = False
        model.projection_type = ProjectionType.THREE_D
        model.active_projection = None
        model.flip_beam = False
        model.pickable_detector_ids = np.arange(n_pickable)
        model.masked_detector_ids = np.array([], dtype=np.int64)
        return model


if __name__ == "__main__":
    unittest.main()
