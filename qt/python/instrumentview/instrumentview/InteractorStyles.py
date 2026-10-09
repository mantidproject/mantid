from typing import Callable
from vtkmodules.vtkInteractionStyle import vtkInteractorStyleTrackballCamera, vtkInteractorStyleRubberBandZoom
from vtkmodules.vtkCommonCore import vtkCommand
from vtkmodules.vtkRenderingCore import vtkActor2D, vtkPolyDataMapper2D
import numpy as np
import pyvista as pv

from mantid.kernel import logger


class InteractorStyles:
    def __init__(self, plotter, picking_callback, hover_callback, camera_changed_callback: Callable | None = None):
        self.SCROLL_ZOOM_WITH_PICKING = FlatProjectionInteractorStyle(plotter)
        self.SCROLL_ZOOM_WITH_HOVER = FlatProjectionInteractorStyle(plotter)
        self.SCROLL_ZOOM_NO_PICKING = FlatProjectionInteractorStyle(plotter)
        self.TRACKBALL = ThreeDInteractorStyle(plotter)
        self.RUBBERBAND_ZOOM = RubberBandZoomInteractorStyle(plotter)

        self.TRACKBALL.set_picking_callback(picking_callback)
        self.SCROLL_ZOOM_WITH_PICKING.set_picking_callback(picking_callback)
        self.RUBBERBAND_ZOOM.set_picking_callback(picking_callback)
        self.SCROLL_ZOOM_WITH_HOVER.set_hover_callback(hover_callback)

        for style in (self.SCROLL_ZOOM_WITH_PICKING, self.SCROLL_ZOOM_WITH_HOVER, self.SCROLL_ZOOM_NO_PICKING):
            style.set_camera_changed_callback(camera_changed_callback)

    def cleanup(self) -> None:
        """Release the callbacks and plotter held by every style.

        Each style registers Python callbacks with VTK, which holds them from C++ where the garbage
        collector cannot see them. Those callbacks refer back to the style, the presenter and the plotter,
        so unless they are removed the whole plotter is kept alive after the Instrument View is closed.
        """
        for style in (
            self.SCROLL_ZOOM_WITH_PICKING,
            self.SCROLL_ZOOM_WITH_HOVER,
            self.SCROLL_ZOOM_NO_PICKING,
            self.TRACKBALL,
            self.RUBBERBAND_ZOOM,
        ):
            style.remove_observers()


class RubberBandZoomInteractorStyle(vtkInteractorStyleRubberBandZoom):
    _RUBBER_BAND_COLOUR = (1.0, 1.0, 1.0)
    _RUBBER_BAND_LINE_WIDTH = 1.0
    _RUBBER_BAND_LINE_STIPPLE_PATTERN = 0xF0F0

    def __init__(self, plotter):
        super().__init__()
        self.plotter = plotter
        self._picking_callback = None
        self._ignore_rubberband_interaction = False
        self._rubber_band_start = None
        self._rubber_band_poly, self._rubber_band_actor = self._create_rubber_band_actor()
        self.plotter.renderer.AddActor2D(self._rubber_band_actor)
        self.update_default_camera_state()
        self.AddObserver(vtkCommand.RightButtonPressEvent, lambda *_: self._reset_camera())
        self.RemoveObservers(vtkCommand.LeftButtonPressEvent)
        self.RemoveObservers(vtkCommand.MouseMoveEvent)
        self.RemoveObservers(vtkCommand.LeftButtonReleaseEvent)
        self.AddObserver(vtkCommand.LeftButtonPressEvent, self._on_left_button_press_event)
        self.AddObserver(vtkCommand.MouseMoveEvent, self._on_mouse_move_event)
        self.AddObserver(vtkCommand.LeftButtonReleaseEvent, self._on_left_button_release_event)

    def _create_rubber_band_actor(self) -> tuple[pv.PolyData, vtkActor2D]:
        """Build the zoom-box outline as a scene overlay actor, positioned in display (pixel) coordinates.

        vtkInteractorStyleRubberBandZoom normally draws its own zoom box by poking pixels directly
        into the render window's framebuffer, bypassing the scene graph entirely. pyvistaqt >= 0.13
        always re-renders the whole scene from the actors before every paint, which wipes out that
        poke before it can ever be shown, so the box is drawn as a real actor instead so it survives
        the re-render.

        vtkPolyDataMapper2D/vtkActor2D have no pyvista-level equivalent (pyvista's own fixed-to-viewport
        overlays, e.g. Renderer.add_border, drop down to the same raw VTK classes), so only the mesh is
        built with pyvista.
        """
        poly_data = pv.PolyData()
        poly_data.points = np.zeros((5, 3))  # closed loop: 4 corners plus a repeat of the first to close it
        poly_data.lines = np.array([5, 0, 1, 2, 3, 4])

        mapper = vtkPolyDataMapper2D()
        mapper.SetInputData(poly_data)

        actor = vtkActor2D()
        actor.SetMapper(mapper)
        actor.GetProperty().SetColor(*self._RUBBER_BAND_COLOUR)
        actor.GetProperty().SetLineWidth(self._RUBBER_BAND_LINE_WIDTH)
        actor.GetProperty().SetLineStipplePattern(self._RUBBER_BAND_LINE_STIPPLE_PATTERN)
        actor.SetVisibility(False)
        return poly_data, actor

    def _set_rubber_band_points(self, start, end):
        x0, y0 = start
        x1, y1 = end
        self._rubber_band_poly.points = np.array([[x0, y0, 0.0], [x1, y0, 0.0], [x1, y1, 0.0], [x0, y1, 0.0], [x0, y0, 0.0]])

    def _event_position(self):
        interactor = self.GetInteractor()
        return interactor.GetEventPosition() if interactor is not None else (0, 0)

    def _modifier_key_pressed(self):
        interactor = self.GetInteractor()
        return bool(interactor and (interactor.GetShiftKey() or interactor.GetControlKey() or interactor.GetAltKey()))

    def _on_left_button_press_event(self, obj, event):
        self._ignore_rubberband_interaction = self._modifier_key_pressed()
        if self._ignore_rubberband_interaction:
            if self._picking_callback is not None:
                self._picking_callback(obj, event)
            return
        self._rubber_band_start = self._event_position()
        self._set_rubber_band_points(self._rubber_band_start, self._rubber_band_start)
        self._rubber_band_actor.SetVisibility(True)
        super().OnLeftButtonDown()

    def set_picking_callback(self, picking_callback: Callable):
        self._picking_callback = picking_callback

    def remove_observers(self):
        """Remove the callbacks VTK holds for this style. Also called by PyVista when its interactor closes."""
        self.RemoveAllObservers()
        self._picking_callback = None
        self.plotter = None

    def _on_mouse_move_event(self, obj, event):
        if self._ignore_rubberband_interaction:
            return
        if self._rubber_band_start is not None:
            self._set_rubber_band_points(self._rubber_band_start, self._event_position())
        super().OnMouseMove()

    def _on_left_button_release_event(self, obj, event):
        if self._ignore_rubberband_interaction:
            self._ignore_rubberband_interaction = False
            return
        super().OnLeftButtonUp()
        self._rubber_band_start = None
        self._rubber_band_actor.SetVisibility(False)
        self.plotter.render_window.Render()

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


class CursorZoomInteractorStyle(vtkInteractorStyleTrackballCamera):
    """Base interactor style that zooms with the mouse wheel about the point under the cursor.

    Every projection uses a parallel projection camera, so the same zoom works whichever way the camera
    faces. Zooming out stops at the full view. None of the trackball's own mouse button actions are used,
    so subclasses choose what the buttons do.
    """

    # How much one wheel notch zooms in from the full view
    _FULL_VIEW_ZOOM_STEP = 2.0

    def __init__(self, plotter):
        super().__init__()

        self.plotter = plotter
        self._renderer = plotter.renderer
        self._camera = self._renderer.GetActiveCamera()
        self._camera_changed_callback = None

        self.update_default_camera_state()

        # Overlaid shapes read the plotter's mouse position
        self.plotter.track_mouse_position()

        # An observer replaces the trackball's own handling of its event, so these switch off its
        # rotate, pan and drag-to-zoom actions. Subclasses add observers for the buttons they use.
        for event in (
            vtkCommand.LeftButtonPressEvent,
            vtkCommand.LeftButtonReleaseEvent,
            vtkCommand.MiddleButtonPressEvent,
            vtkCommand.MiddleButtonReleaseEvent,
            vtkCommand.RightButtonPressEvent,
            vtkCommand.RightButtonReleaseEvent,
        ):
            self.AddObserver(event, lambda *_: None)
        self.AddObserver(vtkCommand.MouseWheelForwardEvent, lambda *_: self._zoom(forward=True))
        self.AddObserver(vtkCommand.MouseWheelBackwardEvent, lambda *_: self._zoom(forward=False))

    def set_camera_changed_callback(self, camera_changed_callback: Callable | None):
        """Register a zero-argument callable fired after this style has moved the camera.

        Anything drawn in fixed screen coordinates (e.g. an overlaid selection shape) covers a
        different part of the instrument once the view is zoomed, so it needs to be told.
        """
        self._camera_changed_callback = camera_changed_callback

    def set_picking_callback(self, picking_callback: Callable):
        self.RemoveObservers(vtkCommand.LeftButtonPressEvent)
        self.AddObserver(vtkCommand.LeftButtonPressEvent, picking_callback)

    def remove_observers(self):
        """Remove the callbacks VTK holds for this style. Also called by PyVista when its interactor closes."""
        self.RemoveAllObservers()
        self._camera_changed_callback = None
        self.plotter = None
        self._renderer = None
        self._camera = None

    def _notify_camera_changed(self):
        if self._camera_changed_callback is None:
            return
        try:
            self._camera_changed_callback()
        except Exception as ex:
            logger.debug(f"Exception in camera_changed callback: {ex}")

    def _zoom(self, forward: bool):
        """Zoom keeping the point under the cursor fixed, stopping at the full view when zooming out."""
        interactor = self.GetInteractor()
        if interactor is None:
            return
        x, y = interactor.GetEventPosition()
        if not self._camera.GetParallelProjection():
            # The cursor zoom relies on a parallel projection, so fall back to the trackball's own zoom
            self.FindPokedRenderer(x, y)
            if forward:
                self.OnMouseWheelForward()
            else:
                self.OnMouseWheelBackward()
            return

        # Take big steps when zoomed out and small steps when zoomed in. The step is measured against the
        # full view rather than in world units, so every projection zooms at the same rate whatever its units.
        parallel_scale = self._camera.GetParallelScale()
        zoom_level = parallel_scale / self._default_parallel_scale if self._default_parallel_scale > 0 else 1.0
        factor = max(1.001, 1.0 + (self._FULL_VIEW_ZOOM_STEP - 1.0) * zoom_level)
        if not forward:
            factor = 1.0 / factor

        if parallel_scale / factor > self._default_parallel_scale:
            self._reset_camera()
        else:
            self._zoom_at_display_point(x, y, factor)
        self._camera_moved()

    def _zoom_at_display_point(self, dx, dy, factor):
        """Zoom by factor, keeping the world point under display (pixel) coords fixed.

        The camera can look along any direction, so the point is taken on the plane through
        the focal point facing the camera.
        """
        focal = np.array(self._camera.GetFocalPoint())
        position = np.array(self._camera.GetPosition())

        # Display depth of the focal point, so the cursor point lies in the same plane
        self._renderer.SetWorldPoint(*focal, 1.0)
        self._renderer.WorldToDisplay()
        focal_depth = self._renderer.GetDisplayPoint()[2]
        self._renderer.SetDisplayPoint(dx, dy, focal_depth)
        self._renderer.DisplayToWorld()
        wx, wy, wz, ww = self._renderer.GetWorldPoint()
        if abs(ww) < 1e-10:
            return
        cursor = np.array([wx, wy, wz]) / ww

        # Moving the focal point towards the cursor by (1 - 1/factor) of the way keeps the
        # cursor point's offset from the view centre, measured in units of parallel scale, unchanged
        shift = (cursor - focal) * (1.0 - 1.0 / factor)
        self._set_camera(focal + shift, position + shift, self._camera.GetParallelScale() / factor)

    def _reset_camera(self):
        """Return to the full view's zoom and centre, keeping the direction the view has been rotated to.

        The flat projections never rotate, so for them this restores the full view exactly.
        """
        direction = np.array(self._camera.GetDirectionOfProjection())
        distance = self._camera.GetDistance()
        self._set_camera(self._default_focal_point, self._default_focal_point - direction * distance, self._default_parallel_scale)

    def _reset_camera_and_notify(self):
        self._reset_camera()
        self._camera_moved()

    def _set_camera(self, focal_point, position, parallel_scale):
        self._camera.SetFocalPoint(*focal_point)
        self._camera.SetPosition(*position)
        self._camera.SetParallelScale(parallel_scale)

    def _camera_moved(self):
        """Redraw after the camera has moved, and tell anything drawn in screen coordinates."""
        self._renderer.ResetCameraClippingRange()
        interactor = self.GetInteractor()
        if interactor is not None:
            interactor.Render()
        self._notify_camera_changed()

    def update_default_camera_state(self):
        """Re-cache the current camera state as the full view.

        Must be called after any operation that changes the intended full-view
        camera state (e.g. after a fill transform is applied on resize).
        """
        self._default_focal_point = np.array(self._camera.GetFocalPoint())
        self._default_parallel_scale = self._camera.GetParallelScale()


class FlatProjectionInteractorStyle(CursorZoomInteractorStyle):
    """Interactor style for the flat (2D) projections: left-click picks a detector and right-click resets the view."""

    def __init__(self, plotter):
        super().__init__(plotter)
        self.AddObserver(vtkCommand.RightButtonPressEvent, lambda *_: self._reset_camera_and_notify())

    def set_hover_callback(self, hover_callback: Callable):
        self.AddObserver(vtkCommand.MouseMoveEvent, hover_callback)


class ThreeDInteractorStyle(CursorZoomInteractorStyle):
    """Interactor style for the 3D projection: left-click picks a detector and right-drag rotates the view."""

    def __init__(self, plotter):
        super().__init__(plotter)
        # The trackball rotates with its left button, so swap the buttons
        self.AddObserver(vtkCommand.LeftButtonPressEvent, lambda *_: self.OnRightButtonDown())
        self.AddObserver(vtkCommand.LeftButtonReleaseEvent, lambda *_: self.OnRightButtonUp())
        self.AddObserver(vtkCommand.RightButtonPressEvent, lambda *_: self.OnLeftButtonDown())
        self.AddObserver(vtkCommand.RightButtonReleaseEvent, lambda *_: self.OnLeftButtonUp())
        self.AddObserver(vtkCommand.MiddleButtonPressEvent, lambda *_: self.OnMiddleButtonDown())
        self.AddObserver(vtkCommand.MiddleButtonReleaseEvent, lambda *_: self.OnMiddleButtonUp())
