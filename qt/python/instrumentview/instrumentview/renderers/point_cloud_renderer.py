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

from instrumentview.renderers.base_renderer import InstrumentRenderer


class PointCloudRenderer(InstrumentRenderer):
    """Renders all detectors as a point cloud with spherical point sprites.

    This is the original (fast) rendering mode. Each detector is a single
    point; VTK renders it as a screen-space sphere of constant pixel size.
    """

    _DETECTOR_POINT_SIZE = 15
    _PICKABLE_POINT_SIZE = 30
    _MASKED_COLOUR = (0.25, 0.25, 0.25)

    # ------------------------------------------------------------------ build
    def build_detector_mesh(self, positions: np.ndarray, model=None) -> pv.PolyData:
        return pv.PolyData(positions)

    def build_pickable_mesh(self, positions: np.ndarray) -> pv.PolyData:
        return pv.PolyData(positions)

    def build_masked_mesh(self, positions: np.ndarray, model=None) -> pv.PolyData:
        return pv.PolyData(positions)

    # ------------------------------------------------------------ add to plot
    def add_detector_mesh_to_plotter(
        self, plotter: BackgroundPlotter, mesh: pv.PolyData, is_projection: bool, scalars: Optional[str] = None
    ) -> None:
        scalar_bar_args = dict(interactive=True, vertical=False, title_font_size=15, label_font_size=12) if scalars is not None else None
        plotter.add_mesh(
            mesh,
            pickable=False,
            scalars=scalars,
            render_points_as_spheres=True,
            point_size=self._DETECTOR_POINT_SIZE,
            scalar_bar_args=scalar_bar_args,
        )

        if plotter.off_screen:
            return

        if not is_projection:
            plotter.enable_trackball_style()
            return

        plotter.view_xy()
        plotter.enable_parallel_projection()
        plotter.enable_zoom_style()

    def add_pickable_mesh_to_plotter(self, plotter: BackgroundPlotter, mesh: pv.PolyData, scalars) -> None:
        plotter.add_mesh(
            mesh,
            scalars=scalars,
            opacity=[0.0, 0.3],
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
    def enable_picking(self, plotter: BackgroundPlotter, callback: Callable[[int], None]) -> None:
        """Set up point picking.  *callback* receives ``(detector_index: int)``."""
        plotter.disable_picking()

        picking_tolerance = 0.01
        if not plotter.off_screen:

            def _point_picked(point_position, picker):
                if point_position is None:
                    return
                point_index = picker.GetPointId()
                callback(point_index)

            plotter.enable_surface_point_picking(
                show_message=False,
                use_picker=True,
                callback=_point_picked,
                show_point=False,
                pickable_window=False,
                picker="point",
                tolerance=picking_tolerance,
            )

    # -------------------------------------------------------------- scalars
    def set_detector_scalars(self, mesh: pv.PolyData, counts: np.ndarray, label: str) -> None:
        mesh.point_data[label] = counts

    def set_pickable_scalars(self, mesh: pv.PolyData, visibility: np.ndarray, label: str) -> None:
        mesh.point_data[label] = visibility
