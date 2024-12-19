# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections.abc import Iterable
import pyvista as pv
from .FullInstrumentViewModel import FullInstrumentViewModel
import numpy as np


class FullInstrumentViewPresenter:
    def __init__(self, view, workspace):
        self._view = view
        self._model = FullInstrumentViewModel(workspace, draw_detector_geometry=False)

        pv.global_theme.color_cycler = "default"

        # Plot orange sphere at the origin
        origin = pv.Sphere(radius=0.01, center=[0, 0, 0])
        self._view.add_simple_shape(origin, colour="orange", pickable=False)

        self._view.enable_point_picking(callback=self.point_picked)
        self._view.show_axes()
        self._view.set_camera_focal_point(self._model.sample_position())

        self._detector_mesh = self.createPolyDataMesh(self._model.detector_positions())
        self._detector_mesh["Integrated Counts"] = self._model.detector_counts()

        self._view.add_mesh(self._detector_mesh, scalars="Integrated Counts", clim=self._model.data_limits(), pickable=True)

        monitor_point_cloud = self.createPolyDataMesh(self._model.monitor_positions())
        monitor_point_cloud["colours"] = self.generateSingleColour(self._model.monitor_positions(), 1, 0, 0, 1)

        self._view.add_rgba_mesh(monitor_point_cloud, scalars="colours")

    def point_picked(self, point, picker):
        if point is None:
            return
        point_index = picker.GetPointId()
        detector_index = self._model.detector_index(point_index)
        self.show_plot_for_detectors([detector_index])
        self.show_info_text_for_detectors([detector_index])

    def rectangle_picked(self, rectangle):
        selected_points = rectangle.frustum_mesh.points
        points = set([self._detector_mesh.find_closest_point(p) for p in selected_points])
        self.show_plot_for_detectors(points)
        self.show_info_text_for_detectors(points)

    def createPolyDataMesh(self, points, faces=None) -> pv.PolyData:
        mesh = pv.PolyData(points, faces)
        return mesh

    def generateSingleColour(self, points, red: float, green: float, blue: float, alpha: float) -> np.ndarray:
        rgba = np.zeros((len(points), 4))
        rgba[:, 0] = red
        rgba[:, 1] = green
        rgba[:, 2] = blue
        rgba[:, 3] = alpha
        return rgba

    def show_plot_for_detectors(self, detector_indices: Iterable[int]) -> None:
        self._view.show_plot_for_detectors(
            self._model.workspace(), [self._model.workspace_index_from_detector_index(d) for d in detector_indices]
        )

    def show_info_text_for_detectors(self, detector_indices: Iterable[int]) -> None:
        detector_infos = [self._model.get_detector_info_text(d) for d in detector_indices]
        self._view.update_selected_detector_info(detector_infos)
