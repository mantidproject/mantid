from instrumentview.InteractorStyles import CursorZoomInteractorStyle, SwappedButtonTrackballCamera, _display_to_world
import unittest
from unittest import mock

import numpy as np
from numpy.testing import assert_array_almost_equal


def _make_mock_plotter(position=(0, 0, 1), focal_point=(0, 0, 0), parallel_scale=1.0):
    """Create a mock plotter with a camera that behaves like a real one."""
    camera = mock.MagicMock()
    camera.position = list(position)
    camera.focal_point = list(focal_point)
    camera.parallel_scale = parallel_scale

    renderer = mock.MagicMock()
    renderer.camera = camera

    plotter = mock.MagicMock()
    plotter.renderer = renderer
    plotter.mouse_position = (100, 100)
    plotter.render_window = mock.MagicMock()
    return plotter


class TestCursorZoomInteractorStyle(unittest.TestCase):
    def _create_style(self, **plotter_kwargs):
        plotter = _make_mock_plotter(**plotter_kwargs)
        style = CursorZoomInteractorStyle(plotter)
        return style, plotter

    def test_caches_default_camera_state(self):
        style, plotter = self._create_style(position=(1, 2, 3), focal_point=(4, 5, 6), parallel_scale=2.5)
        assert_array_almost_equal(style._default_position, [1, 2, 3])
        assert_array_almost_equal(style._default_focal_point, [4, 5, 6])
        self.assertAlmostEqual(style._default_parallel_scale, 2.5)

    def test_track_mouse_position_called(self):
        style, plotter = self._create_style()
        plotter.track_mouse_position.assert_called_once()

    def test_on_mouse_move_updates_cursor_world_pos(self):
        style, plotter = self._create_style()
        plotter.mouse_position = (50, 60)
        # Mock _display_to_world to return a known value
        with mock.patch("instrumentview.InteractorStyles._display_to_world", return_value=np.array([1.0, 2.0, 0.0])):
            style._on_mouse_move(None, None)
        assert_array_almost_equal(style._cursor_world_pos, [1.0, 2.0, 0.0])

    def test_on_mouse_move_skipped_during_zoom(self):
        style, plotter = self._create_style()
        style._zoom_in_progress = True
        style._cursor_world_pos = None
        style._on_mouse_move(None, None)
        self.assertIsNone(style._cursor_world_pos)

    def test_zoom_does_nothing_when_no_cursor_pos(self):
        style, plotter = self._create_style()
        style._cursor_world_pos = None
        camera = plotter.renderer.camera
        original_scale = camera.parallel_scale
        style._zoom(1.1)
        # Camera should not have been modified
        self.assertEqual(camera.parallel_scale, original_scale)

    def test_zoom_in_decreases_parallel_scale(self):
        style, plotter = self._create_style(parallel_scale=1.0)
        style._cursor_world_pos = np.array([0.0, 0.0, 0.0])
        with mock.patch("instrumentview.InteractorStyles._display_to_world", return_value=np.array([0.0, 0.0, 0.0])):
            style._zoom(style.zoom_factor)
        camera = plotter.renderer.camera
        self.assertLess(camera.parallel_scale, 1.0)

    def test_zoom_out_past_default_resets_camera(self):
        style, plotter = self._create_style(position=(0, 0, 1), focal_point=(0, 0, 0), parallel_scale=1.0)
        # Set camera to a state close to default so one zoom-out step exceeds it
        plotter.renderer.camera.parallel_scale = 0.9
        plotter.renderer.camera.focal_point = [0.1, 0.1, 0.0]
        plotter.renderer.camera.position = [0.1, 0.1, 1.0]
        style._cursor_world_pos = np.array([0.0, 0.0, 0.0])
        # Zoom out — the dynamic factor will push parallel_scale past the default
        with mock.patch("instrumentview.InteractorStyles._display_to_world", return_value=np.array([0.0, 0.0, 0.0])):
            style._zoom(1.0 / style.zoom_factor)
        camera = plotter.renderer.camera
        # Should have reset to defaults
        self.assertEqual(camera.position, [0, 0, 1])
        self.assertEqual(camera.focal_point, [0, 0, 0])
        self.assertAlmostEqual(camera.parallel_scale, 1.0)

    def test_reset_camera_restores_defaults(self):
        style, plotter = self._create_style(position=(1, 2, 3), focal_point=(4, 5, 6), parallel_scale=2.5)
        # Modify camera state
        plotter.renderer.camera.position = [9, 9, 9]
        plotter.renderer.camera.focal_point = [8, 8, 8]
        plotter.renderer.camera.parallel_scale = 99.0
        style._reset_camera()
        camera = plotter.renderer.camera
        self.assertEqual(camera.position, [1, 2, 3])
        self.assertEqual(camera.focal_point, [4, 5, 6])
        self.assertAlmostEqual(camera.parallel_scale, 2.5)

    def test_wheel_forward_calls_zoom_in(self):
        style, plotter = self._create_style()
        with mock.patch.object(style, "_zoom") as zoom_mock:
            style._on_wheel_forward(None, None)
            zoom_mock.assert_called_once_with(style.zoom_factor)

    def test_wheel_backward_calls_zoom_out(self):
        style, plotter = self._create_style()
        with mock.patch.object(style, "_zoom") as zoom_mock:
            style._on_wheel_backward(None, None)
            zoom_mock.assert_called_once_with(1.0 / style.zoom_factor)


class TestDisplayToWorld(unittest.TestCase):
    def test_returns_none_for_zero_w(self):
        renderer = mock.MagicMock()
        renderer.GetWorldPoint.return_value = (1.0, 2.0, 0.0, 0.0)
        result = _display_to_world(renderer, 100, 200)
        self.assertIsNone(result)

    def test_converts_display_to_world(self):
        renderer = mock.MagicMock()
        renderer.GetWorldPoint.return_value = (2.0, 4.0, 0.0, 2.0)
        result = _display_to_world(renderer, 100, 200)
        renderer.SetDisplayPoint.assert_called_with(100, 200, 0.0)
        renderer.DisplayToWorld.assert_called_once()
        assert_array_almost_equal(result, [1.0, 2.0, 0.0])

    def test_projects_onto_z_zero_plane(self):
        renderer = mock.MagicMock()
        renderer.GetWorldPoint.return_value = (3.0, 6.0, 9.0, 3.0)
        result = _display_to_world(renderer, 50, 50)
        # z component should always be 0
        self.assertAlmostEqual(result[2], 0.0)


class TestSwappedButtonTrackballCamera(unittest.TestCase):
    def test_instantiates(self):
        style = SwappedButtonTrackballCamera()
        self.assertIsNotNone(style)
