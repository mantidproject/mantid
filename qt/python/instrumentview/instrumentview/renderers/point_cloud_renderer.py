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
    _MASKED_COLOUR = (0.25, 0.25, 0.25)
    _DEFAULT_PICKING_TOLERANCE = 0.01
    # Larger than _DETECTOR_POINT_SIZE so the picked detector reads as a bigger,
    # differently coloured dot than its neighbours.  The halo's depth bulge grows
    # with its point size, and past this it starts showing through detectors
    # that should hide it.
    _PICKED_HALO_POINT_SIZE = 26
    # The hole punched through the middle of the halo is exactly a detector
    # point, so the picked detector — and with it the counts colour — shows
    # through while the magenta reads as a ring around it.
    _PICKED_HALO_HOLE_SIZE = _DETECTOR_POINT_SIZE

    def __init__(self) -> None:
        super().__init__()
        self._picking_tolerance = self._DEFAULT_PICKING_TOLERANCE

    # ------------------------------------------------------------------ build
    def build_detector_mesh(self, positions: np.ndarray, flip_beam: bool, model=None) -> pv.PolyData:
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
            pickable=True,
            scalars=scalars,
            render_points_as_spheres=True,
            point_size=self._DETECTOR_POINT_SIZE,
            scalar_bar_args=scalar_bar_args,
            show_scalar_bar=show_scalar_bar,
        )

        if plotter.off_screen:
            return

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
        """Add the halo actor that marks the picked detectors.

        Point size is in screen pixels, so the halo stays the same size however
        far the view is zoomed out.

        ``render_points_as_spheres`` is what makes the halo survive a crowd.  A
        detector point is also a screen-sized sprite, so on a packed instrument
        the neighbours tile the halo's whole area — a bigger halo cannot outrun
        them, because they close in on it at exactly the same rate.  The only
        way through is depth: a sphere sprite writes per-fragment depth, so the
        larger halo bulges further towards the camera than its neighbours' own
        sprites and wins the depth test against them.  A flat sprite sits at the
        detector's own depth and loses, leaving a few stray pixels of magenta
        peeking between neighbours.

        A filled halo would then hide the detector's counts colour, because a
        larger sprite always covers a smaller one drawn at the same centre — the
        depth bulge that beats the neighbours beats the detector's own point too.
        ``_punch_halo_hole`` carves the middle out so the colour still reads.
        """
        actor = plotter.add_points(
            mesh,
            color=self._PICKED_HIGHLIGHT_COLOUR,
            point_size=self._PICKED_HALO_POINT_SIZE,
            render_points_as_spheres=True,
            lighting=False,
            pickable=False,
            show_scalar_bar=False,
            render=False,
        )
        self._punch_halo_hole(actor)
        return actor

    @classmethod
    def _halo_ring_shader(cls) -> str:
        """GLSL that discards the middle of the halo's point sprite.

        ``gl_PointCoord`` runs 0..1 across the sprite, so this is the sprite's
        squared radius, compared against the hole's as a fraction of the halo's.
        Discarding rather than colouring matters: a discarded fragment writes no
        depth, which is what lets the detector's own point occupy the hole.
        """
        inner_fraction = (cls._PICKED_HALO_HOLE_SIZE / cls._PICKED_HALO_POINT_SIZE) ** 2
        return (
            "//VTK::Color::Impl\n"
            "  float haloX = 2.0 * gl_PointCoord.x - 1.0;\n"
            "  float haloY = 1.0 - 2.0 * gl_PointCoord.y;\n"
            f"  if (haloX * haloX + haloY * haloY < {inner_fraction:.4f}) {{ discard; }}\n"
        )

    def _punch_halo_hole(self, actor) -> None:
        """Turn the halo's filled disc into a ring.

        VTK has no annular point sprite, so the shape has to come from the
        fragment shader.  The replacement is keyed on a marker VTK substitutes
        into its own shader; if a future VTK drops that marker the substitution
        simply does not happen and the halo stays a filled disc, which is a
        cosmetic regression rather than a broken render.
        """
        actor.GetShaderProperty().AddFragmentShaderReplacement("//VTK::Color::Impl", True, self._halo_ring_shader(), False)

    def _build_picked_highlight_mesh(self, mesh: pv.PolyData, visibility: np.ndarray) -> Optional[pv.PolyData]:
        picked = np.flatnonzero(visibility)
        if mesh.number_of_points == 0 or picked.size == 0 or int(picked.max()) >= mesh.number_of_points:
            return None
        return pv.PolyData(mesh.points[picked])

    # -------------------------------------------------------------- scalars
    def set_detector_scalars(self, mesh: pv.PolyData, counts: np.ndarray, label: str) -> None:
        mesh.point_data[label] = counts
