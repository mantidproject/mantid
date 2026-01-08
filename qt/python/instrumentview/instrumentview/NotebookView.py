# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import pyvista as pv


class NotebookView:
    def __init__(self) -> None:
        pv.global_theme.background = "black"
        pv.global_theme.font.color = "white"
        self._plotter = pv.Plotter(notebook=True)
        self._plotter.show(jupyter_backend="trame")

    def subscribe_presenter(self, presenter) -> None:
        self._presenter = presenter

    def add_detector_mesh(self, mesh: pv.PolyData, is_projection: bool, scalars=None) -> None:
        """Draw the given mesh in the main plotter window"""
        scalar_bar_args = dict(interactive=True, vertical=False, title_font_size=15, label_font_size=12) if scalars is not None else None
        self._plotter.add_mesh(
            mesh, pickable=False, scalars=scalars, render_points_as_spheres=True, point_size=15, scalar_bar_args=scalar_bar_args
        )

        if not is_projection:
            self._plotter.enable_trackball_style()
            return

        self._plotter.view_xy()
        self._plotter.enable_parallel_projection()
        self._plotter.enable_zoom_style()

    def reset_camera(self) -> None:
        self._plotter.reset_camera()

    def show_axes(self) -> None:
        self._plotter.show_axes()
