# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
import numpy as np
import pyvista as pv


class FullInstrumentViewPresenter:
    """Presenter for the Instrument View window"""

    _SPHERICAL_X = "Spherical X"
    _SPHERICAL_Y = "Spherical Y"
    _SPHERICAL_Z = "Spherical Z"
    _CYLINDRICAL_X = "Cylindrical X"
    _CYLINDRICAL_Y = "Cylindrical Y"
    _CYLINDRICAL_Z = "Cylindrical Z"
    _PROJECTION_OPTIONS = [_SPHERICAL_X, _SPHERICAL_Y, _SPHERICAL_Z, _CYLINDRICAL_X, _CYLINDRICAL_Y, _CYLINDRICAL_Z]

    def __init__(self, view, workspace):
        """For the given workspace, use the data from the model to plot the detectors. Also include points at the origin and
        any monitors."""
        pv.global_theme.color_cycler = "default"
        pv.global_theme.allow_empty_mesh = True

        self._view = view
        self._model = FullInstrumentViewModel(workspace)

        # Plot orange sphere at the origin
        origin = pv.Sphere(radius=0.01, center=[0, 0, 0])
        self._view.add_simple_shape(origin, colour="orange", pickable=False)

        self._view.show_axes()
        self._view.set_camera_focal_point(self._model.sample_position())

        self._counts_label = "Integrated Counts"
        self._detector_mesh = self.createPolyDataMesh(self._model.detector_positions())
        self._detector_mesh[self._counts_label] = self._model.detector_counts()
        self._contour_limits = [self._model.data_limits()[0], self._model.data_limits()[1]]

        self._view.add_main_mesh(self._detector_mesh, scalars=self._counts_label, clim=self._contour_limits)
        self._view.set_contour_range_limits(self._contour_limits)

        self._pickable_main_mesh = self.createPolyDataMesh(self._model.detector_positions())
        self._pickable_main_mesh["visibility"] = self._model.picked_visibility()

        self._view.add_pickable_main_mesh(self._pickable_main_mesh, scalars="visibility")

        self._bin_limits = [self._model.bin_limits()[0], self._model.bin_limits()[1]]
        self._view.set_tof_range_limits(self._bin_limits)

        if len(self._model.monitor_positions()) > 0:
            monitor_point_cloud = self.createPolyDataMesh(self._model.monitor_positions())
            monitor_point_cloud["colours"] = self.generateSingleColour(self._model.monitor_positions(), 1, 0, 0, 1)
            self._view.add_rgba_mesh(monitor_point_cloud, scalars="colours")

        self.projection_option_selected(0)
        self._view.enable_point_picking(callback=self.point_picked)

    def projection_combo_options(self) -> list[str]:
        return self._PROJECTION_OPTIONS

    def projection_option_selected(self, selected_index: int) -> None:
        """Update the projection based on the selected option."""
        projection_type = self._PROJECTION_OPTIONS[selected_index]
        is_spherical = True
        if projection_type.startswith("Spherical"):
            is_spherical = True
        elif projection_type.startswith("Cylindrical"):
            is_spherical = False
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

        self._model.calculate_projection(is_spherical, axis)
        projection_mesh = self.createPolyDataMesh(self._model.detector_projection_positions())
        projection_mesh[self._counts_label] = self._model.detector_counts()
        self._view.add_projection_mesh(projection_mesh, self._counts_label, clim=self._contour_limits)

        self._pickable_projection_mesh = self.createPolyDataMesh(self._model.detector_projection_positions())
        self._pickable_projection_mesh["visibility"] = self._model.picked_visibility()
        self._view.add_pickable_projection_mesh(self._pickable_projection_mesh, scalars="visibility")

    def set_contour_limits(self, min: int, max: int) -> None:
        self._contour_limits = [min, max]
        self._view.update_scalar_range(self._contour_limits, self._counts_label)

    def set_tof_limits(self, min: int, max: int) -> None:
        self._model.update_time_of_flight_range(min, max)
        self._detector_mesh[self._counts_label] = self._model.detector_counts()
        self.set_contour_limits(self._model.data_limits()[0], self._model.data_limits()[1])

    def point_picked(self, point_position, picker):
        if point_position is None:
            return
        point_index = picker.GetPointId()
        self.update_picked_detectors([point_index])

    def set_multi_select_enabled(self, is_enabled: bool) -> None:
        """Change between single and multi point picking"""
        if is_enabled:
            self._view.enable_rectangle_picking(callback=self.rectangle_picked)
        else:
            self._view.enable_point_picking(callback=self.point_picked)

    def rectangle_picked(self, rectangle):
        """Get points within the selection rectangle and display information for those detectors"""
        selected_mesh = self._detector_mesh.select_enclosed_points(rectangle.frustum_mesh)
        selected_mask = selected_mesh.point_data["SelectedPoints"].view(bool)
        selected_point_indices = np.argwhere(selected_mask).flatten()
        self.update_picked_detectors(selected_point_indices)

    def update_picked_detectors(self, point_indices: list[int] | np.ndarray) -> None:
        if len(point_indices) == 0:
            self._model.clear_all_picked_detectors()
        else:
            self._model.negate_picked_visibility(point_indices)

        # Update to visibility shows up in real time
        self._pickable_main_mesh["visibility"] = self._model.picked_visibility()
        self._pickable_projection_mesh["visibility"] = self._model.picked_visibility()

        self._view.show_plot_for_detectors(self._model.workspace(), self._model.picked_workspace_indices())
        self._view.update_selected_detector_info(self._model.picked_detectors_info_text())

    def clear_all_picked_detectors(self) -> None:
        self.update_picked_detectors([])

    def createPolyDataMesh(self, points, faces=None) -> pv.PolyData:
        """Create a PyVista mesh from the given points and faces"""
        mesh = pv.PolyData(points, faces)
        return mesh

    def generateSingleColour(self, points, red: float, green: float, blue: float, alpha: float) -> np.ndarray:
        """Returns an RGBA colours array for the given set of points, with all points the same colour"""
        rgba = np.zeros((len(points), 4))
        rgba[:, 0] = red
        rgba[:, 1] = green
        rgba[:, 2] = blue
        rgba[:, 3] = alpha
        return rgba
