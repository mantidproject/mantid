# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""ShapeOverlayManager - Chart2D overlay and mouse interaction manager.

Manages a single ``SelectionShape`` drawn on a PyVista ``Chart2D``
overlay, handling drag, resize and rotation via VTK interactor observers.
"""

from mantid.kernel import logger

import numpy as np
import pyvista as pv

from instrumentview.ShapeWidgets import (
    CURSOR_MAP,
    SelectionShape,
    VTK_CURSOR_DEFAULT,
)

# Keys for the _state interaction dictionary.
_STATE_MODE = "mode"
_STATE_ACTIVE_SHAPE = "active_shape"
_STATE_SAVED_STYLE = "_saved_style"
_STATE_ROTATE_START_CURSOR = "rotate_start_cursor"
_STATE_ROTATE_START_ANGLE = "rotate_start_angle"
_STATE_RESIZE_START = "resize_start"
_STATE_RESIZE_SAVED = "resize_saved"
_STATE_RESIZE_WHICH = "_resize_which"
_STATE_DRAG_OFFSET = "drag_offset"


class ShapeOverlayManager:
    """Manages a Chart2D overlay and mouse interaction for a single selection shape.

    This replaces the VTK-widget-based approach with Chart2D overlay shapes.
    It is designed to be used by FullInstrumentViewWindow.
    """

    def __init__(self, plotter):
        self._plotter = plotter
        self._chart = None
        self._shape: SelectionShape | None = None
        self._state = {_STATE_MODE: None, _STATE_ACTIVE_SHAPE: None}
        self._observer_ids = []
        # Cached PlotTransform parameters (sx, sy, tx, ty).
        # Populated by _read_plot_transform() after each successful VTK
        # read, used as fallback when GetPlotTransform() returns None.
        self._cached_transform = None  # tuple | None
        # Pre-projected detector coordinates (Nx2 normalised coords).
        # Populated on the main thread by project_and_cache_points(),
        # consumed once by get_shape_mask().
        self._cached_projected_points = None  # np.ndarray | None

    @property
    def current_shape(self) -> SelectionShape | None:
        return self._shape

    def _ensure_chart(self):
        if self._chart is not None:
            return
        self._chart = pv.Chart2D()
        self._chart.background_color = (0, 0, 0, 0)
        for ax in (self._chart.x_axis, self._chart.y_axis):
            ax.visible = False
            ax.grid = False
            ax.label_visible = False
            ax.ticks_visible = False
            ax.tick_labels_visible = False
            ax.margin = 0
        self._chart.x_range = [0, 1]
        self._chart.y_range = [0, 1]
        self._set_all_chart_borders_zero()
        # Prevent PyVista from detecting this chart under the mouse cursor.
        # Without this, PyVista's render_window_interactor swaps the active
        # interactor style to a Context style that forwards mouse events to
        # the vtkChartXY, causing a grey rubber-band selection overlay.
        self._chart._is_within = lambda pos: False
        self._plotter.add_chart(self._chart)

    def _set_all_chart_borders_zero(self):
        """Set borders to zero on the chart.

        ``vtkChartXY`` uses a default ``HiddenAxisBorder`` of 20 px
        which shifts the data-to-pixel mapping inward.  We zero it
        (and the explicit borders) so that data [0, 1] maps directly
        to the chart's full pixel extent.
        """
        if self._chart is None:
            return
        try:
            self._chart.SetBorders(0, 0, 0, 0)
        except Exception as ex:
            logger.debug(f"Failed to set chart borders to zero: {ex}")
        try:
            self._chart.SetHiddenAxisBorder(0)
        except Exception as ex:
            logger.debug(f"Failed to set chart hidden axis border to zero: {ex}")

    def _read_plot_transform(self):
        """Read the PlotTransform matrix from the chart (main thread only).

        Returns ``(sx, sy, tx, ty)`` or *None* if the transform is
        completely unavailable (no chart, never rendered).  On success
        the values are cached in ``self._cached_transform`` so that
        subsequent calls get the last-known-good values even when VTK
        temporarily returns *None* between render cycles.
        """
        if self._chart is None:
            return self._cached_transform

        transform = None
        try:
            transform = self._chart.GetPlotTransform()
        except Exception as ex:
            logger.debug(f"Failed to read PlotTransform from chart: {ex}")
        if transform is None:
            for sub in getattr(self._chart, "_charts", []):
                try:
                    transform = sub.GetPlotTransform()
                    if transform is not None:
                        break
                except Exception as ex:
                    logger.debug(f"Failed to read PlotTransform from sub-chart: {ex}")

        if transform is not None:
            m = transform.GetMatrix()
            sx = m.GetElement(0, 0)
            sy = m.GetElement(1, 1)
            tx = m.GetElement(0, 2)
            ty = m.GetElement(1, 2)
            self._cached_transform = (sx, sy, tx, ty)
            return self._cached_transform

        # GetPlotTransform() returned None — use last-known-good cache.
        return self._cached_transform

    def _pixel_to_data(self, px, py):
        """Convert pixel (event) coordinates to chart data coordinates [0-1].

        The chart's ``PlotTransform`` is the affine matrix VTK uses to map
        *data → pixel*.  Inverting it gives exact *pixel → data* conversion
        that matches how Chart2D renders shapes, regardless of any residual
        internal borders.

        Falls back to ``px / renderer_size`` normalisation when no
        PlotTransform is available (before the first render).  We must
        use ``renderer.GetSize()`` (physical VTK pixels) rather than
        ``plotter.window_size`` (Qt logical pixels) because
        ``GetEventPosition()`` returns physical pixels.  On Windows
        with DPI scaling the two can differ significantly.
        """
        params = self._read_plot_transform()
        if params is not None:
            sx, sy, tx, ty = params
            if sx != 0 and sy != 0:
                return (px - tx) / sx, (py - ty) / sy
        # Fallback before first paint: normalise by renderer size.
        # renderer.GetSize() returns physical VTK pixels, which is the
        # same coordinate system as GetEventPosition().
        try:
            w, h = self._plotter.renderer.GetSize()
        except Exception as ex:
            logger.debug(f"Failed to get renderer size for pixel-to-data conversion: {ex}")
            try:
                w, h = self._plotter.window_size
            except Exception as ex:
                logger.debug(f"Failed to get plotter window size for pixel-to-data conversion: {ex}")
                w, h = 1, 1
        if w == 0 or h == 0:
            return None
        return px / w, py / h

    def _screen_pos(self, vtk_interactor=None):
        """Return (nx, ny) in chart data coordinates [0-1].

        When called from a VTK observer callback, *vtk_interactor* should be
        the ``obj`` argument so that ``GetEventPosition()`` is read directly
        from the interactor that fired the event.  This is more reliable than
        ``plotter.mouse_position`` which can be ``None`` inside
        ``BackgroundPlotter`` observer callbacks.
        """
        if vtk_interactor is not None:
            try:
                pos = vtk_interactor.GetEventPosition()
            except Exception as ex:
                logger.debug(f"Failed to get event position from interactor: {ex}")
                pos = None
        else:
            pos = self._plotter.mouse_position
        if pos is None:
            return None
        return self._pixel_to_data(pos[0], pos[1])

    def _shape_pixel_aspect(self) -> float:
        """Return current data x:y aspect ratio for equal screen pixels."""
        params = self._read_plot_transform()
        if params is not None:
            sx, sy, _, _ = params
            if sy != 0:
                return abs(sx / sy)
        try:
            w, h = self._plotter.renderer.GetSize()
        except Exception as ex:
            logger.debug(f"Failed to get renderer size for pixel aspect ratio: {ex}")
            try:
                w, h = self._plotter.window_size
            except Exception as ex:
                logger.debug(f"Failed to get plotter window size for pixel aspect ratio: {ex}")
                w, h = 1, 1
        if h == 0:
            return 1.0
        return abs(w / h)

    def _set_cursor(self, hit_type):
        try:
            rw = self._plotter.iren.interactor.GetRenderWindow()
            cursor_id = CURSOR_MAP.get(hit_type, VTK_CURSOR_DEFAULT)
            rw.SetCurrentCursor(cursor_id)
        except Exception as ex:
            logger.debug(f"Failed to set cursor: {ex}")

    def set_shape(self, shape: SelectionShape):
        """Set (replace) the active selection shape and install observers."""
        self.remove_shape()
        self._ensure_chart()
        self._shape = shape
        self._read_plot_transform()
        self._shape.set_pixel_aspect(self._shape_pixel_aspect())
        shape.create_plots(self._chart)
        # create_plots adds line/area plots which create new sub-charts
        # inside PyVista's _MultiCompChart.  Clear their borders too.
        self._set_all_chart_borders_zero()
        self._detach_style()
        self._install_observers()
        self._plotter.render()
        self._read_plot_transform()

    def remove_shape(self):
        """Remove the current shape and clean up."""
        if self._shape is not None:
            self._shape.cleanup()
            self._shape = None
        self._uninstall_observers()
        self._restore_style()
        if self._chart is not None:
            try:
                self._plotter.remove_chart(self._chart)
            except Exception as ex:
                logger.debug(f"Failed to remove chart from plotter: {ex}")
            self._chart = None
        self._cached_projected_points = None
        self._cached_transform = None
        self._state = {_STATE_MODE: None, _STATE_ACTIVE_SHAPE: None}

    def _install_observers(self):
        if self._plotter.iren is None:
            return
        interactor = self._plotter.iren.interactor
        if interactor is None:
            return
        self._observer_ids = [
            interactor.AddObserver("LeftButtonPressEvent", self._on_left_press),
            interactor.AddObserver("LeftButtonReleaseEvent", self._on_left_release),
            interactor.AddObserver("MouseMoveEvent", self._on_mouse_move),
        ]

    def _uninstall_observers(self):
        if not self._observer_ids:
            return
        try:
            interactor = self._plotter.iren.interactor
            if interactor is not None:
                for oid in self._observer_ids:
                    interactor.RemoveObserver(oid)
        except Exception as ex:
            logger.debug(f"Failed to remove VTK observers: {ex}")
        self._observer_ids = []

    def _detach_style(self):
        try:
            self._state[_STATE_SAVED_STYLE] = self._plotter.iren.style
            self._plotter.iren.style = None
        except Exception as ex:
            logger.debug(f"Failed to detach interactor style: {ex}")

    def _restore_style(self):
        saved = self._state.pop(_STATE_SAVED_STYLE, None)
        if saved is not None:
            try:
                self._plotter.iren.style = saved
            except Exception as ex:
                logger.debug(f"Failed to restore interactor style: {ex}")

    def _on_left_press(self, obj, event):
        try:
            if self._shape is None:
                return
            npos = self._screen_pos(obj)
            if npos is None:
                return
            hit = self._shape.hit_test(*npos)
            if hit is None:
                return
            self._state[_STATE_ACTIVE_SHAPE] = self._shape
            match hit:
                case "handle":
                    self._state[_STATE_MODE] = "rotate"
                    self._state[_STATE_ROTATE_START_CURSOR] = np.arctan2(npos[1] - self._shape.cy, npos[0] - self._shape.cx)
                    self._state[_STATE_ROTATE_START_ANGLE] = self._shape.angle
                case "edge" | "inner_edge":
                    self._state[_STATE_MODE] = "resize"
                    self._state[_STATE_RESIZE_START] = npos
                    self._state[_STATE_RESIZE_SAVED] = self._shape.save_size()
                    self._state[_STATE_RESIZE_WHICH] = "inner" if hit == "inner_edge" else "outer"
                case "inside":
                    self._state[_STATE_MODE] = "drag"
                    self._state[_STATE_DRAG_OFFSET] = (npos[0] - self._shape.cx, npos[1] - self._shape.cy)
        except Exception as ex:
            logger.debug(f"Exception in shape left-press handler: {ex}")

    def _on_left_release(self, obj, event):
        try:
            if self._state.get(_STATE_MODE) is not None:
                self._state[_STATE_MODE] = None
                self._state.pop(_STATE_RESIZE_WHICH, None)
                self._state[_STATE_ACTIVE_SHAPE] = None
        except Exception as ex:
            logger.debug(f"Exception in shape left-release handler: {ex}")

    def _on_mouse_move(self, obj, event):
        try:
            s = self._shape
            if s is None:
                return
            mode = self._state.get(_STATE_MODE)
            if mode is not None:
                npos = self._screen_pos(obj)
                if npos is None:
                    return
                nx, ny = npos
                match mode:
                    case "drag":
                        ox, oy = self._state.get(_STATE_DRAG_OFFSET, (0, 0))
                        s.cx = nx - ox
                        s.cy = ny - oy
                    case "resize":
                        s.apply_resize_delta(
                            nx,
                            ny,
                            self._state[_STATE_RESIZE_START][0],
                            self._state[_STATE_RESIZE_START][1],
                            self._state[_STATE_RESIZE_SAVED],
                        )
                    case "rotate":
                        cur = np.arctan2(ny - s.cy, nx - s.cx)
                        s.angle = self._state[_STATE_ROTATE_START_ANGLE] + (cur - self._state[_STATE_ROTATE_START_CURSOR])
                s.update_plots()
                self._plotter.render()
                return

            # Idle: update cursor
            npos = self._screen_pos(obj)
            if npos is not None:
                hit = s.hit_test(*npos)
                self._set_cursor(hit)
        except Exception as ex:
            logger.debug(f"Exception in shape mouse-move handler. {ex}")

    def _project_world_to_data(self, points: np.ndarray) -> np.ndarray:
        """Project Nx3 world-space *points* to Nx2 chart data coords [0-1].

        Uses VTK's ``WorldToDisplay`` per point, then converts display
        pixels to chart data coordinates via the PlotTransform inverse
        (the same mapping ``_pixel_to_data`` uses for mouse events).

        Falls back to ``pixel / window_size`` normalisation when the
        PlotTransform is unavailable.

        .. warning:: VTK is **not** thread-safe — call from the main/Qt
           thread only.
        """
        renderer = self._plotter.renderer
        n = len(points)
        coords = np.empty((n, 2))

        # Localise attribute look-ups for the tight loop.
        set_wp = renderer.SetWorldPoint
        w2d = renderer.WorldToDisplay
        get_dp = renderer.GetDisplayPoint

        for i in range(n):
            set_wp(points[i, 0], points[i, 1], points[i, 2], 1.0)
            w2d()
            dp = get_dp()
            coords[i, 0] = dp[0]
            coords[i, 1] = dp[1]

        # Convert display pixels to chart data coords [0, 1] via
        # the PlotTransform inverse (same mapping _pixel_to_data uses).
        params = self._read_plot_transform()
        if params is not None:
            sx, sy, tx, ty = params
            if sx != 0 and sy != 0:
                coords[:, 0] = (coords[:, 0] - tx) / sx
                coords[:, 1] = (coords[:, 1] - ty) / sy
                return coords

        # Fallback: normalise by window size.
        try:
            w, h = renderer.GetSize()
        except Exception as ex:
            logger.debug(f"Failed to get renderer size for projection: {ex}")
            w, h = self._plotter.window_size
        coords[:, 0] /= max(w, 1)
        coords[:, 1] /= max(h, 1)
        return coords

    def project_and_cache_points(self, points: np.ndarray):
        """Project *points* (Nx3) and cache for the next ``get_shape_mask`` call.

        This must be called from the main/Qt thread (VTK is not thread-safe).
        The projected Nx2 array is kept in ``_cached_projected_points`` and
        consumed once by ``get_shape_mask``.
        """
        self._cached_projected_points = None
        try:
            self._cached_projected_points = self._project_world_to_data(points)
        except Exception as ex:
            logger.debug(f"Failed to project and cache points: {ex}")

    def project_points_to_screen(self, points: np.ndarray) -> np.ndarray:
        """Project Nx3 world-space points to Nx2 chart data coords [0-1].

        .. warning:: Uses VTK calls — prefer ``project_and_cache_points``
           (main thread) followed by ``get_shape_mask`` for thread-safe usage.
        """
        return self._project_world_to_data(points)

    def get_shape_mask(self, points: np.ndarray) -> np.ndarray:
        """Return boolean mask for which 3D points are inside the current shape.

        Prefers the pre-projected cache populated by
        ``project_and_cache_points`` (VTK main-thread projection, most
        accurate).  Falls back to ``project_points_to_screen`` if no
        cache is available.
        """
        if self._shape is None:
            return np.zeros(len(points), dtype=bool)

        cached = self._cached_projected_points
        if cached is not None and len(cached) == len(points):
            # Consume the cache (one-shot).
            self._cached_projected_points = None
            proj = cached
        else:
            proj = self.project_points_to_screen(points)

        return self._shape.indices_in_shape(proj)
