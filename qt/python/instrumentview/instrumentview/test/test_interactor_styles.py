from instrumentview.InteractorStyles import (
    CursorZoomInteractorStyle,
    FlatProjectionInteractorStyle,
    InteractorStyles,
    RubberBandZoomInteractorStyle,
    ThreeDInteractorStyle,
)
import unittest
from unittest import mock
from vtkmodules.vtkCommonCore import vtkCommand

import numpy as np
from numpy.testing import assert_array_almost_equal
from vtkmodules.vtkRenderingCore import vtkRenderer, vtkRenderWindow, vtkRenderWindowInteractor


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


class _PlotterWithRealRenderer:
    """The parts of a PyVista plotter the cursor zoom styles use, around a real VTK renderer so the camera maths is real."""

    def __init__(self, renderer):
        self.renderer = renderer
        self.track_mouse_position = mock.MagicMock()


class _CursorZoomStyleTestBase(unittest.TestCase):
    # The full view: looking at the origin at an angle, as after rotating the 3D view
    FULL_VIEW_POSITION = (3, 4, 10)
    FULL_VIEW_SCALE = 5

    def _create_style(self, style_class=CursorZoomInteractorStyle, parallel_projection=True, position=None, full_view_scale=None):
        """Attach a style to a real renderer and camera. The interactor is never initialised, so nothing is drawn.

        The camera state when the style is created is the full view it stops at when zooming out.
        """
        render_window = vtkRenderWindow()
        render_window.SetSize(200, 100)
        renderer = vtkRenderer()
        render_window.AddRenderer(renderer)
        interactor = vtkRenderWindowInteractor()
        interactor.SetRenderWindow(render_window)

        camera = renderer.GetActiveCamera()
        camera.SetParallelProjection(parallel_projection)
        camera.SetPosition(*(position or self.FULL_VIEW_POSITION))
        camera.SetFocalPoint(0, 0, 0)
        camera.SetParallelScale(full_view_scale or self.FULL_VIEW_SCALE)

        plotter = _PlotterWithRealRenderer(renderer)
        style = style_class(plotter)
        interactor.SetInteractorStyle(style)
        # Keep references so the VTK objects outlive this method
        self._render_window, self._interactor = render_window, interactor
        return style, renderer, interactor

    @staticmethod
    def _display_point(renderer, world_point):
        renderer.SetWorldPoint(*world_point, 1.0)
        renderer.WorldToDisplay()
        return np.array(renderer.GetDisplayPoint()[:2])

    @staticmethod
    def _world_point_under_cursor(renderer, x, y):
        camera = renderer.GetActiveCamera()
        renderer.SetWorldPoint(*camera.GetFocalPoint(), 1.0)
        renderer.WorldToDisplay()
        renderer.SetDisplayPoint(x, y, renderer.GetDisplayPoint()[2])
        renderer.DisplayToWorld()
        wx, wy, wz, ww = renderer.GetWorldPoint()
        return np.array([wx, wy, wz]) / ww

    def _zoom_at(self, style, interactor, forward, x=150, y=75):
        interactor.SetEventPosition(x, y)
        style._zoom(forward=forward)


class TestCursorZoomInteractorStyle(_CursorZoomStyleTestBase):
    def test_caches_default_camera_state(self):
        style, _, _ = self._create_style()
        assert_array_almost_equal(style._default_focal_point, [0, 0, 0])
        self.assertAlmostEqual(style._default_parallel_scale, self.FULL_VIEW_SCALE)

    def test_update_default_camera_state_recaches_current_state(self):
        style, renderer, _ = self._create_style()
        # Simulate the camera changing after construction (e.g. fill transform)
        camera = renderer.GetActiveCamera()
        camera.SetPosition(7, 8, 9)
        camera.SetFocalPoint(1, 2, 3)
        camera.SetParallelScale(2.5)
        style.update_default_camera_state()
        assert_array_almost_equal(style._default_focal_point, [1, 2, 3])
        self.assertAlmostEqual(style._default_parallel_scale, 2.5)

    def test_track_mouse_position_called(self):
        style, _, _ = self._create_style()
        style.plotter.track_mouse_position.assert_called_once()

    def test_trackball_button_actions_are_switched_off(self):
        style, _, _ = self._create_style()
        for event in (
            vtkCommand.LeftButtonPressEvent,
            vtkCommand.LeftButtonReleaseEvent,
            vtkCommand.MiddleButtonPressEvent,
            vtkCommand.MiddleButtonReleaseEvent,
            vtkCommand.RightButtonPressEvent,
            vtkCommand.RightButtonReleaseEvent,
        ):
            # An observer stops the trackball rotating, panning or zooming with that button
            self.assertTrue(style.HasObserver(event))

    def test_wheel_events_call_zoom(self):
        style, _, _ = self._create_style()
        with mock.patch.object(style, "_zoom") as zoom_mock:
            style.InvokeEvent(vtkCommand.MouseWheelForwardEvent)
            zoom_mock.assert_called_once_with(forward=True)
            zoom_mock.reset_mock()
            style.InvokeEvent(vtkCommand.MouseWheelBackwardEvent)
            zoom_mock.assert_called_once_with(forward=False)

    def test_zoom_keeps_point_under_cursor_fixed(self):
        for forward in (True, False):
            with self.subTest(forward=forward):
                style, renderer, interactor = self._create_style()
                # Zoomed in from the full view, so zooming out does not reach it
                renderer.GetActiveCamera().SetParallelScale(1)
                point = self._world_point_under_cursor(renderer, 150, 75)
                self._zoom_at(style, interactor, forward)
                assert_array_almost_equal(self._display_point(renderer, point), [150, 75])

    def test_zoom_keeps_view_direction(self):
        style, renderer, interactor = self._create_style()
        camera = renderer.GetActiveCamera()
        direction_before = np.array(camera.GetDirectionOfProjection())
        self._zoom_at(style, interactor, forward=True)
        assert_array_almost_equal(camera.GetDirectionOfProjection(), direction_before)

    def test_first_zoom_step_from_full_view_does_not_depend_on_world_units(self):
        # e.g. side by side projections are in metres, spherical projections in radians
        for full_view_scale in (0.2, 3.0):
            with self.subTest(full_view_scale=full_view_scale):
                style, renderer, interactor = self._create_style(full_view_scale=full_view_scale)
                self._zoom_at(style, interactor, forward=True)
                self.assertAlmostEqual(renderer.GetActiveCamera().GetParallelScale(), full_view_scale / style._FULL_VIEW_ZOOM_STEP)

    def test_zoom_step_gets_smaller_when_zoomed_in(self):
        style, renderer, interactor = self._create_style()
        # Zoomed in 4x from the full view
        renderer.GetActiveCamera().SetParallelScale(self.FULL_VIEW_SCALE / 4)
        self._zoom_at(style, interactor, forward=True)
        expected_step = 1.0 + (style._FULL_VIEW_ZOOM_STEP - 1.0) / 4
        self.assertAlmostEqual(renderer.GetActiveCamera().GetParallelScale(), self.FULL_VIEW_SCALE / 4 / expected_step)

    def test_zoom_out_past_full_view_returns_to_full_view_keeping_rotation(self):
        style, renderer, interactor = self._create_style()
        camera = renderer.GetActiveCamera()
        # Zoomed in a little, off centre, and rotated to look along a different direction
        camera.SetFocalPoint(1, 1, 0)
        camera.SetPosition(1, 11, 0)
        camera.SetParallelScale(4.5)
        direction_before = np.array(camera.GetDirectionOfProjection())

        self._zoom_at(style, interactor, forward=False)

        self.assertAlmostEqual(camera.GetParallelScale(), self.FULL_VIEW_SCALE)
        assert_array_almost_equal(camera.GetFocalPoint(), [0, 0, 0])
        assert_array_almost_equal(camera.GetDirectionOfProjection(), direction_before)

    def test_zoom_out_past_full_view_without_rotation_returns_to_the_full_view_camera(self):
        """In the flat projections the camera never rotates, so this is a full reset of the view."""
        style, renderer, interactor = self._create_style(position=(0, 0, 10))
        camera = renderer.GetActiveCamera()
        camera.SetFocalPoint(1, 1, 0)
        camera.SetPosition(1, 1, 10)
        camera.SetParallelScale(4.5)

        self._zoom_at(style, interactor, forward=False)

        assert_array_almost_equal(camera.GetPosition(), [0, 0, 10])
        assert_array_almost_equal(camera.GetFocalPoint(), [0, 0, 0])
        self.assertAlmostEqual(camera.GetParallelScale(), self.FULL_VIEW_SCALE)

    def test_zoom_out_within_full_view_zooms_at_cursor(self):
        style, renderer, interactor = self._create_style()
        camera = renderer.GetActiveCamera()
        camera.SetParallelScale(2)
        point = self._world_point_under_cursor(renderer, 150, 75)
        self._zoom_at(style, interactor, forward=False)
        self.assertGreater(camera.GetParallelScale(), 2)
        self.assertLess(camera.GetParallelScale(), self.FULL_VIEW_SCALE)
        assert_array_almost_equal(self._display_point(renderer, point), [150, 75])

    def test_zoom_with_perspective_projection_uses_trackball_zoom(self):
        style, renderer, interactor = self._create_style(parallel_projection=False)
        camera = renderer.GetActiveCamera()
        self._zoom_at(style, interactor, forward=True)
        # The trackball zoom moves the camera towards the focal point, which stays where it was
        assert_array_almost_equal(camera.GetFocalPoint(), [0, 0, 0])
        self.assertLess(camera.GetDistance(), np.linalg.norm(self.FULL_VIEW_POSITION))

    def test_zoom_without_interactor_does_nothing(self):
        render_window = vtkRenderWindow()
        renderer = vtkRenderer()
        render_window.AddRenderer(renderer)
        renderer.GetActiveCamera().SetParallelScale(5)
        style = CursorZoomInteractorStyle(_PlotterWithRealRenderer(renderer))
        style._zoom(forward=True)
        self.assertAlmostEqual(renderer.GetActiveCamera().GetParallelScale(), 5)

    def test_zoom_notifies_camera_changed(self):
        style, _, interactor = self._create_style()
        callback = mock.MagicMock()
        style.set_camera_changed_callback(callback)
        self._zoom_at(style, interactor, forward=True)
        callback.assert_called_once()

    def test_zoom_out_past_full_view_notifies_camera_changed_only_once(self):
        style, renderer, interactor = self._create_style()
        renderer.GetActiveCamera().SetParallelScale(4.5)
        callback = mock.MagicMock()
        style.set_camera_changed_callback(callback)
        self._zoom_at(style, interactor, forward=False)
        callback.assert_called_once()

    def test_reset_camera_returns_to_full_view_keeping_rotation(self):
        style, renderer, _ = self._create_style()
        camera = renderer.GetActiveCamera()
        # Zoomed in, off centre, and rotated to look along a different direction
        camera.SetFocalPoint(8, 8, 8)
        camera.SetPosition(8, 18, 8)
        camera.SetParallelScale(1)
        direction_before = np.array(camera.GetDirectionOfProjection())
        style._reset_camera()
        assert_array_almost_equal(camera.GetFocalPoint(), [0, 0, 0])
        self.assertAlmostEqual(camera.GetParallelScale(), self.FULL_VIEW_SCALE)
        assert_array_almost_equal(camera.GetDirectionOfProjection(), direction_before)

    def test_update_default_camera_state_affects_subsequent_reset(self):
        style, renderer, _ = self._create_style()
        camera = renderer.GetActiveCamera()
        camera.SetFocalPoint(1, 2, 3)
        camera.SetParallelScale(2.5)
        style.update_default_camera_state()
        camera.SetFocalPoint(9, 9, 9)
        camera.SetParallelScale(99)
        style._reset_camera()
        assert_array_almost_equal(camera.GetFocalPoint(), [1, 2, 3])
        self.assertAlmostEqual(camera.GetParallelScale(), 2.5)

    def test_reset_camera_and_notify_notifies_camera_changed(self):
        style, _, _ = self._create_style()
        callback = mock.MagicMock()
        style.set_camera_changed_callback(callback)
        style._reset_camera_and_notify()
        callback.assert_called_once()

    def test_camera_changed_callback_exceptions_do_not_escape_into_vtk(self):
        style, _, _ = self._create_style()
        style.set_camera_changed_callback(mock.MagicMock(side_effect=RuntimeError("boom")))
        style._notify_camera_changed()

    def test_set_picking_callback_replaces_left_button_action(self):
        style, _, _ = self._create_style()
        callback = mock.MagicMock()
        style.set_picking_callback(callback)
        style.InvokeEvent(vtkCommand.LeftButtonPressEvent)
        callback.assert_called_once()

    def test_remove_observers_releases_callbacks_and_plotter(self):
        style, _, _ = self._create_style()
        style.set_camera_changed_callback(mock.MagicMock())
        style.remove_observers()
        self.assertFalse(style.HasObserver(vtkCommand.MouseWheelForwardEvent))
        self.assertIsNone(style.plotter)
        self.assertIsNone(style._camera_changed_callback)


class TestFlatProjectionInteractorStyle(_CursorZoomStyleTestBase):
    def test_right_click_resets_the_view_and_notifies(self):
        style, renderer, _ = self._create_style(FlatProjectionInteractorStyle)
        callback = mock.MagicMock()
        style.set_camera_changed_callback(callback)
        renderer.GetActiveCamera().SetParallelScale(1)
        style.InvokeEvent(vtkCommand.RightButtonPressEvent)
        self.assertAlmostEqual(renderer.GetActiveCamera().GetParallelScale(), self.FULL_VIEW_SCALE)
        callback.assert_called_once()

    def test_set_hover_callback_is_called_on_mouse_move(self):
        style, _, _ = self._create_style(FlatProjectionInteractorStyle)
        hover_callback = mock.MagicMock()
        style.set_hover_callback(hover_callback)
        style.InvokeEvent(vtkCommand.MouseMoveEvent)
        hover_callback.assert_called_once()


class TestRubberBandZoomInteractorStyle(unittest.TestCase):
    def _create_style(self, **plotter_kwargs):
        plotter = _make_mock_plotter(**plotter_kwargs)
        style = RubberBandZoomInteractorStyle(plotter)
        return style, plotter

    def _create_style_with_super_spy(self, **plotter_kwargs):
        parent_calls = []

        class _RubberBandZoomSuperSpy(RubberBandZoomInteractorStyle.__bases__[0]):
            def OnLeftButtonDown(self):
                parent_calls.append("left-down")

            def OnMouseMove(self):
                parent_calls.append("mouse-move")

            def OnLeftButtonUp(self):
                parent_calls.append("left-up")

        class _RubberBandZoomStyleSpy(RubberBandZoomInteractorStyle, _RubberBandZoomSuperSpy):
            pass

        plotter = _make_mock_plotter(**plotter_kwargs)
        style = _RubberBandZoomStyleSpy(plotter)
        return style, plotter, parent_calls

    def test_set_picking_callback_stores_callback(self):
        style, _ = self._create_style()
        callback = mock.MagicMock()

        style.set_picking_callback(callback)

        self.assertIs(style._picking_callback, callback)

    def test_left_press_calls_picking_callback_when_modifier_pressed(self):
        style, _ = self._create_style()
        callback = mock.MagicMock()
        style.set_picking_callback(callback)

        with mock.patch.object(style, "_modifier_key_pressed", return_value=True):
            style._on_left_button_press_event("obj", "event")

        callback.assert_called_once_with("obj", "event")
        self.assertTrue(style._ignore_rubberband_interaction)

    def test_left_press_with_modifier_and_no_callback_does_not_raise(self):
        style, _ = self._create_style()

        with mock.patch.object(style, "_modifier_key_pressed", return_value=True):
            style._on_left_button_press_event("obj", "event")

        self.assertTrue(style._ignore_rubberband_interaction)

    def test_left_press_without_modifier_delegates_to_rubberband_zoom(self):
        style, _, parent_calls = self._create_style_with_super_spy()

        with mock.patch.object(style, "_modifier_key_pressed", return_value=False):
            style._on_left_button_press_event("obj", "event")

        self.assertEqual(parent_calls, ["left-down"])
        self.assertFalse(style._ignore_rubberband_interaction)

    def test_mouse_move_without_ignore_delegates_to_rubberband_zoom(self):
        style, _, parent_calls = self._create_style_with_super_spy()

        style._on_mouse_move_event("obj", "event")

        self.assertEqual(parent_calls, ["mouse-move"])

    def test_left_button_release_without_ignore_delegates_to_rubberband_zoom(self):
        style, _, parent_calls = self._create_style_with_super_spy()

        style._on_left_button_release_event("obj", "event")

        self.assertEqual(parent_calls, ["left-up"])
        self.assertFalse(style._ignore_rubberband_interaction)

    def test_left_button_release_clears_ignore_state(self):
        style, _ = self._create_style()
        style._ignore_rubberband_interaction = True

        style._on_left_button_release_event(None, None)

        self.assertFalse(style._ignore_rubberband_interaction)

    def test_rubber_band_actor_added_to_renderer(self):
        style, plotter = self._create_style()
        plotter.renderer.AddActor2D.assert_called_once_with(style._rubber_band_actor)

    def test_left_press_without_modifier_shows_rubber_band_at_click_position(self):
        style, _, _ = self._create_style_with_super_spy()

        with mock.patch.object(style, "_modifier_key_pressed", return_value=False):
            with mock.patch.object(style, "_event_position", return_value=(10, 20)):
                style._on_left_button_press_event("obj", "event")

        self.assertTrue(style._rubber_band_actor.GetVisibility())
        self.assertEqual(style._rubber_band_start, (10, 20))
        assert_array_almost_equal(style._rubber_band_poly.points[0], [10, 20, 0])

    def test_left_press_with_modifier_does_not_show_rubber_band(self):
        style, _ = self._create_style()

        with mock.patch.object(style, "_modifier_key_pressed", return_value=True):
            style._on_left_button_press_event("obj", "event")

        self.assertFalse(style._rubber_band_actor.GetVisibility())
        self.assertIsNone(style._rubber_band_start)

    def test_mouse_move_updates_rubber_band_points_and_renders(self):
        style, _, _ = self._create_style_with_super_spy()
        style._rubber_band_start = (10, 20)

        with mock.patch.object(style, "_event_position", return_value=(30, 40)):
            style._on_mouse_move_event("obj", "event")

        assert_array_almost_equal(style._rubber_band_poly.points[0], [10, 20, 0])
        assert_array_almost_equal(style._rubber_band_poly.points[1], [30, 20, 0])
        assert_array_almost_equal(style._rubber_band_poly.points[2], [30, 40, 0])
        assert_array_almost_equal(style._rubber_band_poly.points[3], [10, 40, 0])

    def test_left_button_release_hides_rubber_band_and_renders(self):
        style, plotter, _ = self._create_style_with_super_spy()
        style._rubber_band_start = (10, 20)
        style._rubber_band_actor.SetVisibility(True)

        style._on_left_button_release_event("obj", "event")

        self.assertFalse(style._rubber_band_actor.GetVisibility())
        self.assertIsNone(style._rubber_band_start)
        plotter.render_window.Render.assert_called_once()

    def test_left_button_release_with_modifier_leaves_rubber_band_untouched(self):
        style, _ = self._create_style()
        style._ignore_rubberband_interaction = True
        style._rubber_band_start = (10, 20)
        style._rubber_band_actor.SetVisibility(True)

        style._on_left_button_release_event(None, None)

        self.assertTrue(style._rubber_band_actor.GetVisibility())
        self.assertEqual(style._rubber_band_start, (10, 20))


class TestThreeDInteractorStyle(_CursorZoomStyleTestBase):
    def test_buttons_are_swapped_for_the_trackball(self):
        style, _, _ = self._create_style(ThreeDInteractorStyle)
        cases = (
            # Right-drag rotates, which is the trackball's left button
            (vtkCommand.RightButtonPressEvent, "OnLeftButtonDown"),
            (vtkCommand.RightButtonReleaseEvent, "OnLeftButtonUp"),
            (vtkCommand.LeftButtonPressEvent, "OnRightButtonDown"),
            (vtkCommand.LeftButtonReleaseEvent, "OnRightButtonUp"),
            (vtkCommand.MiddleButtonPressEvent, "OnMiddleButtonDown"),
            (vtkCommand.MiddleButtonReleaseEvent, "OnMiddleButtonUp"),
        )
        for event, trackball_action in cases:
            with self.subTest(trackball_action=trackball_action):
                with mock.patch.object(style, trackball_action) as action_mock:
                    style.InvokeEvent(event)
                    action_mock.assert_called_once()

    def test_set_picking_callback_replaces_left_button_action(self):
        style, _, _ = self._create_style(ThreeDInteractorStyle)
        callback = mock.MagicMock()
        style.set_picking_callback(callback)
        with mock.patch.object(style, "OnRightButtonDown") as zoom_drag_mock:
            style.InvokeEvent(vtkCommand.LeftButtonPressEvent)
        callback.assert_called_once()
        zoom_drag_mock.assert_not_called()


class TestInteractorStylesCleanup(unittest.TestCase):
    def test_cleanup_releases_callbacks_and_plotter_from_every_style(self):
        styles = InteractorStyles(
            _make_mock_plotter(),
            picking_callback=mock.MagicMock(),
            hover_callback=mock.MagicMock(),
            camera_changed_callback=mock.MagicMock(),
        )

        styles.cleanup()

        all_styles = (
            styles.SCROLL_ZOOM_WITH_PICKING,
            styles.SCROLL_ZOOM_WITH_HOVER,
            styles.SCROLL_ZOOM_NO_PICKING,
            styles.TRACKBALL,
            styles.RUBBERBAND_ZOOM,
        )
        for style in all_styles:
            self.assertFalse(style.HasObserver(vtkCommand.LeftButtonPressEvent))
            self.assertFalse(style.HasObserver(vtkCommand.MouseMoveEvent))
            self.assertFalse(style.HasObserver(vtkCommand.RightButtonPressEvent))
        for style in (styles.SCROLL_ZOOM_WITH_PICKING, styles.SCROLL_ZOOM_WITH_HOVER, styles.SCROLL_ZOOM_NO_PICKING):
            self.assertIsNone(style.plotter)
            self.assertIsNone(style._camera_changed_callback)
        self.assertIsNone(styles.RUBBERBAND_ZOOM.plotter)
        self.assertIsNone(styles.RUBBERBAND_ZOOM._picking_callback)
