# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Shape widgets for instrument view selection tools.

Chart2D-based 2D overlay shapes that support drag, resize and rotation.
All coordinates are normalised viewport coordinates in [0, 1].

Shape types:
- CircleSelectionShape: Circle (replaces CylinderWidgetNoRotation)
- RectangleSelectionShape: Rectangle with rotation handle (replaces RectangleWidgetNoRotation)
- EllipseSelectionShape: Ellipse with rotation handle (replaces EllipseWidgetNoRotation)
- AnnulusSelectionShape: Annulus (ring) with independent inner/outer radii
"""

from abc import ABC, abstractmethod

from mantid.kernel import logger

import numpy as np
import pyvista as pv

EDGE_TOL = 0.01
HANDLE_OFFSET = 0.03
HANDLE_RADIUS = 0.008

# VTK cursor constants
VTK_CURSOR_DEFAULT = 0
VTK_CURSOR_SIZEALL = 9
VTK_CURSOR_SIZENE = 10
VTK_CURSOR_HAND = 13

CURSOR_MAP = {
    "edge": VTK_CURSOR_SIZENE,
    "inner_edge": VTK_CURSOR_SIZENE,
    "inside": VTK_CURSOR_SIZEALL,
    "handle": VTK_CURSOR_HAND,
}


class SelectionShape(ABC):
    """Base class for 2D overlay selection shapes drawn via Chart2D.

    Coordinates are in normalised viewport space (0-1).
    """

    def __init__(self, cx: float, cy: float):
        self.cx = cx
        self.cy = cy
        self.angle = 0.0
        self._line_plot = None
        self._fill_plot = None

    @abstractmethod
    def outline_xy(self):
        """Return (x_array, y_array) for the outline polyline."""

    @abstractmethod
    def fill_coords(self):
        """Return (x, y_bottom, y_top) arrays for area fill."""

    @abstractmethod
    def hit_test(self, nx: float, ny: float) -> str | None:
        """Return 'edge', 'inside', 'handle', 'inner_edge' or None."""

    @abstractmethod
    def save_size(self) -> dict:
        """Snapshot current size parameters for delta-based resize."""

    @abstractmethod
    def apply_resize_delta(self, nx, ny, start_nx, start_ny, saved_size):
        """Resize based on cursor delta from start position."""

    @abstractmethod
    def indices_in_shape(self, proj: np.ndarray) -> np.ndarray:
        """Return boolean mask for Nx2 projected coords that fall inside."""

    # ── drawing ─────────────────────────────────────────────────────
    def create_plots(self, chart: pv.Chart2D):
        fx, fy_bot, fy_top = self.fill_coords()
        self._fill_plot = chart.area(fx, fy_bot, fy_top, color=(128, 128, 128, 80))
        self._fill_plot.brush.color = (128, 128, 128, 80)
        self._fill_plot.pen.style = ""
        ox, oy = self.outline_xy()
        self._line_plot = chart.line(ox, oy, color="red", width=3.0)

    def update_plots(self):
        ox, oy = self.outline_xy()
        self._line_plot.update(ox, oy)
        fx, fy_bot, fy_top = self.fill_coords()
        self._fill_plot.update(fx, fy_bot, fy_top)

    def cleanup(self):
        """No-op base cleanup; subclasses may override."""
        pass


class CircleSelectionShape(SelectionShape):
    """Circle selection shape in normalised viewport coords."""

    N_OUTLINE = 256
    N_FILL = 256

    def __init__(self, cx: float, cy: float, radius: float):
        super().__init__(cx, cy)
        self.radius = radius
        self._theta = np.linspace(0, 2 * np.pi, self.N_OUTLINE)
        self._fill_x = np.linspace(-1.0, 1.0, self.N_FILL)
        self._fill_upper = np.sqrt(np.clip(1.0 - self._fill_x**2, 0, None))

    def outline_xy(self):
        return (
            self.cx + self.radius * np.cos(self._theta),
            self.cy + self.radius * np.sin(self._theta),
        )

    def fill_coords(self):
        fx = self.cx + self.radius * self._fill_x
        fy_top = self.cy + self.radius * self._fill_upper
        fy_bot = self.cy - self.radius * self._fill_upper
        return fx, fy_bot, fy_top

    def hit_test(self, nx, ny):
        dist = np.hypot(nx - self.cx, ny - self.cy)
        if abs(dist - self.radius) < EDGE_TOL:
            return "edge"
        if dist < self.radius:
            return "inside"
        return None

    def save_size(self):
        return dict(radius=self.radius)

    def apply_resize_delta(self, nx, ny, start_nx, start_ny, saved_size):
        start_dist = np.hypot(start_nx - self.cx, start_ny - self.cy)
        curr_dist = np.hypot(nx - self.cx, ny - self.cy)
        self.radius = max(0.01, saved_size["radius"] + (curr_dist - start_dist))

    def indices_in_shape(self, proj):
        dist = np.hypot(proj[:, 0] - self.cx, proj[:, 1] - self.cy)
        return dist <= self.radius


class RectangleSelectionShape(SelectionShape):
    """Rectangle selection shape with rotation handle."""

    N_HANDLE = 32

    def __init__(self, cx, cy, half_w, half_h, angle=0.0):
        super().__init__(cx, cy)
        self.half_w = half_w
        self.half_h = half_h
        self.angle = angle
        self._handle_line_plot = None
        self._handle_circle_plot = None

    # ── coordinate helpers ──
    def _rot(self, lx, ly):
        c, s = np.cos(self.angle), np.sin(self.angle)
        return self.cx + lx * c - ly * s, self.cy + lx * s + ly * c

    def _inv_rot(self, gx, gy):
        dx, dy = gx - self.cx, gy - self.cy
        c, s = np.cos(self.angle), np.sin(self.angle)
        return dx * c + dy * s, -dx * s + dy * c

    def _corners(self):
        hw, hh = self.half_w, self.half_h
        local = [(-hw, -hh), (hw, -hh), (hw, hh), (-hw, hh)]
        return [self._rot(lx, ly) for lx, ly in local]

    # ── rotation handle ──
    def _handle_pos(self):
        return self._rot(0, self.half_h + HANDLE_OFFSET)

    def _handle_stem_xy(self):
        tx, ty = self._rot(0, self.half_h)
        hx, hy = self._handle_pos()
        return np.array([tx, hx]), np.array([ty, hy])

    def _handle_circle_xy(self):
        theta = np.linspace(0, 2 * np.pi, self.N_HANDLE)
        hx, hy = self._handle_pos()
        return hx + HANDLE_RADIUS * np.cos(theta), hy + HANDLE_RADIUS * np.sin(theta)

    # ── outline / fill ──
    def outline_xy(self):
        corners = self._corners()
        xs = [c[0] for c in corners] + [corners[0][0]]
        ys = [c[1] for c in corners] + [corners[0][1]]
        return np.array(xs), np.array(ys)

    def fill_coords(self):
        corners = self._corners()
        xs_c = np.array([c[0] for c in corners])
        x_min, x_max = xs_c.min(), xs_c.max()
        n = 30
        fx = np.linspace(x_min, x_max, n)
        fy_bot = np.full(n, np.nan)
        fy_top = np.full(n, np.nan)
        edges = [(corners[i], corners[(i + 1) % 4]) for i in range(4)]
        for i, x in enumerate(fx):
            y_vals = []
            for (x1, y1), (x2, y2) in edges:
                ddx = x2 - x1
                if abs(ddx) < 1e-12:
                    if abs(x - x1) < 1e-12:
                        y_vals.extend([y1, y2])
                else:
                    t = (x - x1) / ddx
                    if -1e-9 <= t <= 1 + 1e-9:
                        y_vals.append(y1 + t * (y2 - y1))
            if y_vals:
                fy_bot[i] = min(y_vals)
                fy_top[i] = max(y_vals)
        valid = ~np.isnan(fy_bot)
        return fx[valid], fy_bot[valid], fy_top[valid]

    # ── hit-testing ──
    def hit_test(self, nx, ny):
        hx, hy = self._handle_pos()
        if np.hypot(nx - hx, ny - hy) < HANDLE_RADIUS + EDGE_TOL:
            return "handle"
        lx, ly = self._inv_rot(nx, ny)
        hw, hh = self.half_w, self.half_h
        inside_x = -hw - EDGE_TOL <= lx <= hw + EDGE_TOL
        inside_y = -hh - EDGE_TOL <= ly <= hh + EDGE_TOL
        if (abs(lx + hw) < EDGE_TOL and inside_y) or (abs(lx - hw) < EDGE_TOL and inside_y):
            return "edge"
        if (abs(ly + hh) < EDGE_TOL and inside_x) or (abs(ly - hh) < EDGE_TOL and inside_x):
            return "edge"
        if -hw < lx < hw and -hh < ly < hh:
            return "inside"
        return None

    # ── resize ──
    def save_size(self):
        return dict(half_w=self.half_w, half_h=self.half_h)

    def apply_resize_delta(self, nx, ny, start_nx, start_ny, saved_size):
        lx, ly = self._inv_rot(nx, ny)
        slx, sly = self._inv_rot(start_nx, start_ny)
        dx = abs(lx) - abs(slx)
        dy = abs(ly) - abs(sly)
        self.half_w = max(0.01, saved_size["half_w"] + dx)
        self.half_h = max(0.01, saved_size["half_h"] + dy)

    def indices_in_shape(self, proj):
        dx = proj[:, 0] - self.cx
        dy = proj[:, 1] - self.cy
        c, s = np.cos(self.angle), np.sin(self.angle)
        lx = dx * c + dy * s
        ly = -dx * s + dy * c
        return (np.abs(lx) <= self.half_w) & (np.abs(ly) <= self.half_h)

    # ── drawing (with handle) ──
    def create_plots(self, chart):
        super().create_plots(chart)
        sx, sy = self._handle_stem_xy()
        self._handle_line_plot = chart.line(sx, sy, color="red", width=2.0)
        hx, hy = self._handle_circle_xy()
        self._handle_circle_plot = chart.line(hx, hy, color="red", width=2.0)

    def update_plots(self):
        super().update_plots()
        sx, sy = self._handle_stem_xy()
        self._handle_line_plot.update(sx, sy)
        hx, hy = self._handle_circle_xy()
        self._handle_circle_plot.update(hx, hy)


class EllipseSelectionShape(SelectionShape):
    """Rotatable ellipse selection shape with rotation handle."""

    N_OUTLINE = 256
    N_HANDLE = 32

    def __init__(self, cx, cy, half_a, half_b, angle=0.0):
        super().__init__(cx, cy)
        self.half_a = half_a
        self.half_b = half_b
        self.angle = angle
        self._theta = np.linspace(0, 2 * np.pi, self.N_OUTLINE)
        self._handle_line_plot = None
        self._handle_circle_plot = None

    # ── coordinate helpers ──
    def _rot(self, lx, ly):
        c, s = np.cos(self.angle), np.sin(self.angle)
        return self.cx + lx * c - ly * s, self.cy + lx * s + ly * c

    def _inv_rot(self, gx, gy):
        dx, dy = gx - self.cx, gy - self.cy
        c, s = np.cos(self.angle), np.sin(self.angle)
        return dx * c + dy * s, -dx * s + dy * c

    # ── rotation handle ──
    def _handle_pos(self):
        return self._rot(0, self.half_b + HANDLE_OFFSET)

    def _handle_stem_xy(self):
        tx, ty = self._rot(0, self.half_b)
        hx, hy = self._handle_pos()
        return np.array([tx, hx]), np.array([ty, hy])

    def _handle_circle_xy(self):
        theta = np.linspace(0, 2 * np.pi, self.N_HANDLE)
        hx, hy = self._handle_pos()
        return hx + HANDLE_RADIUS * np.cos(theta), hy + HANDLE_RADIUS * np.sin(theta)

    # ── outline / fill ──
    def outline_xy(self):
        c, s = np.cos(self.angle), np.sin(self.angle)
        lx = self.half_a * np.cos(self._theta)
        ly = self.half_b * np.sin(self._theta)
        return self.cx + lx * c - ly * s, self.cy + lx * s + ly * c

    def fill_coords(self):
        c, s = np.cos(self.angle), np.sin(self.angle)
        a2, b2 = self.half_a**2, self.half_b**2
        x_extent = np.sqrt(a2 * c**2 + b2 * s**2)
        n = 60
        fx = np.linspace(self.cx - x_extent, self.cx + x_extent, n)
        u = fx - self.cx
        A = s**2 / a2 + c**2 / b2
        B = 2 * u * c * s * (1 / a2 - 1 / b2)
        C = u**2 * (c**2 / a2 + s**2 / b2) - 1
        disc = np.clip(B**2 - 4 * A * C, 0, None)
        sq = np.sqrt(disc)
        fy_top = self.cy + (-B + sq) / (2 * A)
        fy_bot = self.cy + (-B - sq) / (2 * A)
        valid = disc > 0
        return fx[valid], fy_bot[valid], fy_top[valid]

    def hit_test(self, nx, ny):
        hx, hy = self._handle_pos()
        if np.hypot(nx - hx, ny - hy) < HANDLE_RADIUS + EDGE_TOL:
            return "handle"
        lx, ly = self._inv_rot(nx, ny)
        r_pt = np.hypot(lx, ly)
        if r_pt < 1e-12:
            return "inside"
        phi = np.arctan2(ly, lx)
        r_bnd = 1.0 / np.sqrt((np.cos(phi) / self.half_a) ** 2 + (np.sin(phi) / self.half_b) ** 2)
        if abs(r_pt - r_bnd) < EDGE_TOL:
            return "edge"
        if r_pt < r_bnd:
            return "inside"
        return None

    def save_size(self):
        return dict(half_a=self.half_a, half_b=self.half_b)

    def apply_resize_delta(self, nx, ny, start_nx, start_ny, saved_size):
        lx, ly = self._inv_rot(nx, ny)
        slx, sly = self._inv_rot(start_nx, start_ny)
        da = abs(lx) - abs(slx)
        db = abs(ly) - abs(sly)
        self.half_a = max(0.01, saved_size["half_a"] + da)
        self.half_b = max(0.01, saved_size["half_b"] + db)

    def indices_in_shape(self, proj):
        dx = proj[:, 0] - self.cx
        dy = proj[:, 1] - self.cy
        c, s = np.cos(self.angle), np.sin(self.angle)
        lx = dx * c + dy * s
        ly = -dx * s + dy * c
        return (lx / self.half_a) ** 2 + (ly / self.half_b) ** 2 <= 1.0

    # ── drawing (with handle) ──
    def create_plots(self, chart):
        super().create_plots(chart)
        sx, sy = self._handle_stem_xy()
        self._handle_line_plot = chart.line(sx, sy, color="red", width=2.0)
        hx, hy = self._handle_circle_xy()
        self._handle_circle_plot = chart.line(hx, hy, color="red", width=2.0)

    def update_plots(self):
        super().update_plots()
        sx, sy = self._handle_stem_xy()
        self._handle_line_plot.update(sx, sy)
        hx, hy = self._handle_circle_xy()
        self._handle_circle_plot.update(hx, hy)


class AnnulusSelectionShape(SelectionShape):
    """Annulus (ring) selection shape — a circle with a concentric hole.

    The outer edge can be resized by dragging the outer boundary;
    the inner edge by dragging the inner boundary.  The shape can
    be dragged by clicking in the ring area between the two circles.
    """

    N_OUTLINE = 256
    N_FILL = 256

    def __init__(self, cx: float, cy: float, inner_radius: float, outer_radius: float):
        super().__init__(cx, cy)
        self.inner_radius = inner_radius
        self.outer_radius = outer_radius
        self._theta = np.linspace(0, 2 * np.pi, self.N_OUTLINE)
        self._fill_plot_bot = None  # second area plot for lower band

    # ── outline: outer circle + inner circle ──────────────────────
    def outline_xy(self):
        t = self._theta
        ox = np.concatenate(
            [
                self.cx + self.outer_radius * np.cos(t),
                [np.nan],
                self.cx + self.inner_radius * np.cos(t),
            ]
        )
        oy = np.concatenate(
            [
                self.cy + self.outer_radius * np.sin(t),
                [np.nan],
                self.cy + self.inner_radius * np.sin(t),
            ]
        )
        return ox, oy

    # ── fill: top band between inner and outer circles ───────────
    def fill_coords(self):
        n = self.N_FILL
        fx_o = np.linspace(-1.0, 1.0, n)
        outer_y = np.sqrt(np.clip(1.0 - fx_o**2, 0, None))
        ratio = self.inner_radius / max(self.outer_radius, 1e-12)
        inner_y = np.sqrt(np.clip(ratio**2 - fx_o**2, 0, None))
        fx = self.cx + self.outer_radius * fx_o
        has_inner = np.abs(fx_o) <= ratio
        fy_top = self.cy + self.outer_radius * outer_y
        fy_bot = np.where(
            has_inner,
            self.cy + self.outer_radius * inner_y,
            self.cy + self.outer_radius * (-outer_y),
        )
        return fx, fy_bot, fy_top

    def _bottom_fill_coords(self):
        """Lower band of the annulus ring for the second area plot."""
        n = self.N_FILL
        fx_o = np.linspace(-1.0, 1.0, n)
        outer_y = np.sqrt(np.clip(1.0 - fx_o**2, 0, None))
        ratio = self.inner_radius / max(self.outer_radius, 1e-12)
        inner_y = np.sqrt(np.clip(ratio**2 - fx_o**2, 0, None))
        has_inner = np.abs(fx_o) <= ratio
        fx = self.cx + self.outer_radius * fx_o
        fy_top = np.where(
            has_inner,
            self.cy - self.outer_radius * inner_y,
            self.cy + self.outer_radius * outer_y,
        )
        fy_bot = self.cy - self.outer_radius * outer_y
        return fx[has_inner], fy_bot[has_inner], fy_top[has_inner]

    # ── hit-testing ──────────────────────────────────────────────
    def hit_test(self, nx, ny):
        dist = np.hypot(nx - self.cx, ny - self.cy)
        if abs(dist - self.outer_radius) < EDGE_TOL:
            return "edge"
        if abs(dist - self.inner_radius) < EDGE_TOL:
            return "inner_edge"
        if self.inner_radius < dist < self.outer_radius:
            return "inside"
        return None

    # ── resize ───────────────────────────────────────────────────
    def save_size(self):
        return dict(inner_radius=self.inner_radius, outer_radius=self.outer_radius)

    def apply_resize_delta(self, nx, ny, start_nx, start_ny, saved_size):
        start_dist = np.hypot(start_nx - self.cx, start_ny - self.cy)
        curr_dist = np.hypot(nx - self.cx, ny - self.cy)
        delta = curr_dist - start_dist
        # ShapeOverlayManager stores '_resize_which' in its state dict;
        # however the shape itself doesn't see it.  We determine which
        # radius to change based on whether the start point was closer
        # to the inner or outer radius.
        inner_diff = abs(start_dist - saved_size["inner_radius"])
        outer_diff = abs(start_dist - saved_size["outer_radius"])
        if inner_diff < outer_diff:
            self.inner_radius = max(
                0.01,
                min(
                    saved_size["inner_radius"] + delta,
                    self.outer_radius - 0.01,
                ),
            )
        else:
            self.outer_radius = max(
                self.inner_radius + 0.01,
                saved_size["outer_radius"] + delta,
            )

    def indices_in_shape(self, proj):
        dist = np.hypot(proj[:, 0] - self.cx, proj[:, 1] - self.cy)
        return (dist >= self.inner_radius) & (dist <= self.outer_radius)

    # ── drawing (two fill bands) ─────────────────────────────────
    def create_plots(self, chart):
        # Top band fill
        fx, fy_bot, fy_top = self.fill_coords()
        self._fill_plot = chart.area(fx, fy_bot, fy_top, color=(128, 128, 128, 80))
        self._fill_plot.brush.color = (128, 128, 128, 80)
        self._fill_plot.pen.style = ""
        # Bottom band fill
        bfx, bfy_bot, bfy_top = self._bottom_fill_coords()
        self._fill_plot_bot = chart.area(bfx, bfy_bot, bfy_top, color=(128, 128, 128, 80))
        self._fill_plot_bot.brush.color = (128, 128, 128, 80)
        self._fill_plot_bot.pen.style = ""
        # Outlines
        ox, oy = self.outline_xy()
        self._line_plot = chart.line(ox, oy, color="red", width=3.0)

    def update_plots(self):
        ox, oy = self.outline_xy()
        self._line_plot.update(ox, oy)
        fx, fy_bot, fy_top = self.fill_coords()
        self._fill_plot.update(fx, fy_bot, fy_top)
        bfx, bfy_bot, bfy_top = self._bottom_fill_coords()
        self._fill_plot_bot.update(bfx, bfy_bot, bfy_top)


class ShapeOverlayManager:
    """Manages a Chart2D overlay and mouse interaction for a single selection shape.

    This replaces the VTK-widget-based approach with Chart2D overlay shapes.
    It is designed to be used by FullInstrumentViewWindow.
    """

    def __init__(self, plotter):
        self._plotter = plotter
        self._chart = None
        self._shape: SelectionShape | None = None
        self._state = dict(mode=None, active_shape=None)
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
        except Exception:
            pass
        try:
            self._chart.SetHiddenAxisBorder(0)
        except Exception:
            pass

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
        except Exception:
            pass
        if transform is None:
            for sub in getattr(self._chart, "_charts", []):
                try:
                    transform = sub.GetPlotTransform()
                    if transform is not None:
                        break
                except Exception:
                    continue

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
        except Exception:
            try:
                w, h = self._plotter.window_size
            except Exception:
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
            except Exception:
                pos = None
        else:
            pos = self._plotter.mouse_position
        if pos is None:
            return None
        return self._pixel_to_data(pos[0], pos[1])

    def _set_cursor(self, hit_type):
        try:
            rw = self._plotter.iren.interactor.GetRenderWindow()
            cursor_id = CURSOR_MAP.get(hit_type, VTK_CURSOR_DEFAULT)
            rw.SetCurrentCursor(cursor_id)
        except Exception:
            pass

    def set_shape(self, shape: SelectionShape):
        """Set (replace) the active selection shape and install observers."""
        self.remove_shape()
        self._ensure_chart()
        self._shape = shape
        shape.create_plots(self._chart)
        # create_plots adds line/area plots which create new sub-charts
        # inside PyVista's _MultiCompChart.  Clear their borders too.
        self._set_all_chart_borders_zero()
        self._install_observers()
        self._plotter.render()
        self._read_plot_transform()

    def remove_shape(self):
        """Remove the current shape and clean up."""
        if self._shape is not None:
            self._shape.cleanup()
            self._shape = None
        self._uninstall_observers()
        if self._chart is not None:
            try:
                self._plotter.remove_chart(self._chart)
            except Exception:
                pass
            self._chart = None
        self._cached_transform = None
        self._state = dict(mode=None, active_shape=None)

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
        except Exception:
            pass
        self._observer_ids = []

    def _detach_style(self):
        try:
            interactor = self._plotter.iren.interactor
            self._state["_saved_style"] = interactor.GetInteractorStyle()
            interactor.SetInteractorStyle(None)
        except Exception:
            pass

    def _restore_style(self):
        saved = self._state.pop("_saved_style", None)
        if saved is not None:
            try:
                self._plotter.iren.interactor.SetInteractorStyle(saved)
                saved.OnLeftButtonUp()
            except Exception:
                pass

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
            self._state["active_shape"] = self._shape
            if hit == "handle":
                self._state["mode"] = "rotate"
                self._state["rotate_start_cursor"] = np.arctan2(npos[1] - self._shape.cy, npos[0] - self._shape.cx)
                self._state["rotate_start_angle"] = self._shape.angle
            elif hit in ("edge", "inner_edge"):
                self._state["mode"] = "resize"
                self._state["resize_start"] = npos
                self._state["resize_saved"] = self._shape.save_size()
                self._state["_resize_which"] = "inner" if hit == "inner_edge" else "outer"
            elif hit == "inside":
                self._state["mode"] = "drag"
                self._state["drag_offset"] = (npos[0] - self._shape.cx, npos[1] - self._shape.cy)
            self._detach_style()
        except Exception:
            logger.debug("Exception in shape left-press handler.")

    def _on_left_release(self, obj, event):
        try:
            if self._state.get("mode") is not None:
                self._state["mode"] = None
                self._state.pop("_resize_which", None)
                self._state["active_shape"] = None
                self._restore_style()
        except Exception:
            logger.debug("Exception in shape left-release handler.")

    def _on_mouse_move(self, obj, event):
        try:
            s = self._shape
            if s is None:
                return
            mode = self._state.get("mode")
            if mode is not None:
                npos = self._screen_pos(obj)
                if npos is None:
                    return
                nx, ny = npos
                if mode == "drag":
                    ox, oy = self._state.get("drag_offset", (0, 0))
                    s.cx = nx - ox
                    s.cy = ny - oy
                elif mode == "resize":
                    s.apply_resize_delta(
                        nx, ny, self._state["resize_start"][0], self._state["resize_start"][1], self._state["resize_saved"]
                    )
                elif mode == "rotate":
                    cur = np.arctan2(ny - s.cy, nx - s.cx)
                    s.angle = self._state["rotate_start_angle"] + (cur - self._state["rotate_start_cursor"])
                s.update_plots()
                self._plotter.render()
                return

            # Idle: update cursor
            npos = self._screen_pos(obj)
            if npos is not None:
                hit = s.hit_test(*npos)
                self._set_cursor(hit)
        except Exception:
            logger.debug("Exception in shape mouse-move handler.")

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
        except Exception:
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
        try:
            self._cached_projected_points = self._project_world_to_data(points)
        except Exception:
            logger.debug("Failed to project and cache points.")

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
