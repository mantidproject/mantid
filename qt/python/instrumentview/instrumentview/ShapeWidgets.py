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
- HollowRectangleSelectionShape: Rectangular frame with rotation handle
"""

from abc import ABC, abstractmethod


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
        self._pixel_aspect = 1.0

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

    def set_pixel_aspect(self, aspect: float) -> None:
        """Set x:y data-unit aspect ratio.

        The *aspect* is ``sx / sy`` from the chart PlotTransform where
        ``sx`` and ``sy`` are data->pixel scales on x/y.  Shapes that need
        round geometry in screen space (e.g. circles) use this.
        """
        aspect = aspect if aspect > 0 else 1.0
        self._pixel_aspect = aspect


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
            self.cy + (self.radius * self._pixel_aspect) * np.sin(self._theta),
        )

    def fill_coords(self):
        fx = self.cx + self.radius * self._fill_x
        ry = self.radius * self._pixel_aspect
        fy_top = self.cy + ry * self._fill_upper
        fy_bot = self.cy - ry * self._fill_upper
        return fx, fy_bot, fy_top

    def hit_test(self, nx, ny):
        dist = np.hypot(nx - self.cx, (ny - self.cy) / self._pixel_aspect)
        if abs(dist - self.radius) < EDGE_TOL:
            return "edge"
        if dist < self.radius:
            return "inside"
        return None

    def save_size(self):
        return dict(radius=self.radius)

    def apply_resize_delta(self, nx, ny, start_nx, start_ny, saved_size):
        start_dist = np.hypot(start_nx - self.cx, (start_ny - self.cy) / self._pixel_aspect)
        curr_dist = np.hypot(nx - self.cx, (ny - self.cy) / self._pixel_aspect)
        self.radius = max(0.01, saved_size["radius"] + (curr_dist - start_dist))

    def indices_in_shape(self, proj):
        dist = np.hypot(proj[:, 0] - self.cx, (proj[:, 1] - self.cy) / self._pixel_aspect)
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
        """Map local shape-space coordinates to chart data coordinates."""
        c, s = np.cos(self.angle), np.sin(self.angle)
        dx = lx * c - ly * s
        dy = lx * s + ly * c
        return self.cx + dx, self.cy + dy * self._pixel_aspect

    def _inv_rot(self, gx, gy):
        """Map chart data coordinates to local shape-space coordinates."""
        dx, dy = gx - self.cx, gy - self.cy
        dy /= self._pixel_aspect
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
        return self.cx + lx * c - ly * s, self.cy + (lx * s + ly * c) * self._pixel_aspect

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
        dy_top = (-B + sq) / (2 * A)
        dy_bot = (-B - sq) / (2 * A)
        fy_top = self.cy + self._pixel_aspect * dy_top
        fy_bot = self.cy + self._pixel_aspect * dy_bot
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
        dy = (proj[:, 1] - self.cy) / self._pixel_aspect
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
                self.cy + (self.outer_radius * self._pixel_aspect) * np.sin(t),
                [np.nan],
                self.cy + (self.inner_radius * self._pixel_aspect) * np.sin(t),
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
        ry_outer = self.outer_radius * self._pixel_aspect
        fy_top = self.cy + ry_outer * outer_y
        fy_bot = np.where(
            has_inner,
            self.cy + ry_outer * inner_y,
            self.cy + ry_outer * (-outer_y),
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
        ry_outer = self.outer_radius * self._pixel_aspect
        fy_top = np.where(
            has_inner,
            self.cy - ry_outer * inner_y,
            self.cy + ry_outer * outer_y,
        )
        fy_bot = self.cy - ry_outer * outer_y
        return fx[has_inner], fy_bot[has_inner], fy_top[has_inner]

    # ── hit-testing ──────────────────────────────────────────────
    def hit_test(self, nx, ny):
        dist = np.hypot(nx - self.cx, (ny - self.cy) / self._pixel_aspect)
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
        start_dist = np.hypot(start_nx - self.cx, (start_ny - self.cy) / self._pixel_aspect)
        curr_dist = np.hypot(nx - self.cx, (ny - self.cy) / self._pixel_aspect)
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
        dist = np.hypot(proj[:, 0] - self.cx, (proj[:, 1] - self.cy) / self._pixel_aspect)
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


class HollowRectangleSelectionShape(SelectionShape):
    """Rectangular frame — outer rectangle with a hollow inner rectangle.

    Both rectangles share the same centre and rotation angle.  The outer
    edge can be resized by dragging the outer boundary; the inner edge by
    dragging the inner boundary.  A rotation handle extends above the
    outer rectangle.
    """

    N_HANDLE = 32

    def __init__(self, cx, cy, outer_half_width, outer_half_height, inner_half_width, inner_half_height, angle=0.0):
        super().__init__(cx, cy)
        self.outer_half_width = outer_half_width
        self.outer_half_height = outer_half_height
        self.inner_half_width = inner_half_width
        self.inner_half_height = inner_half_height
        self.angle = angle
        self._handle_line_plot = None
        self._handle_circle_plot = None
        self._fill_plot_right = None
        self._fill_plot_top = None
        self._fill_plot_bot = None

    # ── coordinate helpers ───────────────────────────────────────
    def _rot(self, lx, ly):
        c, s = np.cos(self.angle), np.sin(self.angle)
        return self.cx + lx * c - ly * s, self.cy + lx * s + ly * c

    def _inv_rot(self, gx, gy):
        dx, dy = gx - self.cx, gy - self.cy
        c, s = np.cos(self.angle), np.sin(self.angle)
        return dx * c + dy * s, -dx * s + dy * c

    def _rect_corners(self, hw, hh):
        local = [(-hw, -hh), (hw, -hh), (hw, hh), (-hw, hh)]
        return [self._rot(lx, ly) for lx, ly in local]

    # ── rotation handle ──────────────────────────────────────────
    def _handle_pos(self):
        return self._rot(0, self.outer_half_height + HANDLE_OFFSET)

    def _handle_stem_xy(self):
        tx, ty = self._rot(0, self.outer_half_height)
        hx, hy = self._handle_pos()
        return np.array([tx, hx]), np.array([ty, hy])

    def _handle_circle_xy(self):
        theta = np.linspace(0, 2 * np.pi, self.N_HANDLE)
        hx, hy = self._handle_pos()
        return hx + HANDLE_RADIUS * np.cos(theta), hy + HANDLE_RADIUS * np.sin(theta)

    # ── outline ──────────────────────────────────────────────────
    def _rect_outline(self, hw, hh):
        corners = self._rect_corners(hw, hh)
        xs = [c[0] for c in corners] + [corners[0][0]]
        ys = [c[1] for c in corners] + [corners[0][1]]
        return np.array(xs), np.array(ys)

    def outline_xy(self):
        ox, oy = self._rect_outline(self.outer_half_width, self.outer_half_height)
        ix, iy = self._rect_outline(self.inner_half_width, self.inner_half_height)
        return (
            np.concatenate([ox, [np.nan], ix]),
            np.concatenate([oy, [np.nan], iy]),
        )

    # ── fill: 4 strips between outer and inner rects ─────────────
    @staticmethod
    def _scanline_fill(corners, n=30):
        """Compute area-fill arrays for an arbitrary rotated quad."""
        xs_c = np.array([c[0] for c in corners])
        x_min, x_max = xs_c.min(), xs_c.max()
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

    def _strip_corners(self, which):
        """Return 4 corners (global coords) for one of the 4 fill strips."""
        ohw, ohh = self.outer_half_width, self.outer_half_height
        ihw, ihh = self.inner_half_width, self.inner_half_height
        match which:
            case "left":
                local = [(-ohw, -ohh), (-ihw, -ohh), (-ihw, ohh), (-ohw, ohh)]
            case "right":
                local = [(ihw, -ohh), (ohw, -ohh), (ohw, ohh), (ihw, ohh)]
            case "top":
                local = [(-ihw, ihh), (ihw, ihh), (ihw, ohh), (-ihw, ohh)]
            case _:  # bottom
                local = [(-ihw, -ohh), (ihw, -ohh), (ihw, -ihh), (-ihw, -ihh)]
        return [self._rot(lx, ly) for lx, ly in local]

    def fill_coords(self):
        # Use the left strip as the "primary" fill for the base class
        return self._scanline_fill(self._strip_corners("left"))

    def _extra_fill_coords(self, which):
        return self._scanline_fill(self._strip_corners(which))

    # ── hit-testing ──────────────────────────────────────────────
    def hit_test(self, nx, ny):
        hx, hy = self._handle_pos()
        if np.hypot(nx - hx, ny - hy) < HANDLE_RADIUS + EDGE_TOL:
            return "handle"
        lx, ly = self._inv_rot(nx, ny)
        ohw, ohh = self.outer_half_width, self.outer_half_height
        ihw, ihh = self.inner_half_width, self.inner_half_height
        # Inner edge check
        in_inner_x = -ihw - EDGE_TOL <= lx <= ihw + EDGE_TOL
        in_inner_y = -ihh - EDGE_TOL <= ly <= ihh + EDGE_TOL
        if (abs(lx + ihw) < EDGE_TOL and in_inner_y) or (abs(lx - ihw) < EDGE_TOL and in_inner_y):
            return "inner_edge"
        if (abs(ly + ihh) < EDGE_TOL and in_inner_x) or (abs(ly - ihh) < EDGE_TOL and in_inner_x):
            return "inner_edge"
        # Outer edge check
        in_outer_x = -ohw - EDGE_TOL <= lx <= ohw + EDGE_TOL
        in_outer_y = -ohh - EDGE_TOL <= ly <= ohh + EDGE_TOL
        if (abs(lx + ohw) < EDGE_TOL and in_outer_y) or (abs(lx - ohw) < EDGE_TOL and in_outer_y):
            return "edge"
        if (abs(ly + ohh) < EDGE_TOL and in_outer_x) or (abs(ly - ohh) < EDGE_TOL and in_outer_x):
            return "edge"
        # Inside the frame (between outer and inner)?
        in_outer = -ohw < lx < ohw and -ohh < ly < ohh
        in_inner = -ihw < lx < ihw and -ihh < ly < ihh
        if in_outer and not in_inner:
            return "inside"
        return None

    # ── resize ───────────────────────────────────────────────────
    def save_size(self):
        return dict(
            outer_half_width=self.outer_half_width,
            outer_half_height=self.outer_half_height,
            inner_half_width=self.inner_half_width,
            inner_half_height=self.inner_half_height,
        )

    def apply_resize_delta(self, nx, ny, start_nx, start_ny, saved_size):
        lx, ly = self._inv_rot(nx, ny)
        slx, sly = self._inv_rot(start_nx, start_ny)
        dw = abs(lx) - abs(slx)
        dh = abs(ly) - abs(sly)
        # Determine whether to resize inner or outer based on which
        # edge the drag started closest to.
        outer_dist = min(
            abs(abs(slx) - saved_size["outer_half_width"]),
            abs(abs(sly) - saved_size["outer_half_height"]),
        )
        inner_dist = min(
            abs(abs(slx) - saved_size["inner_half_width"]),
            abs(abs(sly) - saved_size["inner_half_height"]),
        )
        if inner_dist < outer_dist:
            self.inner_half_width = max(0.01, min(saved_size["inner_half_width"] + dw, self.outer_half_width - 0.01))
            self.inner_half_height = max(0.01, min(saved_size["inner_half_height"] + dh, self.outer_half_height - 0.01))
        else:
            self.outer_half_width = max(self.inner_half_width + 0.01, saved_size["outer_half_width"] + dw)
            self.outer_half_height = max(self.inner_half_height + 0.01, saved_size["outer_half_height"] + dh)

    def indices_in_shape(self, proj):
        dx = proj[:, 0] - self.cx
        dy = proj[:, 1] - self.cy
        c, s = np.cos(self.angle), np.sin(self.angle)
        lx = dx * c + dy * s
        ly = -dx * s + dy * c
        in_outer = (np.abs(lx) <= self.outer_half_width) & (np.abs(ly) <= self.outer_half_height)
        in_inner = (np.abs(lx) <= self.inner_half_width) & (np.abs(ly) <= self.inner_half_height)
        return in_outer & ~in_inner

    # ── drawing (4 fill strips + handle) ─────────────────────────
    def create_plots(self, chart):
        # 4 fill strips
        fx, fy_bot, fy_top = self.fill_coords()
        self._fill_plot = chart.area(fx, fy_bot, fy_top, color=(128, 128, 128, 80))
        self._fill_plot.brush.color = (128, 128, 128, 80)
        self._fill_plot.pen.style = ""
        for which, attr in [
            ("right", "_fill_plot_right"),
            ("top", "_fill_plot_top"),
            ("bottom", "_fill_plot_bot"),
        ]:
            sfx, sfb, sft = self._extra_fill_coords(which)
            p = chart.area(sfx, sfb, sft, color=(128, 128, 128, 80))
            p.brush.color = (128, 128, 128, 80)
            p.pen.style = ""
            setattr(self, attr, p)
        # Outlines (outer + inner via NaN separator)
        ox, oy = self.outline_xy()
        self._line_plot = chart.line(ox, oy, color="red", width=3.0)
        # Handle
        sx, sy = self._handle_stem_xy()
        self._handle_line_plot = chart.line(sx, sy, color="red", width=2.0)
        hx, hy = self._handle_circle_xy()
        self._handle_circle_plot = chart.line(hx, hy, color="red", width=2.0)

    def update_plots(self):
        ox, oy = self.outline_xy()
        self._line_plot.update(ox, oy)
        # Fill strips
        fx, fy_bot, fy_top = self.fill_coords()
        self._fill_plot.update(fx, fy_bot, fy_top)
        for which, attr in [
            ("right", "_fill_plot_right"),
            ("top", "_fill_plot_top"),
            ("bottom", "_fill_plot_bot"),
        ]:
            sfx, sfb, sft = self._extra_fill_coords(which)
            getattr(self, attr).update(sfx, sfb, sft)
        # Handle
        sx, sy = self._handle_stem_xy()
        self._handle_line_plot.update(sx, sy)
        hx, hy = self._handle_circle_xy()
        self._handle_circle_plot.update(hx, hy)
