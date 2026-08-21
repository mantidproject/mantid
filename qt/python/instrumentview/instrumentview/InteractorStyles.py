from typing import Callable
from vtkmodules.vtkInteractionStyle import vtkInteractorStyleUser, vtkInteractorStyleTrackballCamera, vtkInteractorStyleRubberBandZoom
from vtkmodules.vtkCommonCore import vtkCommand
import numpy as np

from mantid.kernel import logger


class _PlotterWrapper:
    """Wrapper to provide PyVista-compatible interface for picking."""

    def __init__(self, plotter):
        self._plotter = plotter
        super().__init__()


class InteractorStyles:
    def __init__(self, plotter, picking_callback, hover_callback, camera_changed_callback: Callable | None = None):
        self.SCROLL_ZOOM_WITH_PICKING = CursorZoomInteractorStyle(plotter)
        self.SCROLL_ZOOM_WITH_HOVER = CursorZoomInteractorStyle(plotter)
        self.SCROLL_ZOOM_NO_PICKING = CursorZoomInteractorStyle(plotter)
        self.TRACKBALL = SwappedButtonTrackballCamera()
        self.RUBBERBAND_ZOOM = RubberBandZoomInteractorStyle(plotter)

        self.TRACKBALL.set_picking_callback(picking_callback)
        self.SCROLL_ZOOM_WITH_PICKING.set_picking_callback(picking_callback)
        self.RUBBERBAND_ZOOM.set_picking_callback(picking_callback)
        self.SCROLL_ZOOM_WITH_HOVER.set_hover_callback(hover_callback)

        for style in (self.SCROLL_ZOOM_WITH_PICKING, self.SCROLL_ZOOM_WITH_HOVER, self.SCROLL_ZOOM_NO_PICKING):
            style.set_camera_changed_callback(camera_changed_callback)


class RubberBandZoomInteractorStyle(vtkInteractorStyleRubberBandZoom):
    def __init__(self, plotter):
        super().__init__()
        self.plotter = plotter
        self._pyvista_plotter = _PlotterWrapper(plotter)  # HACK: Wrapper for PyVista compatibility
        self._picking_callback = None
        self._ignore_rubberband_interaction = False
        self.update_default_camera_state()
        self.AddObserver(vtkCommand.RightButtonPressEvent, lambda *_: self._reset_camera())
        self.RemoveObservers(vtkCommand.LeftButtonPressEvent)
        self.RemoveObservers(vtkCommand.MouseMoveEvent)
        self.RemoveObservers(vtkCommand.LeftButtonReleaseEvent)
        self.AddObserver(vtkCommand.LeftButtonPressEvent, self._on_left_button_press_event)
        self.AddObserver(vtkCommand.MouseMoveEvent, self._on_mouse_move_event)
        self.AddObserver(vtkCommand.LeftButtonReleaseEvent, self._on_left_button_release_event)

    def _modifier_key_pressed(self):
        interactor = self.GetInteractor()
        return bool(interactor and (interactor.GetShiftKey() or interactor.GetControlKey() or interactor.GetAltKey()))

    def _on_left_button_press_event(self, obj, event):
        self._ignore_rubberband_interaction = self._modifier_key_pressed()
        if self._ignore_rubberband_interaction:
            if self._picking_callback is not None:
                self._picking_callback(obj, event)
            return
        super().OnLeftButtonDown()

    def set_picking_callback(self, picking_callback: Callable):
        self._picking_callback = picking_callback

    def _on_mouse_move_event(self, obj, event):
        if self._ignore_rubberband_interaction:
            return
        super().OnMouseMove()

    def _on_left_button_release_event(self, obj, event):
        if self._ignore_rubberband_interaction:
            self._ignore_rubberband_interaction = False
            return
        super().OnLeftButtonUp()

    def update_default_camera_state(self):
        """Re-cache the current camera state as the default (right-click reset) state.

        Must be called after any operation that changes the intended full-view
        camera state (e.g. after a fill transform is applied on resize).
        """
        camera = self.plotter.renderer.camera
        self._default_position = np.array(camera.position).copy()
        self._default_focal_point = np.array(camera.focal_point).copy()
        self._default_parallel_scale = camera.parallel_scale

    def _reset_camera(self):
        renderer = self.plotter.renderer
        camera = renderer.camera
        camera.position = self._default_position.tolist()
        camera.focal_point = self._default_focal_point.tolist()
        camera.parallel_scale = self._default_parallel_scale
        renderer.reset_camera_clipping_range()


class CursorZoomInteractorStyle(vtkInteractorStyleUser):
    """Custom interactor style for cursor-centered zoom with parallel projection."""

    def __init__(self, plotter):
        super().__init__()

        self.plotter = plotter
        self._pyvista_plotter = _PlotterWrapper(plotter)  # HACK: Wrapper for PyVista compatibility
        self.zoom_factor = 1.1

        self.update_default_camera_state()

        # Cache the current world coordinates under the cursor
        self._cursor_world_pos = None
        self._zoom_in_progress = False
        self._camera_changed_callback = None

        # Setup plotter
        self.plotter.track_mouse_position()

        # Register observers on the style using VTK command constants
        self.AddObserver(vtkCommand.MouseMoveEvent, self._on_mouse_move)
        self.AddObserver(vtkCommand.MouseWheelForwardEvent, self._on_wheel_forward)
        self.AddObserver(vtkCommand.MouseWheelBackwardEvent, self._on_wheel_backward)
        self.AddObserver(vtkCommand.RightButtonPressEvent, lambda *_: self._reset_camera_and_notify())

    def set_camera_changed_callback(self, camera_changed_callback: Callable | None):
        """Register a zero-argument callable fired after this style has moved the camera.

        Anything drawn in fixed screen coordinates (e.g. an overlaid selection shape) covers a
        different part of the instrument once the view is zoomed, so it needs to be told.
        """
        self._camera_changed_callback = camera_changed_callback

    def _notify_camera_changed(self):
        if self._camera_changed_callback is None:
            return
        try:
            self._camera_changed_callback()
        except Exception as ex:
            logger.debug(f"Exception in camera_changed callback: {ex}")

    def set_picking_callback(self, picking_callback: Callable):
        self.RemoveObservers(vtkCommand.LeftButtonPressEvent)
        self.AddObserver(vtkCommand.LeftButtonPressEvent, picking_callback)

    def set_hover_callback(self, hover_callback: Callable):
        self.RemoveObservers(vtkCommand.MouseMoveEvent)
        self.AddObserver(vtkCommand.MouseMoveEvent, hover_callback)
        self.AddObserver(vtkCommand.MouseMoveEvent, self._on_mouse_move)

    def _on_mouse_move(self, obj, event):
        if self._zoom_in_progress:
            return
        x, y = self.plotter.mouse_position
        self._cursor_world_pos = _display_to_world(self.plotter.renderer, x, y)

    def _on_wheel_forward(self, obj, event):
        self._zoom(self.zoom_factor)

    def _on_wheel_backward(self, obj, event):
        self._zoom(1.0 / self.zoom_factor)

    def _parent(self):
        """Return a reference to the plotter for PyVista compatibility."""
        return self._pyvista_plotter

    def _zoom(self, factor):
        """Zoom keeping the world-space point under the cursor fixed."""
        renderer = self.plotter.renderer
        camera = renderer.camera

        # Calculate dynamic zoom factor based on current parallel_scale
        zoom_sensitivity = 7  # Arbitrary
        scale_ratio = camera.parallel_scale * zoom_sensitivity
        base_zoom_power = self.zoom_factor - 1.0
        dynamic_base = 1.0 + base_zoom_power * scale_ratio
        dynamic_base = max(1.001, dynamic_base)  # Clamp to minimum

        # Adjust factor based on the dynamic base
        if factor > 1:  # zoom in
            factor = dynamic_base
        else:  # zoom out
            factor = 1.0 / dynamic_base

        # HACK: PyVista has a bug where the mouse position is only accurate
        # if it's called from a mouse move event.
        # The cached mouse position is the accurate one, the position measured
        # inside this call is inacurate. Zooming in uses the inacurate one, hence
        # the reason for needing both mouse positions to predict shift of camera.

        # Use cached mouse position from last mouse move
        world_cursor = self._cursor_world_pos
        if world_cursor is None:
            return

        # Inacurate mouse position
        x, y = self.plotter.mouse_position
        distorted_before = _display_to_world(renderer, x, y)
        if distorted_before is None:
            return

        focal = np.array(camera.focal_point)
        cam_pos = np.array(camera.position)

        # Calculate shift to keep cursor at same screen position
        # In parallel projection: when parallel_scale changes by factor,
        # offset from focal point needs to change by 1/factor to maintain screen position
        offset_before = world_cursor - focal
        offset_after = (distorted_before - focal) / factor  # Use inacurate mouse position due to PyVista bug
        shift = offset_before - offset_after

        new_parallel_scale = camera.parallel_scale / factor

        # If parallel_scale exceeds the default (zoom out too far), reset to default camera state
        if new_parallel_scale > self._default_parallel_scale:
            self._reset_camera()
        else:
            camera.parallel_scale = new_parallel_scale
            camera.focal_point = (focal + shift).tolist()
            camera.position = (cam_pos + shift).tolist()

        renderer.reset_camera_clipping_range()
        self.plotter.render_window.Render()
        # Covers both branches above, so a zoom-out past the default notifies only once
        self._notify_camera_changed()

    def _reset_camera(self):
        renderer = self.plotter.renderer
        camera = renderer.camera
        camera.position = self._default_position.tolist()
        camera.focal_point = self._default_focal_point.tolist()
        camera.parallel_scale = self._default_parallel_scale
        renderer.reset_camera_clipping_range()
        self.plotter.render_window.Render()

    def _reset_camera_and_notify(self):
        self._reset_camera()
        self._notify_camera_changed()

    def update_default_camera_state(self):
        """Re-cache the current camera state as the default (right-click reset) state.

        Must be called after any operation that changes the intended full-view
        camera state (e.g. after a fill transform is applied on resize).
        """
        camera = self.plotter.renderer.camera
        self._default_position = np.array(camera.position).copy()
        self._default_focal_point = np.array(camera.focal_point).copy()
        self._default_parallel_scale = camera.parallel_scale


def _display_to_world(renderer, dx, dy):
    """Convert display (pixel) coords to world coords on the z=0 plane."""
    # VTK's viewport picking: map display → view → world
    # Set z to zero since points assumed to be in xy plane
    renderer.SetDisplayPoint(dx, dy, 0.0)
    renderer.DisplayToWorld()
    wx, wy, wz, ww = renderer.GetWorldPoint()
    if abs(ww) < 1e-10:
        return None
    return np.array([wx / ww, wy / ww, 0])  # project onto z=0


class SwappedButtonTrackballCamera(vtkInteractorStyleTrackballCamera):
    def __init__(self):
        super().__init__()
        self.AddObserver(vtkCommand.LeftButtonPressEvent, lambda *_: self.OnRightButtonDown())
        self.AddObserver(vtkCommand.RightButtonPressEvent, lambda *_: self.OnLeftButtonDown())
        self.AddObserver(vtkCommand.LeftButtonReleaseEvent, lambda *_: self.OnRightButtonUp())
        self.AddObserver(vtkCommand.RightButtonReleaseEvent, lambda *_: self.OnLeftButtonUp())

    def set_picking_callback(self, picking_callback: Callable):
        self.RemoveObservers(vtkCommand.LeftButtonPressEvent)
        self.AddObserver(vtkCommand.LeftButtonPressEvent, picking_callback)
