# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from unittest.mock import MagicMock
import numpy as np

from instrumentview.ShapeWidgets import (
    AnnulusSelectionShape,
    CircleSelectionShape,
    RectangleSelectionShape,
    EllipseSelectionShape,
    ShapeOverlayManager,
)


class TestCircleSelectionShape(unittest.TestCase):
    def setUp(self):
        self.shape = CircleSelectionShape(cx=0.5, cy=0.5, radius=0.15)

    def test_init(self):
        self.assertEqual(self.shape.cx, 0.5)
        self.assertEqual(self.shape.cy, 0.5)
        self.assertEqual(self.shape.radius, 0.15)

    def test_hit_test_inside(self):
        self.assertEqual(self.shape.hit_test(0.5, 0.5), "inside")

    def test_hit_test_edge(self):
        # On the edge of the circle
        hit = self.shape.hit_test(0.5 + 0.15, 0.5)
        self.assertEqual(hit, "edge")

    def test_hit_test_outside(self):
        self.assertIsNone(self.shape.hit_test(0.0, 0.0))

    def test_indices_in_shape_center(self):
        proj = np.array([[0.5, 0.5]])
        mask = self.shape.indices_in_shape(proj)
        self.assertTrue(mask[0])

    def test_indices_in_shape_outside(self):
        proj = np.array([[0.0, 0.0]])
        mask = self.shape.indices_in_shape(proj)
        self.assertFalse(mask[0])

    def test_indices_in_shape_boundary(self):
        # Point just inside the boundary
        proj = np.array([[0.5 + 0.15 - 1e-9, 0.5]])
        mask = self.shape.indices_in_shape(proj)
        self.assertTrue(mask[0])

    def test_indices_in_shape_multiple(self):
        proj = np.array([[0.5, 0.5], [0.0, 0.0], [0.55, 0.5]])
        mask = self.shape.indices_in_shape(proj)
        np.testing.assert_array_equal(mask, [True, False, True])

    def test_save_size(self):
        s = self.shape.save_size()
        self.assertEqual(s["radius"], 0.15)

    def test_apply_resize_delta(self):
        saved = self.shape.save_size()
        # Move cursor outward from just at the radius to further out
        self.shape.apply_resize_delta(0.5 + 0.25, 0.5, 0.5 + 0.15, 0.5, saved)
        self.assertAlmostEqual(self.shape.radius, 0.25, places=5)

    def test_apply_resize_delta_clamps_minimum(self):
        saved = self.shape.save_size()
        # Move cursor inward past center
        self.shape.apply_resize_delta(0.5, 0.5, 0.5 + 0.15, 0.5, saved)
        self.assertEqual(self.shape.radius, 0.01)

    def test_outline_xy_returns_arrays(self):
        ox, oy = self.shape.outline_xy()
        self.assertEqual(len(ox), CircleSelectionShape.N_OUTLINE)
        self.assertEqual(len(oy), CircleSelectionShape.N_OUTLINE)

    def test_fill_coords_returns_arrays(self):
        fx, fy_bot, fy_top = self.shape.fill_coords()
        self.assertEqual(len(fx), CircleSelectionShape.N_FILL)
        self.assertTrue(np.all(fy_top >= fy_bot))


class TestRectangleSelectionShape(unittest.TestCase):
    def setUp(self):
        self.shape = RectangleSelectionShape(cx=0.5, cy=0.5, half_w=0.1, half_h=0.08)

    def test_init(self):
        self.assertEqual(self.shape.cx, 0.5)
        self.assertEqual(self.shape.cy, 0.5)
        self.assertEqual(self.shape.half_w, 0.1)
        self.assertEqual(self.shape.half_h, 0.08)
        self.assertEqual(self.shape.angle, 0.0)

    def test_hit_test_inside(self):
        self.assertEqual(self.shape.hit_test(0.5, 0.5), "inside")

    def test_hit_test_edge(self):
        hit = self.shape.hit_test(0.5 + 0.1, 0.5)
        self.assertEqual(hit, "edge")

    def test_hit_test_handle(self):
        hx, hy = self.shape._handle_pos()
        self.assertEqual(self.shape.hit_test(hx, hy), "handle")

    def test_hit_test_outside(self):
        self.assertIsNone(self.shape.hit_test(0.0, 0.0))

    def test_indices_in_shape_no_rotation(self):
        proj = np.array([[0.5, 0.5], [0.0, 0.0], [0.55, 0.55]])
        mask = self.shape.indices_in_shape(proj)
        np.testing.assert_array_equal(mask, [True, False, True])

    def test_indices_in_shape_with_rotation(self):
        self.shape.angle = np.pi / 4.0
        # After 45 degree rotation, a point at (0.6, 0.5) is rotated to local coords
        proj = np.array([[0.6, 0.5]])  # offset 0.1 in x, should still be inside
        mask = self.shape.indices_in_shape(proj)
        self.assertTrue(mask[0])

    def test_save_size(self):
        s = self.shape.save_size()
        self.assertEqual(s["half_w"], 0.1)
        self.assertEqual(s["half_h"], 0.08)

    def test_apply_resize_delta(self):
        saved = self.shape.save_size()
        # Expand to larger rectangle by moving local x boundary outward
        self.shape.apply_resize_delta(0.5 + 0.15, 0.5 + 0.12, 0.5 + 0.1, 0.5 + 0.08, saved)
        self.assertAlmostEqual(self.shape.half_w, 0.15, places=5)
        self.assertAlmostEqual(self.shape.half_h, 0.12, places=5)

    def test_corners_no_rotation(self):
        corners = self.shape._corners()
        self.assertEqual(len(corners), 4)
        # Bottom-left and top-right
        np.testing.assert_almost_equal(corners[0], (0.4, 0.42))
        np.testing.assert_almost_equal(corners[2], (0.6, 0.58))

    def test_outline_xy_closed(self):
        ox, oy = self.shape.outline_xy()
        # Outline polyline should be closed (first == last)
        self.assertAlmostEqual(ox[0], ox[-1])
        self.assertAlmostEqual(oy[0], oy[-1])

    def test_fill_coords_returns_arrays(self):
        fx, fy_bot, fy_top = self.shape.fill_coords()
        self.assertGreater(len(fx), 0)
        self.assertTrue(np.all(fy_top >= fy_bot))


class TestEllipseSelectionShape(unittest.TestCase):
    def setUp(self):
        self.shape = EllipseSelectionShape(cx=0.5, cy=0.5, half_a=0.15, half_b=0.1)

    def test_init(self):
        self.assertEqual(self.shape.cx, 0.5)
        self.assertEqual(self.shape.cy, 0.5)
        self.assertEqual(self.shape.half_a, 0.15)
        self.assertEqual(self.shape.half_b, 0.1)
        self.assertEqual(self.shape.angle, 0.0)

    def test_hit_test_inside(self):
        self.assertEqual(self.shape.hit_test(0.5, 0.5), "inside")

    def test_hit_test_edge(self):
        # On the semi-major axis boundary
        hit = self.shape.hit_test(0.5 + 0.15, 0.5)
        self.assertEqual(hit, "edge")

    def test_hit_test_handle(self):
        hx, hy = self.shape._handle_pos()
        self.assertEqual(self.shape.hit_test(hx, hy), "handle")

    def test_hit_test_outside(self):
        self.assertIsNone(self.shape.hit_test(0.0, 0.0))

    def test_indices_in_shape_center(self):
        proj = np.array([[0.5, 0.5]])
        mask = self.shape.indices_in_shape(proj)
        self.assertTrue(mask[0])

    def test_indices_in_shape_outside(self):
        proj = np.array([[0.0, 0.0]])
        mask = self.shape.indices_in_shape(proj)
        self.assertFalse(mask[0])

    def test_indices_in_shape_on_boundary(self):
        # Point just inside the semi-major axis boundary
        proj = np.array([[0.5 + 0.15 - 1e-9, 0.5]])
        mask = self.shape.indices_in_shape(proj)
        self.assertTrue(mask[0])

    def test_indices_in_shape_with_rotation(self):
        self.shape.angle = np.pi / 2.0
        # After 90 degree rotation, half_a (0.15) now extends along y, half_b (0.1) along x
        proj = np.array([[0.5, 0.5 + 0.14]])  # inside along rotated major axis
        mask = self.shape.indices_in_shape(proj)
        self.assertTrue(mask[0])

    def test_indices_in_shape_with_rotation_outside(self):
        self.shape.angle = np.pi / 2.0
        # After 90 degree rotation, x extent is only half_b=0.1
        proj = np.array([[0.5 + 0.12, 0.5]])
        mask = self.shape.indices_in_shape(proj)
        self.assertFalse(mask[0])

    def test_save_size(self):
        s = self.shape.save_size()
        self.assertEqual(s["half_a"], 0.15)
        self.assertEqual(s["half_b"], 0.1)

    def test_apply_resize_delta(self):
        saved = self.shape.save_size()
        self.shape.apply_resize_delta(0.5 + 0.2, 0.5, 0.5 + 0.15, 0.5, saved)
        self.assertAlmostEqual(self.shape.half_a, 0.2, places=5)

    def test_outline_xy_returns_arrays(self):
        ox, oy = self.shape.outline_xy()
        self.assertEqual(len(ox), EllipseSelectionShape.N_OUTLINE)

    def test_fill_coords_returns_arrays(self):
        fx, fy_bot, fy_top = self.shape.fill_coords()
        self.assertGreater(len(fx), 0)
        self.assertTrue(np.all(fy_top >= fy_bot))

    def test_evaluate_function_equivalence_no_rotation(self):
        """Verify that indices_in_shape matches the old ImplicitEllipse.EvaluateFunction approach."""
        # Set up the ellipse in normalised coords
        self.shape.half_a = 0.2
        self.shape.half_b = 0.1
        self.shape.angle = 0.0

        # Interior point
        proj = np.array([[0.5, 0.5]])
        mask = self.shape.indices_in_shape(proj)
        self.assertTrue(mask[0])

        # Boundary point (semi-major axis end)
        proj = np.array([[0.5 + 0.2, 0.5]])
        mask = self.shape.indices_in_shape(proj)
        self.assertTrue(mask[0])

        # Exterior point
        proj = np.array([[0.5 + 0.3, 0.5]])
        mask = self.shape.indices_in_shape(proj)
        self.assertFalse(mask[0])

    def test_evaluate_function_equivalence_with_rotation(self):
        """Verify rotation transforms work correctly for containment."""
        self.shape.half_a = 0.2  # semi-major
        self.shape.half_b = 0.1  # semi-minor
        self.shape.angle = np.pi / 2.0  # 90 deg rotation

        # After 90 degree rotation, semi-major (0.2) extends along y, semi-minor (0.1) along x
        # Point at +0.15 along y: should be inside (0.15 < 0.2)
        proj = np.array([[0.5, 0.5 + 0.15]])
        mask = self.shape.indices_in_shape(proj)
        self.assertTrue(mask[0])

        # Point at +0.15 along x: should be outside (0.15 > 0.1)
        proj = np.array([[0.5 + 0.15, 0.5]])
        mask = self.shape.indices_in_shape(proj)
        self.assertFalse(mask[0])


class TestShapeOverlayManager(unittest.TestCase):
    def _make_mock_plotter(self):
        plotter = mock.MagicMock()
        plotter.window_size = (800, 600)
        plotter.mouse_position = (400, 300)
        plotter.iren = mock.MagicMock()
        plotter.iren.interactor = mock.MagicMock()
        plotter.renderer = mock.MagicMock()
        return plotter

    def test_init(self):
        plotter = self._make_mock_plotter()
        mgr = ShapeOverlayManager(plotter)
        self.assertIsNone(mgr.current_shape)
        self.assertIsNone(mgr._cached_transform)

    def test_set_shape(self):
        plotter = self._make_mock_plotter()
        mgr = ShapeOverlayManager(plotter)
        shape = CircleSelectionShape(0.5, 0.5, 0.1)
        mgr.set_shape(shape)
        self.assertIs(mgr.current_shape, shape)

    def test_remove_shape(self):
        plotter = self._make_mock_plotter()
        mgr = ShapeOverlayManager(plotter)
        shape = CircleSelectionShape(0.5, 0.5, 0.1)
        mgr.set_shape(shape)
        mgr.remove_shape()
        self.assertIsNone(mgr.current_shape)

    def test_get_shape_mask_no_shape(self):
        plotter = self._make_mock_plotter()
        mgr = ShapeOverlayManager(plotter)
        points = np.array([[0.0, 0.0, 0.0], [1.0, 1.0, 1.0]])
        mask = mgr.get_shape_mask(points)
        np.testing.assert_array_equal(mask, [False, False])

    def test_pixel_to_data_uses_cached_transform(self):
        """_pixel_to_data should use cached transform parameters."""
        plotter = self._make_mock_plotter()
        mgr = ShapeOverlayManager(plotter)
        # Simulate a cached PlotTransform: sx=760, sy=560, tx=20, ty=20
        # (data range [0,1] maps to pixels [20, 780] x [20, 580])
        mgr._cached_transform = (760.0, 560.0, 20.0, 20.0)
        # Pixel (400, 300) should map to data ((400-20)/760, (300-20)/560)
        result = mgr._pixel_to_data(400, 300)
        self.assertIsNotNone(result)
        self.assertAlmostEqual(result[0], 380.0 / 760.0, places=6)
        self.assertAlmostEqual(result[1], 280.0 / 560.0, places=6)

    def test_pixel_to_data_fallback_without_cache(self):
        """Without cache or chart, _pixel_to_data falls back to renderer size."""
        plotter = self._make_mock_plotter()
        plotter.renderer.GetSize.return_value = (800, 600)
        mgr = ShapeOverlayManager(plotter)
        result = mgr._pixel_to_data(400, 300)
        self.assertIsNotNone(result)
        self.assertAlmostEqual(result[0], 0.5)
        self.assertAlmostEqual(result[1], 0.5)

    def test_project_points_uses_cached_projection(self):
        """project_points_to_screen uses VTK WorldToDisplay + PlotTransform inverse."""
        plotter = self._make_mock_plotter()
        renderer = plotter.renderer

        # Mock VTK renderer to return a predictable display point.
        renderer.GetDisplayPoint.return_value = (400.0, 300.0, 0.0)
        renderer.SetWorldPoint = MagicMock()
        renderer.WorldToDisplay = MagicMock()

        mgr = ShapeOverlayManager(plotter)
        # PlotTransform maps [0,1] → [0, 800] / [0, 600] — no borders.
        mgr._cached_transform = (800.0, 600.0, 0.0, 0.0)

        # Display (400, 300) → data ((400-0)/800, (300-0)/600) = (0.5, 0.5)
        points = np.array([[0.0, 0.0, 0.0]])
        proj = mgr.project_points_to_screen(points)
        self.assertAlmostEqual(proj[0, 0], 0.5, places=4)
        self.assertAlmostEqual(proj[0, 1], 0.5, places=4)

    def test_project_points_accounts_for_chart_borders(self):
        """PlotTransform offset should shift projected data coords."""
        plotter = self._make_mock_plotter()
        renderer = plotter.renderer

        # Mock VTK renderer to return a predictable display point.
        renderer.GetDisplayPoint.return_value = (400.0, 300.0, 0.0)
        renderer.SetWorldPoint = MagicMock()
        renderer.WorldToDisplay = MagicMock()

        mgr = ShapeOverlayManager(plotter)
        # PlotTransform: data [0,1] maps to pixels [20, 780] × [20, 580]
        # so sx=760, tx=20, sy=560, ty=20
        mgr._cached_transform = (760.0, 560.0, 20.0, 20.0)

        # Display (400, 300) → data ((400-20)/760, (300-20)/560)
        points = np.array([[0.0, 0.0, 0.0]])
        proj = mgr.project_points_to_screen(points)
        self.assertAlmostEqual(proj[0, 0], 380.0 / 760.0, places=4)
        self.assertAlmostEqual(proj[0, 1], 280.0 / 560.0, places=4)

    def test_get_shape_mask_uses_cached_projected_points(self):
        """get_shape_mask prefers pre-projected VTK cache when available."""
        plotter = self._make_mock_plotter()
        mgr = ShapeOverlayManager(plotter)

        # Install a simple circle shape at (0.5, 0.5) with radius 0.2
        shape = CircleSelectionShape(0.5, 0.5, 0.1)
        mgr._shape = shape

        # Provide pre-projected points (simulating project_and_cache_points)
        # Point at (0.5, 0.5) is at centre → inside
        # Point at (0.9, 0.9) is far from centre → outside
        mgr._cached_projected_points = np.array([[0.5, 0.5], [0.9, 0.9]])

        world_pts = np.array([[0.0, 0.0, 0.0], [1.0, 1.0, 1.0]])
        mask = mgr.get_shape_mask(world_pts)
        np.testing.assert_array_equal(mask, [True, False])

        # Cache should be consumed (cleared after single use)
        self.assertIsNone(mgr._cached_projected_points)

    def test_project_and_cache_points_stores_projected_data(self):
        """project_and_cache_points populates _cached_projected_points."""
        plotter = self._make_mock_plotter()
        renderer = plotter.renderer

        # Make WorldToDisplay return predictable display coords.
        # We'll track what SetWorldPoint receives and return fixed values.
        display_returns = iter([(400.0, 300.0, 0.0), (100.0, 500.0, 0.0)])
        renderer.GetDisplayPoint = MagicMock(side_effect=display_returns)
        renderer.SetWorldPoint = MagicMock()
        renderer.WorldToDisplay = MagicMock()
        renderer.GetSize.return_value = (800, 600)

        mgr = ShapeOverlayManager(plotter)
        # No PlotTransform → falls back to dividing by renderer size
        mgr._cached_transform = None

        world_pts = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]])
        mgr.project_and_cache_points(world_pts)

        self.assertIsNotNone(mgr._cached_projected_points)
        self.assertEqual(mgr._cached_projected_points.shape, (2, 2))
        # (400/800, 300/600) = (0.5, 0.5)
        self.assertAlmostEqual(mgr._cached_projected_points[0, 0], 0.5)
        self.assertAlmostEqual(mgr._cached_projected_points[0, 1], 0.5)
        # (100/800, 500/600)
        self.assertAlmostEqual(mgr._cached_projected_points[1, 0], 0.125)
        self.assertAlmostEqual(mgr._cached_projected_points[1, 1], 500.0 / 600.0)


class TestAnnulusSelectionShape(unittest.TestCase):
    def setUp(self):
        self.shape = AnnulusSelectionShape(cx=0.5, cy=0.5, inner_radius=0.06, outer_radius=0.15)

    def test_init(self):
        self.assertEqual(self.shape.cx, 0.5)
        self.assertEqual(self.shape.cy, 0.5)
        self.assertEqual(self.shape.inner_radius, 0.06)
        self.assertEqual(self.shape.outer_radius, 0.15)

    def test_hit_test_inside_ring(self):
        # Point halfway between inner and outer radius
        self.assertEqual(self.shape.hit_test(0.5 + 0.10, 0.5), "inside")

    def test_hit_test_outer_edge(self):
        self.assertEqual(self.shape.hit_test(0.5 + 0.15, 0.5), "edge")

    def test_hit_test_inner_edge(self):
        self.assertEqual(self.shape.hit_test(0.5 + 0.06, 0.5), "inner_edge")

    def test_hit_test_inside_hole(self):
        # Inside the inner circle (the hole) — not part of the annulus
        self.assertIsNone(self.shape.hit_test(0.5, 0.5))

    def test_hit_test_outside(self):
        self.assertIsNone(self.shape.hit_test(0.0, 0.0))

    def test_indices_in_shape_in_ring(self):
        proj = np.array([[0.5 + 0.10, 0.5]])
        mask = self.shape.indices_in_shape(proj)
        self.assertTrue(mask[0])

    def test_indices_in_shape_in_hole(self):
        proj = np.array([[0.5, 0.5]])
        mask = self.shape.indices_in_shape(proj)
        self.assertFalse(mask[0])

    def test_indices_in_shape_outside(self):
        proj = np.array([[0.0, 0.0]])
        mask = self.shape.indices_in_shape(proj)
        self.assertFalse(mask[0])

    def test_indices_in_shape_multiple(self):
        proj = np.array(
            [
                [0.5 + 0.10, 0.5],  # in ring
                [0.5, 0.5],  # in hole
                [0.0, 0.0],  # outside
                [0.5, 0.5 + 0.07],  # in ring
            ]
        )
        mask = self.shape.indices_in_shape(proj)
        np.testing.assert_array_equal(mask, [True, False, False, True])

    def test_save_size(self):
        s = self.shape.save_size()
        self.assertEqual(s["inner_radius"], 0.06)
        self.assertEqual(s["outer_radius"], 0.15)

    def test_apply_resize_delta_outer(self):
        saved = self.shape.save_size()
        # Start near outer edge, drag outward
        self.shape.apply_resize_delta(
            0.5 + 0.20,
            0.5,  # current
            0.5 + 0.15,
            0.5,  # start (on outer edge)
            saved,
        )
        self.assertAlmostEqual(self.shape.outer_radius, 0.20, places=5)
        # Inner should be unchanged
        self.assertAlmostEqual(self.shape.inner_radius, 0.06, places=5)

    def test_apply_resize_delta_inner(self):
        saved = self.shape.save_size()
        # Start near inner edge, drag inward
        self.shape.apply_resize_delta(
            0.5 + 0.04,
            0.5,  # current
            0.5 + 0.06,
            0.5,  # start (on inner edge)
            saved,
        )
        self.assertAlmostEqual(self.shape.inner_radius, 0.04, places=5)
        # Outer should be unchanged
        self.assertAlmostEqual(self.shape.outer_radius, 0.15, places=5)

    def test_apply_resize_delta_inner_clamps_to_min(self):
        saved = self.shape.save_size()
        # Drag inner edge inward past minimum
        self.shape.apply_resize_delta(
            0.5,
            0.5,  # current (at center)
            0.5 + 0.06,
            0.5,  # start (on inner edge)
            saved,
        )
        self.assertEqual(self.shape.inner_radius, 0.01)

    def test_apply_resize_delta_outer_clamps_above_inner(self):
        saved = self.shape.save_size()
        # Drag outer edge inward past inner
        self.shape.apply_resize_delta(
            0.5 + 0.05,
            0.5,  # current
            0.5 + 0.15,
            0.5,  # start (on outer edge)
            saved,
        )
        self.assertEqual(self.shape.outer_radius, self.shape.inner_radius + 0.01)

    def test_outline_xy_has_nan_separator(self):
        ox, oy = self.shape.outline_xy()
        # Should have a NaN separator between outer and inner circles
        self.assertTrue(np.any(np.isnan(ox)))
        self.assertTrue(np.any(np.isnan(oy)))

    def test_fill_coords_returns_arrays(self):
        fx, fy_bot, fy_top = self.shape.fill_coords()
        self.assertGreater(len(fx), 0)
        self.assertTrue(np.all(fy_top >= fy_bot))


if __name__ == "__main__":
    unittest.main()
