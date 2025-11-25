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
from qtpy.QtWidgets import QFileDialog
from vtk import vtkCylinder
from mantid import mtd
from mantid.kernel import logger, ConfigService
from mantidqt.io import open_a_file_dialog

from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewWindow
from instrumentview.InstrumentViewADSObserver import InstrumentViewADSObserver
from instrumentview.Peaks.WorkspaceDetectorPeaks import WorkspaceDetectorPeaks

from pathlib import Path


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
        self._model.setup()
        self.setup()

    def setup(self):
        self._view.subscribe_presenter(self)
        self._view.set_projection_combo_options(*self._model.get_default_projection_index_and_options())
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
        self.update_plotter()

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
        self._update_view_main_plotter()
        self.update_detector_picker()
        self.on_peaks_workspace_selected()

    def _update_view_main_plotter(self):
        self._detector_mesh = self.create_poly_data_mesh(self._model.detector_positions)
        self._detector_mesh[self._counts_label] = self._model.detector_counts
        self._view.add_detector_mesh(self._detector_mesh, is_projection=self._model.is_2d_projection(), scalars=self._counts_label)

        self._pickable_mesh = self.create_poly_data_mesh(self._model.detector_positions)
        self._pickable_mesh[self._visible_label] = self._model.picked_visibility
        self._view.add_pickable_mesh(self._pickable_mesh, scalars=self._visible_label)

        self._masked_mesh = self.create_poly_data_mesh(self._model.masked_positions)
        self._view.add_masked_mesh(self._masked_mesh)

        self._view.enable_or_disable_mask_widgets()
        self.set_view_contour_limits()
        self.set_view_integration_limits()

    def update_detector_picker(self) -> None:
        """Change between single and multi point picking"""
        if self._view.is_multi_picking_checkbox_checked():
            self._view.check_sum_spectra_checkbox()

            def rectangle_picked(rectangle: RectangleSelection) -> None:
                """Get points within the selection rectangle and display information for those detectors"""
                selected_mesh = self._detector_mesh.select_enclosed_points(rectangle.frustum_mesh)
                selected_mask = selected_mesh.point_data["SelectedPoints"].view(bool)
                self.update_picked_detectors(selected_mask)

            self._view.enable_rectangle_picking(self._model.is_2d_projection(), callback=rectangle_picked)
        else:

            def point_picked(point_position: np.ndarray | None, picker: PickerType.POINT.value) -> None:
                if point_position is None:
                    return
                point_index = picker.GetPointId()
                picked_mask = np.full(self._detector_mesh.GetNumberOfPoints(), False)
                picked_mask[point_index] = True
                self.update_picked_detectors(picked_mask)

            self._view.enable_point_picking(self._model.is_2d_projection(), callback=point_picked)

    def update_picked_detectors(self, picked_mask: np.ndarray) -> None:
        if np.sum(picked_mask) == 0:
            self._model.clear_all_picked_detectors()
        else:
            self._model.negate_picked_visibility(picked_mask)
        # Update to visibility shows up in real time
        self._pickable_mesh[self._visible_label] = self._model.picked_visibility
        self._update_line_plot_ws_and_draw(self._view.current_selected_unit())

    def on_add_cylinder_clicked(self) -> None:
        self._view.add_cylinder_widget(self._detector_mesh.GetBounds(), lambda *_: None)

    def on_cylinder_select_clicked(self) -> None:
        widget = self._view.get_current_widget()
        cylinder = vtkCylinder()
        widget.GetCylinderRepresentation().GetCylinder(cylinder)
        mask = [(cylinder.FunctionValue(self._detector_mesh.GetPoint(i)) < 0) for i in range(self._detector_mesh.GetNumberOfPoints())]
        new_key = self._model.add_new_detector_mask(mask)
        self._view.set_new_mask_key(new_key)

    def on_mask_item_selected(self) -> None:
        self._model.apply_detector_masks(self._view.selected_masks())
        self.update_plotter()
        self._update_line_plot_ws_and_draw(self._view.current_selected_unit())

    def on_save_mask_to_workspace_clicked(self) -> None:
        self._model.save_mask_workspace_to_ads()

    def on_overwrite_mask_clicked(self) -> None:
        self._model.overwrite_mask_to_current_workspace()
        self.on_clear_masks_clicked()

    def on_clear_masks_clicked(self) -> None:
        self._view.clear_mask_list()
        self._model.clear_stored_masks()
        self.on_mask_item_selected()

    def _update_line_plot_ws_and_draw(self, unit: str) -> None:
        self._model.extract_spectra_for_line_plot(unit, self._view.sum_spectra_selected())
        self._view.show_plot_for_detectors(self._model.line_plot_workspace)
        self._view.set_selected_detector_info(self._model.picked_detectors_info_text())
        self._update_peaks_workspaces()
        self.refresh_lineplot_peaks()

    def on_clear_selected_detectors_clicked(self) -> None:
        self.update_picked_detectors(np.array([]))

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
            projected_points = self._model.detector_positions[ordered_indices]
            # Plot the peaks and their labels on the projection
            if len(projected_points) > 0:
                self._view.plot_overlay_mesh(projected_points, labels, ws_peaks.colour)

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

    def on_save_xml_mask_clicked(self):
        filename = open_a_file_dialog(
            accept_mode=QFileDialog.AcceptSave,
            file_mode=QFileDialog.AnyFile,
            file_filter="XML files (*xml)",
            directory=ConfigService["defaultsave.directory"],
        )
        # TODO: Figure out if this can be done automatically by the dialog
        if Path(filename).suffix != ".xml":
            filename += ".xml"
        self._model.save_xml_mask(filename)
