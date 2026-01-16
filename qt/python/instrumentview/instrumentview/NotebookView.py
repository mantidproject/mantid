# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.dataobjects import Workspace2D

from matplotlib.axes import Axes
import matplotlib.pyplot as plt
import numpy as np
import pyvista as pv
from typing import Optional


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

    def add_selection_mesh(self, point_cloud: pv.PolyData, scalars: np.ndarray | str) -> None:
        self._plotter.add_mesh(
            point_cloud,
            scalars=scalars,
            opacity=[0.0, 0.3],
            show_scalar_bar=False,
            pickable=False,
            cmap="Oranges",
            point_size=30,
            render_points_as_spheres=True,
        )

    def reset_camera(self) -> None:
        self._plotter.reset_camera()

    def show_axes(self) -> None:
        self._plotter.show_axes()

    def pick_detectors(self, detector_ids: list[int] | np.ndarray, sum_spectra: bool = True) -> Optional[Axes]:
        return self._presenter.pick_detectors(detector_ids, sum_spectra)

    def _plot_spectra(self, workspace: Workspace2D, sum_spectra: bool) -> Axes:
        if workspace is not None and workspace.getNumberHistograms() > 0:
            spectra = workspace.getSpectrumNumbers()
            _, detector_spectrum_axes = plt.subplots(subplot_kw={"projection": "mantid"})
            for spec in spectra:
                detector_spectrum_axes.plot(workspace, specNum=spec, label=f"Spectrum {spec}" if not sum_spectra else None)
            if not sum_spectra:
                detector_spectrum_axes.legend(fontsize=8.0).set_draggable(True)
            return detector_spectrum_axes
