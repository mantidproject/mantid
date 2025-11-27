# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import pyvista as pv
from pyvista.plotting.picking import RectangleSelection
from pyvista.plotting.opts import PickerType
from mantid import mtd
from mantid.kernel import logger

from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewWindow
from instrumentview.InstrumentViewADSObserver import InstrumentViewADSObserver
from instrumentview.Peaks.WorkspaceDetectorPeaks import WorkspaceDetectorPeaks

from vtkmodules.vtkRenderingCore import vtkCoordinate


class FullInstrumentViewPresenter:
    """Presenter for the Instrument View window"""

    _TIME_OF_FLIGHT = "TOF"
    _D_SPACING = "dSpacing"
    _WAVELENGTH = "Wavelength"
    _MOMENTUM_TRANSFER = "MomentumTransfer"
    _UNIT_OPTIONS = [_TIME_OF_FLIGHT, _D_SPACING, _WAVELENGTH, _MOMENTUM_TRANSFER]

    _COLOURS = ["#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b", "#e377c2", "#7f7f7f", "#bcbd22", "#17becf"]

    def __init__(self, view: FullInstrumentViewWindow, model: FullInstrumentViewModel):
        """For the given workspace, use the data from the model to plot the detectors. Also include points at the origin and
        any monitors."""
        self._view = view
        self._model = model
        self._transform = np.eye(4)
        self._model.setup()
        self.setup()

    def setup(self):
        self._view.subscribe_presenter(self)
        default_index, options = self.projection_combo_options()
        self._view.set_projection_combo_options(default_index, options)
        self._view.setup_connections_to_presenter()
        self._view.set_contour_range_limits(self._model.counts_limits)
        self._view.set_integration_range_limits(self._model.integration_limits)

        if len(self._model.monitor_positions) > 0:
            monitor_point_cloud = self.create_poly_data_mesh(self._model.monitor_positions)
            monitor_point_cloud["colours"] = self.generate_single_colour(len(self._model.monitor_positions), 1, 0, 0, 1)
            self._view.add_rgba_mesh(monitor_point_cloud, scalars="colours")

        self._counts_label = "Integrated Counts"
        self._visible_label = "Visible Picked"
        self._view.show_axes()
        self._is_projection_selected = False
        self.on_projection_option_selected(default_index)

        if self._model.workspace_x_unit in self._UNIT_OPTIONS:
            self._view.set_unit_combo_box_index(self._UNIT_OPTIONS.index(self._model.workspace_x_unit))

        self._view.hide_status_box()
        self._ads_observer = InstrumentViewADSObserver(
            delete_callback=self.delete_workspace_callback,
            rename_callback=self.rename_workspace_callback,
            clear_callback=self.clear_workspace_callback,
            replace_callback=self.replace_workspace_callback,
            add_callback=self.add_workspace_callback,
        )
        self._view.hide_status_box()

    # TODO: Sort out view names and return names all in one place
    def projection_combo_options(self) -> tuple[int, list[str]]:
        default_projection = self._model.default_projection
        try:
            possible_returns = ["3D", "SPHERICAL_X", "SPHERICAL_Y", "SPHERICAL_Z", "CYLINDRICAL_X", "CYLINDRICAL_Y", "CYLINDRICAL_Z"]
            default_index = possible_returns.index(default_projection)
        except ValueError:
            default_index = 0
        return default_index, self._model._PROJECTION_OPTIONS

    def on_export_workspace_clicked(self) -> None:
        self._model.save_line_plot_workspace_to_ads()

    def on_sum_spectra_checkbox_clicked(self) -> None:
        self._update_line_plot_ws_and_draw(self._view.current_selected_unit())

    def available_unit_options(self) -> list[str]:
        if self._model.has_unit:
            return self._UNIT_OPTIONS
        return ["No units"]

    @property
    def workspace_display_unit(self) -> str:
        if self._model.has_unit:
            return self._model.workspace_x_unit_display
        return ""

    def on_integration_limits_updated(self) -> None:
        """When integration limits are changed, read the new limits and tell the presenter to update the colours accordingly"""
        self._model.integration_limits = self._view.get_integration_limits()
        self.set_view_integration_limits()

    def set_view_integration_limits(self) -> None:
        self._detector_mesh[self._counts_label] = self._model.detector_counts

    def on_contour_limits_updated(self) -> None:
        """When contour limits are changed, read the new limits and tell the presenter to update the colours accordingly"""
        self._model.counts_limits = self._view.get_contour_limits()
        self.set_view_contour_limits()

    def set_view_contour_limits(self) -> None:
        self._view.set_plotter_scalar_bar_range(self._model.counts_limits, self._counts_label)

    def on_projection_option_selected(self, selected_index: int) -> None:
        """Update the projection based on the selected option."""
        projection_type = self._model._PROJECTION_OPTIONS[selected_index]

        if projection_type.startswith("3D"):
            self._model.reset_cached_projection_positions()
            self._apply_projection_state(False, self._model.detector_positions)
            self._view.set_aspect_ratio_box_visibility(False)
            return

        self._view.set_aspect_ratio_box_visibility(True)
        projected_points = self._adjust_points_for_selected_projection(self._model.detector_positions, projection_type)
        self._apply_projection_state(True, projected_points)

    def _adjust_points_for_selected_projection(self, points: np.ndarray, projection_type: str) -> np.ndarray:
        if projection_type.startswith("3D"):
            self._model.reset_cached_projection_positions()
            return points

        if projection_type.endswith("X"):
            axis = [1, 0, 0]
        elif projection_type.endswith("Y"):
            axis = [0, 1, 0]
        elif projection_type.endswith("Z"):
            axis = [0, 0, 1]
        elif projection_type == self._model._SIDE_BY_SIDE:
            axis = [0, 0, 1]
        else:
            raise ValueError(f"Unknown projection type {projection_type}")

        return self._model.calculate_projection(projection_type, axis, points)

    def _apply_projection_state(self, is_projection: bool, positions: np.ndarray) -> None:
        self._is_projection_selected = is_projection
        self._update_view_main_plotter(positions, is_projection=self._is_projection_selected)
        self.on_multi_select_detectors_clicked()
        self.on_peaks_workspace_selected()

    def _update_view_main_plotter(self, positions: np.ndarray, is_projection: bool):
        self._detector_mesh = self.create_poly_data_mesh(positions)
        self._detector_mesh[self._counts_label] = self._model.detector_counts
        self._view.add_main_mesh(self._detector_mesh, is_projection=is_projection, scalars=self._counts_label)
        self._update_transform(is_projection, self._detector_mesh)
        self._detector_mesh.transform(self._transform, inplace=True)

        self._pickable_main_mesh = self.create_poly_data_mesh(positions)
        self._pickable_main_mesh[self._visible_label] = self._model.picked_visibility
        self._pickable_main_mesh.transform(self._transform, inplace=True)
        self._view.add_pickable_main_mesh(self._pickable_main_mesh, scalars=self._visible_label)
        self._view.enable_point_picking(self._is_projection_selected, callback=self.point_picked)
        self.set_view_contour_limits()
        self.set_view_integration_limits()

        self._view.reset_camera()

    def _update_transform(self, is_projection: bool, mesh: pv.PolyData) -> None:
        if not is_projection or self._view.is_maintain_aspect_ratio_checkbox_checked():
            self._transform = np.eye(4)
        else:
            self._transform = self._transform_mesh_to_fill_window(mesh)

    def _transform_mesh_to_fill_window(self, mesh: pv.PolyData) -> np.ndarray:
        x_min, x_max, y_min, y_max, z_min, z_max = mesh.bounds
        min_max_points = [
            [x_min, y_min, z_min],
            [x_max, y_max, z_max],
        ]

        # Convert to display coordinates (pixels)
        plotter = self._view.main_plotter
        coordinate = vtkCoordinate()
        coordinate.SetCoordinateSystemToWorld()
        display_coords = []
        for p in min_max_points:
            coordinate.SetValue(*p)
            display_coords.append(coordinate.GetComputedDisplayValue(plotter.renderer))

        window_width, window_height = plotter.window_size

        mesh_width = display_coords[1][0] - display_coords[0][0]
        mesh_height = display_coords[1][1] - display_coords[0][1]

        return self._scale_matrix_relative_to_centre(mesh.center, window_width / mesh_width, window_height / mesh_height)

    def _scale_matrix_relative_to_centre(self, centre, scale_x=1.0, scale_y=1.0) -> np.ndarray:
        # Translate to centre, scale, translate back
        # The matrix below is the product of those three transformations
        c_x, c_y, _ = centre
        return np.array([[scale_x, 0, 0, c_x * (1 - scale_x)], [0, scale_y, 0, c_y * (1 - scale_y)], [0, 0, 1, 0], [0, 0, 0, 1]])

    def _transform_vectors_with_matrix(self, points: np.ndarray, transform: np.ndarray) -> np.ndarray:
        # The transform is a 4x4 matrix, the points are 3D vectors, first we need an extra
        # entry on the points
        transformed_points = np.hstack([points, np.ones((points.shape[0], 1))])
        transformed_points = transformed_points @ transform.T
        # Now remove extra point
        return transformed_points[:, :3]

    def on_aspect_ratio_check_box_clicked(self) -> None:
        self._view.store_maintain_aspect_ratio_option()
        self.on_projection_option_selected(self._view._projection_combo_box.currentIndex())

    def on_multi_select_detectors_clicked(self) -> None:
        """Change between single and multi point picking"""
        if self._view.is_multi_picking_checkbox_checked():
            self._view.check_sum_spectra_checkbox()
            self._view.enable_rectangle_picking(self._is_projection_selected, callback=self.rectangle_picked)
        else:
            self._view.enable_point_picking(self._is_projection_selected, callback=self.point_picked)

    def point_picked(self, point_position: np.ndarray | None, picker: PickerType) -> None:
        if point_position is None:
            return
        point_index = picker.GetPointId()
        self.update_picked_detectors([point_index])

    def rectangle_picked(self, rectangle: RectangleSelection) -> None:
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
        self._pickable_main_mesh[self._visible_label] = self._model.picked_visibility

        self._update_line_plot_ws_and_draw(self._view.current_selected_unit())

    def _update_line_plot_ws_and_draw(self, unit: str) -> None:
        self._model.extract_spectra_for_line_plot(unit, self._view.sum_spectra_selected())
        self._view.show_plot_for_detectors(self._model.line_plot_workspace)
        self._view.set_selected_detector_info(self._model.picked_detectors_info_text())
        self._update_relative_detector_angle()
        self._update_peaks_workspaces()
        self.refresh_lineplot_peaks()

    def _update_relative_detector_angle(self) -> None:
        if len(self._model.picked_detector_ids) != 2:
            self._view.set_relative_detector_angle(None)
        else:
            self._view.set_relative_detector_angle(self._model.relative_detector_angle())

    def on_clear_selected_detectors_clicked(self) -> None:
        self.update_picked_detectors([])

    def create_poly_data_mesh(self, points: np.ndarray, faces=None) -> pv.PolyData:
        """Create a PyVista mesh from the given points and faces"""
        mesh = pv.PolyData(points, faces)
        return mesh

    def generate_single_colour(self, number_of_points: int, red: float, green: float, blue: float, alpha: float) -> np.ndarray:
        """Returns an RGBA colours array for the given set of points, with all points the same colour"""
        rgba = np.zeros((number_of_points, 4))
        rgba[:, 0] = red
        rgba[:, 1] = green
        rgba[:, 2] = blue
        rgba[:, 3] = alpha
        return rgba

    def _reload_peaks_workspaces(self):
        self._view.refresh_peaks_ws_list()
        self.on_peaks_workspace_selected()

    def delete_workspace_callback(self, ws_name):
        if self._model._workspace.name() == ws_name:
            self._view.close()
            logger.warning(f"Workspace {ws_name} deleted, closed Experimental Instrument View.")
        else:
            self._reload_peaks_workspaces()

    def rename_workspace_callback(self, ws_old_name, ws_new_name):
        if self._model._workspace.name() == ws_old_name:
            self._model._workspace = mtd[ws_new_name]
            logger.warning(f"Workspace {ws_old_name} renamed to {ws_new_name}, updated Experimental Instrument View.")
        self._reload_peaks_workspaces()

    def clear_workspace_callback(self):
        self._view.close()

    def replace_workspace_callback(self, ws_name, ws):
        if ws_name in self.peaks_workspaces_in_ads():
            self._reload_peaks_workspaces()
        elif ws_name == self._model.workspace.name():
            self._view.close()

    def add_workspace_callback(self, ws_name, ws):
        self._reload_peaks_workspaces()

    def handle_close(self):
        # The observers are unsubscribed on object deletion, it's safer to manually
        # delete the observer rather than wait for the garbage collector, because
        # we don't want stale workspace references hanging around.
        if hasattr(self, "_ads_observer"):
            del self._ads_observer

    def on_unit_option_selected(self, value) -> None:
        self._update_line_plot_ws_and_draw(self._UNIT_OPTIONS[value])

    def peaks_workspaces_in_ads(self) -> list[str]:
        return [ws.name() for ws in self._model.peaks_workspaces_in_ads()]

    def _update_peaks_workspaces(self) -> None:
        peaks_grouped_by_ws = []
        points_from_model = self._model.peak_overlay_points()
        for ws_index in range(len(points_from_model)):
            peaks_grouped_by_ws.append(WorkspaceDetectorPeaks(points_from_model[ws_index], self._COLOURS[ws_index % len(self._COLOURS)]))
        self._peaks_grouped_by_ws = peaks_grouped_by_ws

    def on_peaks_workspace_selected(self) -> None:
        self._model.set_peaks_workspaces(self._view.selected_peaks_workspaces())
        self._view.clear_overlay_meshes()
        self._update_peaks_workspaces()
        self.refresh_lineplot_peaks()
        self._view.refresh_peaks_ws_list_colours()
        if len(self._peaks_grouped_by_ws) == 0:
            return
        # Keeping the points from each workspace separate so we can colour them differently
        for ws_peaks in self._peaks_grouped_by_ws:
            peaks_detector_ids = np.array([p.detector_id for p in ws_peaks.detector_peaks])
            detector_ids = self._model.detector_ids
            # Use argsort + searchsorted for fast lookup. Using np.where(np.isin) does not
            # maintain the original order. It is faster to sort then search the sorted
            # array for matching detector IDs
            sorted_idx = np.argsort(detector_ids)
            sorted_detector_ids = detector_ids[sorted_idx]
            positions = np.searchsorted(sorted_detector_ids, peaks_detector_ids)
            # Map back to original indices
            ordered_indices = sorted_idx[positions]
            valid = sorted_detector_ids[positions] == peaks_detector_ids
            ordered_indices = ordered_indices[valid]
            labels = [p.label for i, p in enumerate(ws_peaks.detector_peaks) if valid[i]]
            projected_points = self._model.current_projected_positions[ordered_indices]
            # Plot the peaks and their labels on the projection
            if len(projected_points) > 0:
                transformed_points = self._transform_vectors_with_matrix(projected_points, self._transform)
                self._view.plot_overlay_mesh(transformed_points, labels, ws_peaks.colour)

    def refresh_lineplot_peaks(self) -> None:
        # Plot vertical lines on the lineplot if the peak detector is selected
        self._view.clear_lineplot_overlays()

        for ws_peaks in self._peaks_grouped_by_ws:
            x_values = []
            labels = []
            for peak in ws_peaks.detector_peaks:
                if peak.detector_id in self._model.picked_detector_ids:
                    match self._view.current_selected_unit():
                        case self._TIME_OF_FLIGHT:
                            x_values += [p.tof for p in peak.peaks]
                        case self._D_SPACING:
                            x_values += [p.dspacing for p in peak.peaks]
                        case self._WAVELENGTH:
                            x_values += [p.wavelength for p in peak.peaks]
                        case self._MOMENTUM_TRANSFER:
                            x_values += [p.q for p in peak.peaks]
                        case _:
                            raise RuntimeError("Unknown unit for drawing peak overlays")
                    labels += [p.label for p in peak.peaks]
            if len(x_values) > 0:
                self._view.plot_lineplot_overlay(x_values, labels, ws_peaks.colour)
        self._view.redraw_lineplot()
