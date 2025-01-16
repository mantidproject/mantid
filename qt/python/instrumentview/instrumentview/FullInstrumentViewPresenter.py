# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from collections.abc import Iterable
import numpy as np
import pyvista as pv


class FullInstrumentViewPresenter:
    _SPHERICAL_X = "Spherical X"
    _SPHERICAL_Y = "Spherical Y"
    _SPHERICAL_Z = "Spherical Z"
    _CYLINDRICAL_X = "Cylindrical X"
    _CYLINDRICAL_Y = "Cylindrical Y"
    _CYLINDRICAL_Z = "Cylindrical Z"
    _SIDE_BY_SIDE = "Side-By-Side"
    _PROJECTION_OPTIONS = [_SPHERICAL_X, _SPHERICAL_Y, _SPHERICAL_Z, _CYLINDRICAL_X, _CYLINDRICAL_Y, _CYLINDRICAL_Z, _SIDE_BY_SIDE]

    def __init__(self, view, workspace):
        pv.global_theme.color_cycler = "default"
        pv.global_theme.allow_empty_mesh = True

        self._view = view
        self._model = FullInstrumentViewModel(workspace, draw_detector_geometry=False)

        # Plot orange sphere at the origin
        origin = pv.Sphere(radius=0.01, center=[0, 0, 0])
        self._view.add_simple_shape(origin, colour="orange", pickable=False)

        self._view.enable_point_picking(callback=self.point_picked)
        # self.plotter.enable_rectangle_picking(show_message=False, callback=self.rectangle_picked, use_picker=True)
        self._view.show_axes()
        self._view.set_camera_focal_point(self._model.sample_position())

        self._counts_label = "Integrated Counts"
        self._detector_mesh = self.createPolyDataMesh(self._model.detector_positions())
        self._detector_mesh[self._counts_label] = self._model.detector_counts()
        self._contour_limits = [self._model.data_limits()[0], self._model.data_limits()[1]]

        self._view.add_mesh(self._detector_mesh, scalars=self._counts_label, clim=self._contour_limits, pickable=True)
        self._view.set_contour_range_limits(self._contour_limits)

        self._bin_limits = [self._model.bin_limits()[0], self._model.bin_limits()[1]]
        self._view.set_tof_range_limits(self._bin_limits)

        if len(self._model.monitor_positions()) > 0:
            monitor_point_cloud = self.createPolyDataMesh(self._model.monitor_positions())
            monitor_point_cloud["colours"] = self.generateSingleColour(self._model.monitor_positions(), 1, 0, 0, 1)
            self._view.add_rgba_mesh(monitor_point_cloud, scalars="colours")

        self.projection_option_selected(0)

    def projection_combo_options(self) -> list[str]:
        return self._PROJECTION_OPTIONS

    def projection_option_selected(self, selected_index: int) -> None:
        projection_type = self._PROJECTION_OPTIONS[selected_index]
        if projection_type.startswith("Spherical"):
            is_spherical = True
        elif projection_type.startswith("Cylindrical"):
            is_spherical = False
        elif projection_type == self._SIDE_BY_SIDE:
            pass
        else:
            raise ValueError(f"Unknown projection type: {projection_type}")

        if projection_type.endswith("X"):
            axis = [1, 0, 0]
        elif projection_type.endswith("Y"):
            axis = [0, 1, 0]
        elif projection_type.endswith("Z"):
            axis = [0, 0, 1]
        else:
            raise ValueError(f"Unknown projection type {projection_type}")

        projection = self._model.calculate_projection(is_spherical, axis)
        projection_mesh = self.createPolyDataMesh(projection)
        projection_mesh[self._counts_label] = self._model.detector_counts()
        self._view.add_projection_mesh(projection_mesh, self._counts_label, clim=self._contour_limits)

    def set_contour_limits(self, min: int, max: int) -> None:
        self._contour_limits = [min, max]
        self._view.update_scalar_range(self._contour_limits, self._counts_label)

    def set_tof_limits(self, min: int, max: int) -> None:
        self._model.update_time_of_flight_range(min, max)
        self._detector_mesh[self._counts_label] = self._model.detector_counts()
        self.set_contour_limits(self._model.data_limits()[0], self._model.data_limits()[1])

    def point_picked(self, point, picker):
        if point is None:
            return
        point_index = picker.GetPointId()
        detector_index = self._model.detector_index(point_index)
        self.show_plot_for_detectors([detector_index])
        self.show_info_text_for_detectors([detector_index])

    # def rectangle_picked(self, rectangle):
    #     selected_points = rectangle.frustum_mesh.points
    #     points = set([self.m_detector_mesh.find_closest_point(p) for p in selected_points])
    #     self.show_plot_for_detectors(points)
    #     self.show_info_text_for_detectors(points)

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
