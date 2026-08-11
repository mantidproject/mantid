# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Callable, Optional

import numpy as np
import pyvista as pv
from pyvistaqt import BackgroundPlotter
from vtkmodules.vtkRenderingCore import vtkPointPicker

from instrumentview.renderers.base_renderer import InstrumentRenderer


class PointCloudRenderer(InstrumentRenderer):
    """Renders all detectors as a point cloud with spherical point sprites.

    This is the original (fast) rendering mode. Each detector is a single
    point; VTK renders it as a screen-space sphere of constant pixel size.
    """

    _DETECTOR_POINT_SIZE = 15
    _PICKABLE_POINT_SIZE = 30
    _MASKED_COLOUR = (0.25, 0.25, 0.25)
    _DEFAULT_PICKING_TOLERANCE = 0.01
    # Larger than _DETECTOR_POINT_SIZE so a ring of highlight colour shows around
    # the picked detector's own point.
    _PICKED_HALO_POINT_SIZE = 26

    def __init__(self) -> None:
        super().__init__()
        self._picking_tolerance = self._DEFAULT_PICKING_TOLERANCE

    # ------------------------------------------------------------------ build
    def build_detector_mesh(self, positions: np.ndarray, flip_beam: bool, model=None) -> pv.PolyData:
        return pv.PolyData(positions)

    def build_pickable_mesh(self, positions: np.ndarray, flip_beam: bool) -> pv.PolyData:
        return pv.PolyData(positions)

    def build_masked_mesh(self, positions: np.ndarray, flip_beam: bool, model=None) -> pv.PolyData:
        return pv.PolyData(positions)

    # ------------------------------------------------------------ add to plot
    def add_detector_mesh_to_plotter(
        self, plotter: BackgroundPlotter, mesh: pv.PolyData, scalars: Optional[str] = None, show_scalar_bar: bool = True
    ) -> None:
        scalar_bar_args = (
            dict(interactive=True, vertical=False, title_font_size=15, label_font_size=12)
            if scalars is not None and show_scalar_bar
            else None
        )
        plotter.add_mesh(
            mesh,
            pickable=False,
            scalars=scalars,
            render_points_as_spheres=True,
            point_size=self._DETECTOR_POINT_SIZE,
            scalar_bar_args=scalar_bar_args,
            show_scalar_bar=show_scalar_bar,
        )

        if plotter.off_screen:
            return

    def add_pickable_mesh_to_plotter(self, plotter: BackgroundPlotter, mesh: pv.PolyData, scalars) -> None:
        plotter.add_mesh(
            mesh,
            scalars=scalars,
            opacity=self._PICKED_FILL_OPACITY,
            clim=[0, 1],
            show_scalar_bar=False,
            pickable=True,
            cmap="Oranges",
            point_size=self._PICKABLE_POINT_SIZE,
            render_points_as_spheres=True,
        )

    def add_masked_mesh_to_plotter(self, plotter: BackgroundPlotter, mesh: pv.PolyData) -> None:
        if mesh.number_of_points == 0:
            return
        plotter.add_mesh(
            mesh,
            color=self._MASKED_COLOUR,
            pickable=False,
            render_points_as_spheres=True,
            point_size=self._DETECTOR_POINT_SIZE,
        )

    # --------------------------------------------------------------- picking
    def get_callback_tied_to_detector_index(
        self, plotter: BackgroundPlotter, callback: Callable[[int], None], hover: bool = False
    ) -> Callable:
        """Set up left-click point picking.  *callback* receives ``(detector_index: int)``."""

        if plotter.off_screen:
            return lambda _obj, _event: None

        picker = vtkPointPicker()
        picker.SetTolerance(self._effective_picking_tolerance(hover))

        def _on_pick(_obj, _event):
            x, y = plotter.iren.get_event_position()
            pick_result = picker.Pick(x, y, 0, plotter.renderer)
            if pick_result > 0:
                point_id = picker.GetPointId()
                if point_id >= 0:
                    callback(point_id)

        return _on_pick

    # ----------------------------------------------------- picked highlight
    def _add_picked_highlight_actor(self, plotter: BackgroundPlotter, mesh: pv.PolyData):
        """Add the halo actor that sits behind the picked detectors.

        Point size is in screen pixels, so the halo stays the same size however
        far the view is zoomed out.

        The halo deliberately does *not* use ``render_points_as_spheres``.  A
        sphere point sprite writes per-fragment depth, so the larger halo would
        win the depth test against the detector's own point and hide it
        completely.  A flat sprite is drawn at the detector's own depth instead,
        which lets the detector sphere — already drawn, and nearer the camera
        across its whole face — punch through the middle.  The counts colour
        therefore stays visible, and keeps tracking the contour limits, without
        the highlight having to redraw or recolour anything itself.
        """
        return plotter.add_points(
            mesh,
            color=self._PICKED_HIGHLIGHT_COLOUR,
            point_size=self._PICKED_HALO_POINT_SIZE,
            lighting=False,
            pickable=False,
            show_scalar_bar=False,
            render=False,
        )

    def _build_picked_highlight_mesh(self, mesh: pv.PolyData, visibility: np.ndarray) -> Optional[pv.PolyData]:
        picked = np.flatnonzero(visibility)
        if mesh.number_of_points == 0 or picked.size == 0 or int(picked.max()) >= mesh.number_of_points:
            return None
        return pv.PolyData(mesh.points[picked])

    # -------------------------------------------------------------- scalars
    def set_detector_scalars(self, mesh: pv.PolyData, counts: np.ndarray, label: str) -> None:
        mesh.point_data[label] = counts

    def set_pickable_scalars(self, mesh: pv.PolyData, visibility: np.ndarray, label: str) -> None:
        mesh.point_data[label] = visibility
