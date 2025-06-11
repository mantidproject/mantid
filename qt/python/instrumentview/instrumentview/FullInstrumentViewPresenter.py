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
    """Presenter for the Instrument View window"""

    _SPHERICAL_X = "Spherical X"
    _SPHERICAL_Y = "Spherical Y"
    _SPHERICAL_Z = "Spherical Z"
    _CYLINDRICAL_X = "Cylindrical X"
    _CYLINDRICAL_Y = "Cylindrical Y"
    _CYLINDRICAL_Z = "Cylindrical Z"
    _SIDE_BY_SIDE = "Side-By-Side"
    _PROJECTION_OPTIONS = [_SPHERICAL_X, _SPHERICAL_Y, _SPHERICAL_Z, _CYLINDRICAL_X, _CYLINDRICAL_Y, _CYLINDRICAL_Z, _SIDE_BY_SIDE]

    def __init__(self, view, workspace):
        """For the given workspace, use the data from the model to plot the detectors. Also include points at the origin and
        any monitors."""
        pv.global_theme.color_cycler = "default"
        pv.global_theme.allow_empty_mesh = True

        self._view = view
        self._model = FullInstrumentViewModel(workspace, draw_detector_geometry=False)

        # Plot orange sphere at the origin
        origin = pv.Sphere(radius=0.01, center=[0, 0, 0])
        self._view.add_simple_shape(origin, colour="orange", pickable=False)

        self._view.enable_point_picking(callback=self.point_picked)
        self._view.show_axes()
        self._view.set_camera_focal_point(self._model.sample_position())

        self._counts_label = "Integrated Counts"
        self._detector_mesh = self.createPolyDataMesh(self._model.detector_positions())
        self._detector_mesh[self._counts_label] = self._model.detector_counts()
        self._contour_limits = [self._model.data_limits()[0], self._model.data_limits()[1]]

        self._view.add_mesh(self._detector_mesh, scalars=self._counts_label, clim=self._contour_limits, pickable=False)
        self._view.set_contour_range_limits(self._contour_limits)

        self._picked_detector_mesh = self.createPolyDataMesh(self._model.detector_positions())
        self._picked_detector_mesh["visibility"] = np.zeros(self._picked_detector_mesh.number_of_points)
        self._view.add_picked_mesh(self._picked_detector_mesh, scalars="visibility", pickable=True)

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
        """Update the projection based on the selected option."""
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

        self._model.calculate_projection(is_spherical, axis)
        projection_mesh = self.createPolyDataMesh(self._model.detector_positions_projection())
        projection_mesh[self._counts_label] = self._model.detector_counts()
        self._view.add_projection_mesh(projection_mesh, self._counts_label, clim=self._contour_limits)

        self._pickable_projection_mesh = self.createPolyDataMesh(self._model.detector_positions_projection())
        self._pickable_projection_mesh["visibility"] = np.array(list(self._model.detector_visibility.values())).astype(int)
        self._view.add_pickable_projection_mesh(self._pickable_projection_mesh, scalars="visibility")

    def set_contour_limits(self, min: int, max: int) -> None:
        self._contour_limits = [min, max]
        self._view.update_scalar_range(self._contour_limits, self._counts_label)

    def set_tof_limits(self, min: int, max: int) -> None:
        self._model.update_time_of_flight_range(min, max)
        self._detector_mesh[self._counts_label] = self._model.detector_counts()
        self.set_contour_limits(self._model.data_limits()[0], self._model.data_limits()[1])

    def point_picked(self, point, picker):
        """For the given point, get the detector index and show all the information for that detector"""
        if point is None:
            return
        point_index = picker.GetPointId()
        detector_index = self._model.detector_index(point_index)
        self.update_picked_detectors([detector_index])

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
        selected_point_ids = np.argwhere(selected_mask).flatten()
        selected_detector_indices = [self._model.detector_index(id) for id in selected_point_ids]
        self.update_picked_detectors(selected_detector_indices)

    def update_picked_detectors(self, detector_indices: list[int]) -> None:
        self._model.negate_picked_visibility(detector_indices)
        # Update to visibility shows up in the plot in real time
        self._picked_detector_mesh["visibility"] = list(self._model.detector_visibility.values())
        self._pickable_projection_mesh["visibility"] = list(self._model.detector_visibility.values())
        visible_detector_indices = [key for key, val in self._model.detector_visibility.items() if val]
        self.show_plot_for_detectors(visible_detector_indices)
        self.show_info_text_for_detectors(visible_detector_indices)

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

    def show_plot_for_detectors(self, detector_indices: Iterable[int]) -> None:
        """Show line plot for specified detectors"""
        self._view.show_plot_for_detectors(
            self._model.workspace(), [self._model.workspace_index_from_detector_index(d) for d in detector_indices]
        )

    def show_info_text_for_detectors(self, detector_indices: Iterable[int]) -> None:
        """Show text information for specified detectors"""
        detector_infos = [self._model.get_detector_info_text(d) for d in detector_indices]
        self._view.update_selected_detector_info(detector_infos)
