# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import pyvista as pv
from pyvista.plotting.opts import PickerType
from qtpy.QtWidgets import QFileDialog
from typing import Optional
from mantid import mtd
from mantid.kernel import logger, ConfigService
from mantid.simpleapi import AnalysisDataService
from mantidqt.io import open_a_file_dialog

from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewWindow
from instrumentview.InstrumentViewADSObserver import InstrumentViewADSObserver
from instrumentview.Peaks.WorkspaceDetectorPeaks import WorkspaceDetectorPeaks

from vtkmodules.vtkRenderingCore import vtkCoordinate

from enum import Enum


class SuppressRendering:
    def __init__(self, plotter):
        self.plotter = plotter
        self.old_value = plotter.suppress_rendering

    def __enter__(self):
        self.plotter.suppress_rendering = True
        return self.plotter

    def __exit__(self, exc_type, exc, tb):
        self.plotter.suppress_rendering = self.old_value


class PeakInteractionStatus(Enum):
    Disabled = 1
    Adding = 2
    Deleting = 3


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
        self._counts_label = "Integrated Counts"
        self._visible_label = "Visible Picked"
        self._model.setup()
        self.setup()

    def setup(self):
        self._view.subscribe_presenter(self)
        self._view.set_projection_combo_options(*self._model.get_default_projection_index_and_options())
        self._view.setup_connections_to_presenter()
        self._view.set_contour_range_limits(self._model.counts_limits)
        self._view.set_integration_range_limits(self._model.integration_limits)

        self._view.show_axes()
        self.update_plotter()

        if self._model.workspace_x_unit in self._UNIT_OPTIONS:
            self._view.set_unit_combo_box_index(self._UNIT_OPTIONS.index(self._model.workspace_x_unit))

        self._ads_observer = InstrumentViewADSObserver(
            delete_callback=self.delete_workspace_callback,
            rename_callback=self.rename_workspace_callback,
            clear_callback=self.clear_workspace_callback,
            replace_callback=self.replace_workspace_callback,
            add_callback=self.add_workspace_callback,
        )
        self._view.hide_status_box()
        self._peak_interaction_status = PeakInteractionStatus.Disabled
        self._update_peak_buttons()

    def _create_and_add_monitor_mesh(self) -> Optional[pv.PolyData]:
        if len(self._model.monitor_positions) == 0 or not self._view.is_show_monitors_checkbox_checked():
            return None
        monitor_point_cloud = self.create_poly_data_mesh(self._model.monitor_positions)
        monitor_point_cloud["colours"] = self.generate_single_colour(len(self._model.monitor_positions), 1, 0, 0, 1)
        self._view.add_rgba_mesh(monitor_point_cloud, scalars="colours")
        return monitor_point_cloud

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

    def update_plotter(self) -> None:
        """Update the projection based on the selected option."""
        self._model.projection_type = self._view.current_selected_projection()
        with SuppressRendering(self._view.main_plotter):
            self._update_view_main_plotter()
            self.update_detector_picker()
            self.on_peaks_workspace_selected()

    def _update_view_main_plotter(self):
        self._view.clear_main_plotter()

        self._detector_mesh = self.create_poly_data_mesh(self._model.detector_positions)
        self._detector_mesh[self._counts_label] = self._model.detector_counts
        self._view.add_detector_mesh(self._detector_mesh, is_projection=self._model.is_2d_projection, scalars=self._counts_label)

        self._pickable_mesh = self.create_poly_data_mesh(self._model.detector_positions)
        self._pickable_mesh[self._visible_label] = self._model.picked_visibility
        self._view.add_pickable_mesh(self._pickable_mesh, scalars=self._visible_label)

        self._masked_mesh = self.create_poly_data_mesh(self._model.masked_positions)
        self._view.add_masked_mesh(self._masked_mesh)

        monitor_mesh = self._create_and_add_monitor_mesh()

        # Update transform needs to happen after adding to plotter
        # Uses display coordinates
        self._update_transform()
        self._detector_mesh.transform(self._transform, inplace=True)
        self._pickable_mesh.transform(self._transform, inplace=True)
        self._masked_mesh.transform(self._transform, inplace=True)
        if monitor_mesh is not None:
            monitor_mesh.transform(self._transform, inplace=True)

        self._view.enable_or_disable_mask_widgets()
        self._view.enable_or_disable_aspect_ratio_box()
        self.set_view_contour_limits()
        self.set_view_integration_limits()

        self._view.cache_camera_position()
        self._view.reset_camera()

    def _update_transform(self) -> None:
        if not self._model.is_2d_projection or self._view.is_maintain_aspect_ratio_checkbox_checked():
            self._transform = np.eye(4)
        else:
            self._transform = self._transform_mesh_to_fill_window()

    def _transform_mesh_to_fill_window(self) -> np.ndarray:
        xmin, xmax, ymin, ymax, zmin, zmax = self._detector_mesh_bounds
        min_point = np.array([xmin, ymin, zmin])
        max_point = np.array([xmax, ymax, zmax])

        # Convert to display coordinates (pixels)
        plotter = self._view.main_plotter
        coordinate = vtkCoordinate()
        coordinate.SetCoordinateSystemToWorld()
        display_coords = []
        for p in (min_point, max_point):
            coordinate.SetValue(*p)
            display_coords.append(coordinate.GetComputedDisplayValue(plotter.renderer))

        mesh_width = display_coords[1][0] - display_coords[0][0]
        mesh_height = display_coords[1][1] - display_coords[0][1]

        window_width, window_height = plotter.window_size

        return self._scale_matrix_relative_to_centre((min_point + max_point) / 2, window_width / mesh_width, window_height / mesh_height)

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
        self.update_plotter()

    @property
    def _detector_mesh_bounds(self) -> list[float]:
        # Output format matches vtk's mesh.GetBounds()
        meshes_bounds = np.vstack([self._detector_mesh.bounds, self._masked_mesh.bounds])
        min_point = np.min(meshes_bounds[:, 0::2], axis=0)
        max_point = np.max(meshes_bounds[:, 1::2], axis=0)
        # Return list of xmin, xmax, ymin, ymax, zmin, zmax
        return [x for pair in zip(min_point, max_point) for x in pair]

    def update_detector_picker(self) -> None:
        """Change between single and multi point picking"""

        def point_picked(point_position: np.ndarray | None, picker: PickerType.POINT.value) -> None:
            if point_position is None:
                return
            point_index = picker.GetPointId()
            self._model.update_point_picked_detectors(point_index)
            # Update to visibility shows up in real time
            self._pickable_mesh[self._visible_label] = self._model.picked_visibility
            self._update_line_plot_ws_and_draw(self._view.current_selected_unit())
            self._peak_interaction_status = PeakInteractionStatus.Disabled
            self._view.remove_peak_cursor_from_lineplot()
            self._update_peak_buttons()

        self._view.enable_point_picking(self._model.is_2d_projection, callback=point_picked)

    def on_add_mask_clicked(self) -> None:
        implicit_function = self._view.get_current_widget_implicit_function()
        if not implicit_function:
            return
        mask = [(implicit_function.EvaluateFunction(pt) < 0) for pt in self._detector_mesh.points]
        new_key = self._model.add_new_detector_mask(mask)
        self._view.set_new_mask_key(new_key)

    def on_add_selection_clicked(self) -> None:
        implicit_function = self._view.get_current_widget_implicit_function()
        if not implicit_function:
            return
        mask = [(implicit_function.EvaluateFunction(pt) < 0) for pt in self._detector_mesh.points]
        new_key = self._model.add_new_detector_picking_selection(mask)
        self._view.set_new_picking_selection_key(new_key)

    def on_roi_item_selected(self) -> None:
        self._model.apply_detector_pick_selections(self._view.selected_pick_selections())
        # Update to visibility shows up in real time
        self._pickable_mesh[self._visible_label] = self._model.picked_visibility
        self._view.enable_or_disable_mask_widgets()
        self._update_line_plot_ws_and_draw(self._view.current_selected_unit())
        self._peak_interaction_status = PeakInteractionStatus.Disabled
        self._view.remove_peak_cursor_from_lineplot()
        self._update_peak_buttons()

    def on_mask_item_selected(self) -> None:
        self._model.apply_detector_masks(self._view.selected_masks())
        self.update_plotter()
        self._update_line_plot_ws_and_draw(self._view.current_selected_unit())
        self._update_peak_buttons()

    def on_save_roi_to_workspace_clicked(self) -> None:
        self._model.save_roi_workspace_to_ads()

    def on_save_mask_to_workspace_clicked(self) -> None:
        self._model.save_mask_workspace_to_ads()

    def on_overwrite_mask_clicked(self) -> None:
        self._model.overwrite_mask_to_current_workspace()
        self._view.clear_mask_list()

    def on_overwrite_roi_clicked(self) -> None:
        self._model.overwrite_roi_to_current_workspace()
        self._view.clear_roi_list()

    def on_clear_masks_clicked(self) -> None:
        self._view.clear_mask_list()
        self._model.clear_stored_masks()
        self.on_mask_item_selected()

    def on_clear_roi_clicked(self) -> None:
        self._view.clear_roi_list()
        self._model.clear_stored_rois()
        self.on_roi_item_selected()

    def on_save_xml_mask_clicked(self):
        filename = self._get_filename_from_dialog()
        if not filename:
            return
        self._model.save_xml_mask(filename)

    def on_save_xml_roi_clicked(self):
        filename = self._get_filename_from_dialog()
        if not filename:
            return
        self._model.save_xml_roi(filename)

    def _get_filename_from_dialog(self):
        filename = open_a_file_dialog(
            accept_mode=QFileDialog.AcceptSave,
            file_mode=QFileDialog.AnyFile,
            file_filter="XML files (*xml)",
            directory=ConfigService["defaultsave.directory"],
        )
        return filename

    def _reload_mask_workspaces(self) -> None:
        self._view.refresh_mask_ws_list()
        self.on_mask_item_selected()

    def mask_workspaces_in_ads(self) -> list[str]:
        return [ws.name() for ws in self._model.get_mask_workspaces_in_ads()]

    def cached_masks_keys(self) -> list[str]:
        return self._model.cached_masks_keys

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
            self._reload_mask_workspaces()

    def rename_workspace_callback(self, ws_old_name, ws_new_name):
        if self._model._workspace.name() == ws_old_name:
            self._model._workspace = mtd[ws_new_name]
            self._model.setup()
            logger.warning(f"Workspace {ws_old_name} renamed to {ws_new_name}, updated Experimental Instrument View.")
        self._reload_peaks_workspaces()
        self._reload_mask_workspaces()

    def clear_workspace_callback(self):
        self._view.close()

    def replace_workspace_callback(self, ws_name, ws):
        if ws_name in self.peaks_workspaces_in_ads():
            self._reload_peaks_workspaces()
        elif ws_name in self.mask_workspaces_in_ads():
            self._reload_mask_workspaces()
        elif ws_name == self._model.workspace.name():
            # This check is needed because observers are triggered
            # before the RenameWorkspace is completed.
            # Prevents strange behaviour from workspace not being fully replaced yet
            if AnalysisDataService.retrieve(ws_name).name() != ws_name:
                return
            self._model._workspace = AnalysisDataService.retrieve(ws_name)
            self._model.setup()
            self.update_plotter()

    def add_workspace_callback(self, ws_name, ws):
        self._reload_peaks_workspaces()
        self._reload_mask_workspaces()

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
        points_from_model = list(self._model.peak_overlay_points().values())
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
            peaks_spectrum_nos = np.array([p.spectrum_no for p in ws_peaks.detector_peaks])
            spectrum_nos = self._model.spectrum_nos
            # Use argsort + searchsorted for fast lookup. Using np.where(np.isin) does not
            # maintain the original order. It is faster to sort then search the sorted
            # array for matching spectrum numbers
            sorted_idx = np.argsort(spectrum_nos)
            sorted_spectrum_nos = spectrum_nos[sorted_idx]
            positions = np.searchsorted(sorted_spectrum_nos, peaks_spectrum_nos)
            # Map back to original indices
            ordered_indices = sorted_idx[positions]
            valid = sorted_spectrum_nos[positions] == peaks_spectrum_nos
            ordered_indices = ordered_indices[valid]
            labels = [p.label for i, p in enumerate(ws_peaks.detector_peaks) if valid[i]]
            projected_points = self._model.detector_positions[ordered_indices]
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
                if peak.spectrum_no in self._model.picked_spectrum_nos:
                    x_values += [p.location_in_unit(self._view.current_selected_unit()) for p in peak.peaks]
                    labels += [p.label for p in peak.peaks]
            if len(x_values) > 0:
                self._view.plot_lineplot_overlay(x_values, labels, ws_peaks.colour)
        self._view.redraw_lineplot()

    def on_add_peak_clicked(self) -> None:
        self._on_peak_clicked_in_lineplot(PeakInteractionStatus.Adding)

    def on_peak_selected(self, x: float) -> None:
        # First convert to workspace x unit
        x_in_workspace_unit = self._model.convert_units(self._view.current_selected_unit(), self._model.workspace_x_unit, 0, x)
        if self._peak_interaction_status == PeakInteractionStatus.Adding:
            peaks_ws = self._model.add_peak(x_in_workspace_unit, self._view.selected_peaks_workspaces())
            self._view.select_peaks_workspace(peaks_ws)
        elif self._peak_interaction_status == PeakInteractionStatus.Deleting:
            self._model.delete_peak(x_in_workspace_unit)
        else:
            raise RuntimeError("Unknown peak operation")
        self._peak_interaction_status = PeakInteractionStatus.Disabled
        self._view.remove_peak_cursor_from_lineplot()
        self._update_peak_buttons()

    def _update_peak_buttons(self) -> None:
        self._view.set_add_peak_button_enabled(
            len(self._model.picked_detector_ids) == 1 and self._peak_interaction_status != PeakInteractionStatus.Adding
        )
        self._view.set_delete_peak_button_enabled(
            self._view.has_any_peak_overlays() and self._peak_interaction_status != PeakInteractionStatus.Adding
        )
        self._view.set_delete_all_selected_peaks_button_enabled(
            len(self._model.picked_detector_ids) > 0 and self._peak_interaction_status != PeakInteractionStatus.Adding
        )

    def _on_peak_clicked_in_lineplot(self, status: PeakInteractionStatus) -> None:
        self._peak_interaction_status = status
        self._view.add_peak_cursor_to_lineplot()
        self._update_peak_buttons()

    def on_delete_peak_clicked(self) -> None:
        self._on_peak_clicked_in_lineplot(PeakInteractionStatus.Deleting)

    def on_delete_all_selected_peaks_clicked(self) -> None:
        self._model.delete_peaks_on_all_selected_detectors()
        self._update_peak_buttons()

    def on_show_monitors_check_box_clicked(self) -> None:
        self.update_plotter()
