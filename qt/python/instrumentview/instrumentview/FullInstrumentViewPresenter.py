# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL-3.0+
import numpy as np
import pyvista as pv
from queue import Queue
from typing import Literal, Optional, cast
from instrumentview.Globals import CurrentTab
from threading import Thread

from mantid import mtd
from mantid.kernel import logger
from mantid.simpleapi import AnalysisDataService
from mantid.dataobjects import MaskWorkspace, GroupingWorkspace, PeaksWorkspace

from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewView
from instrumentview.InstrumentViewADSObserver import InstrumentViewADSObserver
from instrumentview.ComponentTreeModel import ComponentTreeModel
from instrumentview.ComponentTreePresenter import ComponentTreePresenter
from instrumentview.Projections.ProjectionType import ProjectionType
from instrumentview.renderers.point_cloud_renderer import PointCloudRenderer
from instrumentview.renderers.shape_renderer import ShapeRenderer
from instrumentview.renderers.side_by_side_shape_renderer import SideBySideShapeRenderer

from instrumentview.InteractorStyles import InteractorStyles

from vtkmodules.vtkRenderingCore import vtkCoordinate


class SuppressRendering:
    def __init__(self, plotter):
        self.plotter = plotter
        self.old_value = plotter.suppress_rendering

    def __enter__(self):
        self.plotter.suppress_rendering = True
        return self.plotter

    def __exit__(self, exc_type, exc, tb):
        self.plotter.suppress_rendering = self.old_value


class FullInstrumentViewPresenter:
    """Presenter for the Instrument View window"""

    _TIME_OF_FLIGHT = "TOF"
    _D_SPACING = "dSpacing"
    _WAVELENGTH = "Wavelength"
    _MOMENTUM_TRANSFER = "MomentumTransfer"
    _UNIT_OPTIONS = [_TIME_OF_FLIGHT, _D_SPACING, _WAVELENGTH, _MOMENTUM_TRANSFER]

    _LINEAR = "Linear"
    _LOGARITHMIC = "Logarithmic"

    _XML_FILE_FILTER = "XML files (*xml)"
    _CAL_FILE_FILTER = "CAL files (*cal)"

    def __init__(self, view: FullInstrumentViewView, model: FullInstrumentViewModel):
        """For the given workspace, use the data from the model to plot the detectors. Also include points at the origin and
        any monitors."""
        self._view = view
        self._model = model
        self._closing = False
        self._transform = np.eye(4)
        self._counts_label = "Integrated Counts"
        self._visible_label = "Visible Picked"
        self._count_scale_mode = self._LINEAR
        self._detector_mesh: Optional[pv.PolyData] = None
        self._pickable_mesh: Optional[pv.PolyData] = None
        self._masked_mesh: Optional[pv.PolyData] = None
        self._model.setup()
        self._point_cloud_renderer = PointCloudRenderer()
        self._shape_renderer = ShapeRenderer(self._model.workspace)
        self._shape_renderer_full = ShapeRenderer(self._model.workspace, use_optimised_shapes=False)
        self._sbs_shape_renderer = SideBySideShapeRenderer(self._model.workspace)
        self._sbs_shape_renderer_full = SideBySideShapeRenderer(self._model.workspace, use_optimised_shapes=False)
        self._renderer = self._get_renderer_for_mode(view.get_render_mode_option())
        self._interactor_styles = InteractorStyles(self._view.main_plotter, picking_callback=lambda: None, hover_callback=lambda: None)
        self._last_hovered_point_index: Optional[int] = None
        self._select_bank_tube = False
        self._callback_queue = Queue()
        self._callback_stop_sentinel = object()
        self._callback_thread = Thread(None, self._callback_worker, daemon=True)
        self._callback_thread.start()
        self.monitor_colour = (230, 55, 55)
        self.sample_position_colour = (70, 160, 70)
        self.setup()

    def _callback_worker(self):
        while True:
            item = self._callback_queue.get()
            if item is self._callback_stop_sentinel:
                self._callback_queue.task_done()
                break
            func, args = item
            try:
                if not self._closing:
                    func(*args)
            except Exception as e:
                logger.error(f"Error in callback worker: {e}")
            finally:
                self._callback_queue.task_done()

    def setup(self):
        self._view.subscribe_presenter(self)
        self._view.set_projection_combo_options(self._model.get_projection_options())
        self._view.set_default_projection(self._model.get_default_projection())
        self._view.setup_connections_to_presenter()
        self._view.set_contour_range_limits(self._model.counts_limits)
        self._view.set_integration_range_limits(self._model.integration_limits)
        self._view.show_axes()
        self._setup_component_tree()

        self._ads_observer = InstrumentViewADSObserver(
            delete_callback=self.delete_workspace_callback,
            rename_callback=self.rename_workspace_callback,
            clear_callback=self.clear_workspace_callback,
            replace_callback=self.replace_workspace_callback,
            add_callback=self.add_workspace_callback,
        )
        self._view.hide_status_box()
        self._select_bank_tube = self._view.is_select_bank_tube_checked()
        self.update_plotter(False)

        if self._model.workspace_base_unit in self._UNIT_OPTIONS:
            self._view.set_unit_combo_box_index(self._UNIT_OPTIONS.index(self._model.workspace_base_unit))

    def _setup_component_tree(self) -> None:
        component_tree_model = ComponentTreeModel(self._model.workspace)
        self._component_tree_presenter = ComponentTreePresenter(
            self._view.component_tree, component_tree_model, self.on_component_tree_item_selected
        )
        self._view.component_tree.subscribe_presenter(self._component_tree_presenter)

    def _create_and_add_monitor_mesh(self) -> Optional[pv.PolyData]:
        if len(self._model.monitor_positions) == 0 or not self._view.is_show_monitors_checkbox_checked():
            return None
        return self._create_and_add_component_point_mesh(np.array(self._model.monitor_positions), self.monitor_colour)

    def _create_and_add_sample_mesh(self) -> Optional[pv.PolyData]:
        if self._model.sample_position is None or not self._view.is_show_sample_position_checkbox_checked():
            return None
        if self._model.sample_shape is not None:
            n = len(self._model.sample_shape)
            vertices = self._model.sample_shape.reshape(-1, 3)
            faces = np.c_[np.full(n, 3, dtype=int), np.arange(n * 3).reshape(n, 3)]
            sample_shape_mesh = pv.PolyData(vertices, faces)
            sample_shape_mesh["colours"] = self.generate_single_colour(n, self.sample_position_colour, 0.5)
            self._view.add_rgba_mesh(sample_shape_mesh, scalars="colours")
            return sample_shape_mesh
        return self._create_and_add_component_point_mesh(np.array([self._model.sample_position]), self.sample_position_colour)

    def _create_and_add_component_point_mesh(self, points: np.ndarray, colour: tuple[int, int, int]) -> pv.PolyData:
        point_cloud = self.create_poly_data_mesh(points)
        point_cloud["colours"] = self.generate_single_colour(len(points), colour, 1)
        self._view.add_rgba_mesh(point_cloud, scalars="colours")
        return point_cloud

    def on_export_workspace_clicked(self) -> None:
        self._model.save_line_plot_workspace_to_ads()

    def on_sum_spectra_checkbox_clicked(self) -> None:
        self._update_line_plot_ws_and_draw(self._view.current_selected_lineplot_unit())

    def available_unit_options(self) -> list[str]:
        if self._model.has_unit:
            return self._UNIT_OPTIONS
        return ["No units"]

    @property
    def workspace_display_unit(self) -> str:
        if self._model.has_unit:
            return self._model.workspace_x_unit_display
        return ""

    def integration_limits_in_current_unit(self) -> tuple[float, float]:
        return self._model.integration_limits

    def on_integration_limits_updated(self) -> None:
        """When integration limits are changed, read the new limits and tell the presenter to update the colours accordingly"""
        self._model.integration_limits = self._view.get_integration_limits()
        self.set_view_integration_limits()
        self.on_contour_range_reset_clicked()

    def on_integration_limits_reset_clicked(self) -> None:
        self._model.calculate_and_set_full_integration_range()
        self._view.set_integration_range_limits(self._model.full_integration_limits)
        self._view.set_integration_min_max_boxes(self._model.full_integration_limits)
        self.set_view_integration_limits()

    def set_view_integration_limits(self) -> None:
        display_counts = self._transform_counts(self._model.detector_counts)
        self._renderer.set_detector_scalars(self._detector_mesh, display_counts, self._counts_label)
        self.on_contour_range_reset_clicked()
        self.refresh_plotter_peaks()
        self._update_line_plot_ws_and_draw(self._view.current_selected_lineplot_unit())

    def on_contour_limits_updated(self) -> None:
        """When contour limits are changed, read the new limits and tell the presenter to update the colours accordingly"""
        # Read limits from view (these are in the current display scale).
        lower, upper = self._view.get_contour_limits()
        # Convert back to model's linear counts if we're in Logarithmic display mode
        if self._count_scale_mode == self._LINEAR:
            self._model.counts_limits = (lower, upper)
        else:
            # Inverse of log10(counts + 1): counts = 10**value - 1
            with np.errstate(over="ignore", invalid="ignore"):
                lin_lower = 10**lower - 1
                lin_upper = 10**upper - 1
            self._model.counts_limits = (lin_lower, lin_upper)
        self._view.set_plotter_scalar_bar_range((lower, upper), self._counts_label)

    def on_contour_range_reset_clicked(self) -> None:
        self._model.counts_limits = self._model.full_counts_limits
        self.set_view_contour_limits()

    def set_view_contour_limits(self) -> None:
        transformed_limits = self._transform_counts(np.array(self._model.counts_limits))
        clim = (float(transformed_limits[0]), float(transformed_limits[1]))
        display_title = self._counts_label if self._count_scale_mode == self._LINEAR else f"log10({self._counts_label})"
        self._view.set_plotter_scalar_bar_range(clim, self._counts_label, display_title=display_title)
        self._view.set_contour_range_limits(clim)
        self._view.set_contour_min_max_boxes(clim)

    def _on_projection_option_changed(self) -> None:
        """Update the projection, enable/disable render mode combo, and select appropriate renderer."""
        self._model.projection_type = self._view.current_selected_projection()
        self._view.set_render_mode_combo_enabled(True)
        self._on_render_mode_changed(self._view.get_render_mode_option())

        if self._view.current_selected_projection() == ProjectionType.THREE_D:
            self._view.set_rubberband_zoom_checked(False)
            self._view.set_overlaid_shape_controls_checked(False)
            self._view.set_hover_pick_checked(False)

        enabled = self._view.current_selected_projection() != ProjectionType.THREE_D
        self._view.set_rubberband_zoom_enabled(enabled)
        self._view.set_overlaid_shape_controls_enabled(enabled)
        self._view.set_hover_pick_enabled(enabled)
        self._view.set_aspect_ratio_box_enabled(enabled)
        self._view.set_flip_beam_box_enabled(enabled)

    def on_projection_option_changed(self) -> None:
        self._callback_queue.put((self._on_projection_option_changed, ()))

    def update_plotter(self, refresh_limits=True) -> None:
        if self._closing:
            return
        self._model.projection_type = self._view.current_selected_projection()
        self._model.flip_beam = self._view.is_flip_beam_checkbox_checked()
        with SuppressRendering(self._view.main_plotter):
            self._update_view_main_plotter(refresh_limits=refresh_limits)
            self.refresh_plotter_peaks()

    def count_scale_combo_options(self) -> list[str]:
        return [self._LINEAR, self._LOGARITHMIC]

    def on_count_scale_selected(self, _index) -> None:
        """Handler for count scale combo box changes."""
        text = self._view.current_selected_count_scale()
        if text in (self._LINEAR, self._LOGARITHMIC):
            self._count_scale_mode = text
            self.set_view_integration_limits()

    def _transform_counts(self, counts: np.ndarray) -> np.ndarray:
        """Return counts transformed for display according to selected scale."""
        if self._count_scale_mode == self._LINEAR:
            return counts
        # Logarithmic: use base-10 log with +1 offset to avoid -inf at zero
        # Preserve NaNs/infs if present
        with np.errstate(divide="ignore", invalid="ignore"):
            return np.log10(counts + 1)

    def _update_view_main_plotter(self, refresh_limits: bool) -> None:
        self._view.cache_current_camera_position()

        self._view.clear_main_plotter()
        renderer = self._renderer

        self._detector_mesh = renderer.build_detector_mesh(self._model.detector_positions, self._model.flip_beam, self._model)
        display_counts = self._transform_counts(self._model.detector_counts)
        renderer.set_detector_scalars(self._detector_mesh, display_counts, self._counts_label)
        renderer.add_detector_mesh_to_plotter(self._view.main_plotter, self._detector_mesh, scalars=self._counts_label)

        self._pickable_mesh = renderer.build_pickable_mesh(self._model.detector_positions, self._model.flip_beam)
        renderer.set_pickable_scalars(self._pickable_mesh, self._model.picked_visibility, self._visible_label)
        renderer.add_pickable_mesh_to_plotter(self._view.main_plotter, self._pickable_mesh, scalars=self._visible_label)

        self._masked_mesh = renderer.build_masked_mesh(self._model.masked_positions, self._model.flip_beam, self._model)
        renderer.add_masked_mesh_to_plotter(self._view.main_plotter, self._masked_mesh)

        monitor_mesh = self._create_and_add_monitor_mesh()
        sample_position_mesh = self._create_and_add_sample_mesh()

        self._view.enable_parallel_projection()

        # Update transform needs to happen after adding to plotter
        # Uses display coordinates
        self._update_transform()
        for mesh in [self._detector_mesh, self._pickable_mesh, self._masked_mesh, monitor_mesh, sample_position_mesh]:
            if mesh is not None:
                mesh.transform(self._transform, inplace=True)

        # If refreshing the limits we reset both the contour and integration sliders.
        # If not, we need to manually update the contour limits to what they were set to before we added the
        # meshes above, because adding the meshes resets the contour limits in the plotter.
        if refresh_limits:
            self.on_integration_limits_reset_clicked()
        else:
            self.on_contour_limits_updated()

        self._view.reset_camera()

        # Reload styles after camera reset for correct camera defaults
        self.reload_interactor_styles()

        self._view.set_camera_to_cached_state()
        self._view.cache_current_selected_projection()

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

        # Safeguard against division by zero
        mesh_width = mesh_width if mesh_width > 0 else window_width
        mesh_height = mesh_height if mesh_height > 0 else window_height

        return self._scale_matrix_relative_to_centre((min_point + max_point) / 2, window_width / mesh_width, window_height / mesh_height)

    def _scale_matrix_relative_to_centre(self, centre, scale_x=1.0, scale_y=1.0) -> np.ndarray:
        # Translate to centre, scale, translate back
        # The matrix below is the product of those three transformations
        c_x, c_y, _ = centre
        return np.array([[scale_x, 0, 0, c_x * (1 - scale_x)], [0, scale_y, 0, c_y * (1 - scale_y)], [0, 0, 1, 0], [0, 0, 0, 1]])

    def _transform_vectors_with_matrix(self, points: np.ndarray, transform: np.ndarray) -> np.ndarray:
        if points.size == 0:
            return points
        # The transform is a 4x4 matrix, the points are 3D vectors, first we need an extra
        # entry on the points
        transformed_points = np.hstack([points, np.ones((points.shape[0], 1))])
        transformed_points = transformed_points @ transform.T
        # Now remove extra point
        return transformed_points[:, :3]

    def on_aspect_ratio_check_box_clicked(self) -> None:
        self._view.store_maintain_aspect_ratio_option()
        self.update_plotter()
        self._view.reset_camera()

    def on_flip_beam_check_box_clicked(self) -> None:
        self._view.store_flip_beam_option()
        self.update_plotter(refresh_limits=False)
        self._view.reset_camera()

    @property
    def _detector_mesh_bounds(self) -> list[float]:
        # Output format matches vtk's mesh.GetBounds()
        meshes_bounds = np.vstack([self._detector_mesh.bounds, self._masked_mesh.bounds])
        min_point = np.min(meshes_bounds[:, 0::2], axis=0)
        max_point = np.max(meshes_bounds[:, 1::2], axis=0)
        # Return list of xmin, xmax, ymin, ymax, zmin, zmax
        return [x for pair in zip(min_point, max_point) for x in pair]

    def on_rubberband_zoom_toggled(self, checked: bool) -> None:
        if checked:
            self._view.set_start_adding_peaks_checked(False)
            self._view.set_hover_pick_checked(False)
            self._view.delete_current_overlaid_shape()
        self._view.set_overlaid_shape_controls_enabled(not checked)
        self._update_interactor_style()

    def on_hover_pick_toggled(self, checked: bool) -> None:
        if checked:
            self._view.set_start_adding_peaks_checked(False)
            self._view.set_rubberband_zoom_checked(False)
            self._view.delete_current_overlaid_shape()
            self._view.clear_lineplot_overlays()
            self._view.show_plot_for_detectors(self._model.line_plot_workspace, self._model.lineplot_limits)
            self._view.set_selected_detector_info([])
            self._view.set_relative_detector_angle(None)
            self._view.remove_peak_cursor_from_lineplot()

        self._view.set_clear_point_picked_detectors_disabled(checked)
        self._view.set_sum_spectra_checkbox_disabled(checked)
        self._view.set_select_bank_tube_disabled(checked)
        self._view.set_export_workspace_button_disabled(checked)
        self._view.set_overlaid_shape_controls_enabled(not checked)

        self._last_hovered_point_index = None
        self._update_interactor_style()

        if checked:
            return

        self.update_picked_detectors_on_view()

    def _update_hover_pick_plot(self, point_index: int | None) -> None:
        if point_index is None:
            return
        n_pickable = int(np.count_nonzero(self._model.is_pickable))
        if point_index < 0 or point_index >= n_pickable:
            self._last_hovered_point_index = None
            return
        self._model.extract_spectra_for_line_plot(self._view.current_selected_lineplot_unit(), False, np.array([point_index]))
        detector_info = self._model.detector_info_text_for_workspace_index(point_index)
        if len(detector_info) == 0:
            return
        self._view.show_plot_for_detectors(self._model.line_plot_workspace, self._model.lineplot_limits)
        self._view.set_selected_detector_info(detector_info)
        self._view.set_relative_detector_angle(None)

    def update_picked_detectors_on_view(self) -> None:
        # Update to visibility shows up in real time
        self._renderer.set_pickable_scalars(self._pickable_mesh, self._model.picked_visibility, self._visible_label)
        self._update_line_plot_ws_and_draw(self._view.current_selected_lineplot_unit())

    def _on_clear_point_picked_detectors_clicked(self) -> None:
        self._model.clear_point_picked_detectors()
        self.update_picked_detectors_on_view()

    def on_clear_point_picked_detectors_clicked(self) -> None:
        self._callback_queue.put((self._on_clear_point_picked_detectors_clicked, ()))

    def on_overlaid_shape_added(self) -> None:
        self._update_interactor_style()
        self._view.set_add_selection_and_mask_buttons_enabled(True)

    def on_overlaid_shape_removed(self) -> None:
        self._update_interactor_style()
        self._view.set_add_selection_and_mask_buttons_enabled(False)

    def _on_add_item_clicked(self) -> None:
        centres = self._transform_vectors_with_matrix(np.array(self._model.detector_positions), self._transform)
        mask = self._view.get_shape_mask(centres)
        if not np.any(mask):
            return

        if self._select_bank_tube:
            mask = self._model.expand_pickable_mask_to_parent_subtrees(mask)
        new_key = self._model.add_new_detector_key(mask.tolist(), self._view.get_current_selected_tab())
        self._view.set_new_item_key(self._view.get_current_selected_tab(), new_key)
        self._view.set_overlaid_shape_controls_checked(False)

    def on_select_bank_tube_toggled(self, checked: bool) -> None:
        self._select_bank_tube = checked

    def on_add_item_clicked(self) -> None:
        centres = self._transform_vectors_with_matrix(np.array(self._model.detector_positions), self._transform)
        self._view.project_and_cache_detector_points(centres)
        self._callback_queue.put((self._on_add_item_clicked, ()))

    def _on_list_item_selected(self, kind: CurrentTab) -> None:
        self._model.apply_detector_items(self._view.selected_items_in_list(kind), kind)

        if kind is CurrentTab.Masking:
            self.update_plotter()
            self.on_integration_limits_reset_clicked()
            self._update_line_plot_ws_and_draw(self._view.current_selected_lineplot_unit())
        else:
            self.update_picked_detectors_on_view()

    def on_list_item_selected(self, kind: CurrentTab) -> None:
        self._callback_queue.put((self._on_list_item_selected, (kind,)))

    def _on_save_to_workspace_clicked(self) -> None:
        self._model.save_workspace_to_ads(self._view.get_current_selected_tab())

    def on_save_to_workspace_clicked(self) -> None:
        self._callback_queue.put((self._on_save_to_workspace_clicked, ()))

    def _on_apply_permanently_clicked(self) -> None:
        # Clear both lists before overwriting to workspace (reset of model)
        self._view.clear_item_list(CurrentTab.Masking)
        self._view.clear_item_list(CurrentTab.Grouping)
        self._model.overwrite_mask_to_current_workspace()

    def on_apply_permanently_clicked(self) -> None:
        self._callback_queue.put((self._on_apply_permanently_clicked, ()))

    def _on_clear_list_clicked(self) -> None:
        self._view.clear_item_list(self._view.get_current_selected_tab())
        self._model.clear_stored_keys(self._view.get_current_selected_tab())
        self.on_list_item_selected(self._view.get_current_selected_tab())

    def on_clear_list_clicked(self) -> None:
        self._callback_queue.put((self._on_clear_list_clicked, ()))

    def _on_save_mask_to_xml_clicked(self):
        filename = self._view.get_filename_from_dialog(self._XML_FILE_FILTER)
        if not filename:
            return
        self._model.save_mask_to_xml(filename)

    def on_save_mask_to_xml_clicked(self):
        self._callback_queue.put((self._on_save_mask_to_xml_clicked, ()))

    def _on_save_mask_to_cal_clicked(self):
        filename = self._view.get_filename_from_dialog(self._CAL_FILE_FILTER)
        if not filename:
            return
        self._model.save_mask_to_cal(filename)

    def on_save_mask_to_cal_clicked(self):
        self._callback_queue.put((self._on_save_mask_to_cal_clicked, ()))

    def _on_save_grouping_to_ads_clicked(self):
        self._model.save_grouping_to_ads()

    def on_save_grouping_to_ads_clicked(self):
        self._callback_queue.put((self._on_save_grouping_to_ads_clicked, ()))

    def _on_save_grouping_to_xml_clicked(self):
        filename = self._view.get_filename_from_dialog(self._XML_FILE_FILTER)
        if not filename:
            return
        self._model.save_grouping_to_xml(filename)

    def on_save_grouping_to_xml_clicked(self):
        self._callback_queue.put((self._on_save_grouping_to_xml_clicked, ()))

    def _on_save_grouping_to_cal_clicked(self):
        filename = self._view.get_filename_from_dialog(self._CAL_FILE_FILTER)
        if not filename:
            return
        self._model.save_grouping_to_cal(filename)

    def on_save_grouping_to_cal_clicked(self):
        self._callback_queue.put((self._on_save_grouping_to_cal_clicked, ()))

    def _reload_mask_workspaces(self) -> None:
        self._view.refresh_workspaces_in_list(CurrentTab.Masking)
        self.on_list_item_selected(CurrentTab.Masking)

    def _reload_grouping_workspaces(self) -> None:
        self._view.refresh_workspaces_in_list(CurrentTab.Grouping)
        self.on_list_item_selected(CurrentTab.Grouping)

    def get_list_keys_from_workspaces_in_ads(self, kind: CurrentTab):
        if kind is CurrentTab.Masking:
            # Mask list shows workspace names
            return [ws.name() for ws in self._model.get_workspaces_in_ads_of_type(MaskWorkspace)]
        else:
            # Grouping list shows an entry per group in grouping workspace
            return self._model.get_grouping_keys_from_workspaces_in_ads()

    def cached_keys(self, kind: CurrentTab) -> list[str]:
        return self._model.cached_keys(kind)

    def _update_line_plot_ws_and_draw(self, unit: str) -> None:
        if self._view.is_hover_pick_mode_checked():
            if self._last_hovered_point_index is not None:
                self._update_hover_pick_plot(self._last_hovered_point_index)
            return

        self._model.extract_spectra_for_line_plot(unit, self._view.sum_spectra_selected())
        self._view.show_plot_for_detectors(self._model.line_plot_workspace, self._model.lineplot_limits)
        self._view.set_selected_detector_info(self._model.picked_detectors_info_text())
        self._update_relative_detector_angle()
        self.refresh_lineplot_peaks()
        if self._model.peak_picking_enabled():
            self._view.add_peak_cursor_to_lineplot()

    def _update_relative_detector_angle(self) -> None:
        if len(self._model.picked_detector_ids) != 2:
            self._view.set_relative_detector_angle(None)
        else:
            self._view.set_relative_detector_angle(self._model.relative_detector_angle())

    def create_poly_data_mesh(self, points: np.ndarray, faces=None) -> pv.PolyData:
        """Create a PyVista mesh from the given points and faces"""
        mesh = pv.PolyData(points, faces)
        return mesh

    def generate_single_colour(self, number_of_points: int, colour: tuple[int, int, int], alpha: float) -> np.ndarray:
        """Returns an RGBA colours array for the given set of points, with all points the same colour"""
        rgba = np.zeros((number_of_points, 4))
        rgba[:, 0] = float(colour[0]) / 255.0  # red
        rgba[:, 1] = float(colour[1]) / 255.0  # green
        rgba[:, 2] = float(colour[2]) / 255.0  # blue
        rgba[:, 3] = alpha
        return rgba

    def _reload_peaks_workspaces(self):
        self._view.refresh_peaks_ws_list()
        self.on_peaks_workspace_selected()

    def _delete_workspace_callback(self, ws_name):
        if self._model.workspace.name() == ws_name:
            self._view.close_window()
            logger.warning(f"Workspace {ws_name} deleted, closed Experimental Instrument View.")
        else:
            self._reload_everything()

    def delete_workspace_callback(self, ws_name):
        self._callback_queue.put((self._delete_workspace_callback, (ws_name,)))

    def _rename_workspace_callback(self, ws_old_name, ws_new_name):
        if self._model._workspace.name() == ws_old_name:
            self._model._workspace = mtd[ws_new_name]
            self._model.setup()
            self._setup_component_tree()
            logger.warning(f"Workspace {ws_old_name} renamed to {ws_new_name}, updated Experimental Instrument View.")

        self._reload_everything()

    def rename_workspace_callback(self, ws_old_name, ws_new_name):
        self._callback_queue.put((self._rename_workspace_callback, (ws_old_name, ws_new_name)))

    def clear_workspace_callback(self):
        self._view.close_window()

    def _replace_workspace_callback(self, ws_name, ws):
        if isinstance(ws, PeaksWorkspace):
            self._reload_peaks_workspaces()
        elif isinstance(ws, MaskWorkspace):
            self._reload_mask_workspaces()
        elif isinstance(ws, GroupingWorkspace):
            self._reload_grouping_workspaces()
        elif ws_name == self._model.workspace.name():
            self._reset_model_workspace(ws_name)

    def _reset_model_workspace(self, ws_name):
        self._model._workspace = AnalysisDataService.retrieve(ws_name)
        self._model.setup()
        self._setup_component_tree()
        self._reload_renderers()  # Clear cached renderers before rendering
        self.update_plotter()

    def replace_workspace_callback(self, ws_name, ws):
        self._callback_queue.put((self._replace_workspace_callback, (ws_name, ws)))

    def _add_workspace_callback(self, ws_name, ws):
        if isinstance(ws, PeaksWorkspace):
            self._reload_peaks_workspaces()
        elif isinstance(ws, MaskWorkspace):
            self._reload_mask_workspaces()
        elif isinstance(ws, GroupingWorkspace):
            self._reload_grouping_workspaces()

    def add_workspace_callback(self, ws_name, ws):
        self._callback_queue.put((self._add_workspace_callback, (ws_name, ws)))

    def handle_close(self):
        self._closing = True
        # The observers are unsubscribed on object deletion, it's safer to manually
        # delete the observer rather than wait for the garbage collector, because
        # we don't want stale workspace references hanging around.
        if hasattr(self, "_ads_observer"):
            del self._ads_observer
        if hasattr(self, "_callback_queue"):
            self._callback_queue.put(self._callback_stop_sentinel)
        # Drop presenter->model reference on close while keeping _model non-optional for static typing.
        self._model = cast(FullInstrumentViewModel, None)

    def on_sliders_unit_selected(self, value) -> None:
        self._model.set_integration_units(self._UNIT_OPTIONS[value])
        self._update_line_plot_ws_and_draw(self._UNIT_OPTIONS[value])
        self.on_integration_limits_reset_clicked()

    def on_lineplot_unit_selected(self, value) -> None:
        self._update_line_plot_ws_and_draw(self._UNIT_OPTIONS[value])

    def peaks_workspaces_in_ads(self) -> list[str]:
        return [ws.name() for ws in self._model.get_workspaces_in_ads_of_type(PeaksWorkspace)]

    def on_peaks_workspace_selected(self) -> None:
        self.refresh_plotter_peaks()
        self.refresh_lineplot_peaks()

    def refresh_plotter_peaks(self) -> None:
        self._view.clear_overlay_meshes()
        pos, labels, selected_peaks_workspaces = self._model.get_peak_overlay_arguments(self._view.selected_peaks_workspaces())
        transformed_pos = [self._transform_vectors_with_matrix(p, self._transform) for p in pos]
        self._view.plot_overlay_meshes(transformed_pos, labels, selected_peaks_workspaces)

    def refresh_lineplot_peaks(self) -> None:
        # Plot vertical lines on the lineplot if the peak detector is selected
        self._view.clear_lineplot_overlays()
        self._view.plot_lineplot_peak_overlays(*self._model.get_peak_lineplot_overlay_arguments(self._view.selected_peaks_workspaces()))
        self._view.redraw_lineplot()

    def on_start_adding_peaks_toggled(self, checked) -> None:
        if checked:
            self._model.turn_on_single_point_picking()
            self._view.set_rubberband_zoom_checked(False)
            self._view.set_hover_pick_checked(False)
            self._view.add_peak_cursor_to_lineplot()
            self._view.disable_and_uncheck_selection_list()
        else:
            self._model.turn_off_single_point_picking()
            self._view.remove_peak_cursor_from_lineplot()
            self._view.enable_and_restore_selection_list()

        self._on_list_item_selected(CurrentTab.Grouping)
        self._view.set_list_enabled(CurrentTab.Masking, not checked)
        self._view.set_delete_all_selected_peaks_button_enabled(not checked)
        self._view.set_overlaid_shape_controls_enabled(not checked)

    def on_peak_selected_in_lineplot(self, x: float, mouse_click: Literal["right", "left"]) -> None:
        if len(self._model.picked_detector_ids) == 0:
            return
        if mouse_click == "left":
            peaks_ws = self._model.add_peak(x, self._view.selected_peaks_workspaces())
            # Trigger selection of peak ws must happen after the callbacks from add peak are complete
            self._callback_queue.put((self._view.select_peaks_workspace, (peaks_ws,)))
        elif mouse_click == "right":
            self._model.delete_peak(x, self._view.selected_peaks_workspaces())

    def on_delete_all_selected_peaks_clicked(self) -> None:
        self._model.delete_peaks_on_all_selected_detectors(self._view.selected_peaks_workspaces())

    def on_show_monitors_check_box_clicked(self) -> None:
        self.update_plotter()

    def on_show_sample_position_check_box_clicked(self) -> None:
        self.update_plotter()

    def on_component_tree_item_selected(self, component_indices: np.ndarray) -> None:
        self._model.component_tree_indices_selected(component_indices)
        self.update_plotter()

    def reload_interactor_styles(self):
        def point_hovered(point_index: int | None) -> None:
            if point_index is None or point_index == self._last_hovered_point_index:
                return
            self._last_hovered_point_index = point_index
            self._update_hover_pick_plot(point_index)
            return

        def detector_picked(detector_index: int) -> None:
            self._model.update_point_picked_detectors(detector_index, self._select_bank_tube)
            self.update_picked_detectors_on_view()
            return

        wrapped_picking_callback = self._renderer.get_callback_tied_to_detector_index(
            self._view.main_plotter, callback=detector_picked, hover=False
        )
        wrapped_hover_callback = self._renderer.get_callback_tied_to_detector_index(
            self._view.main_plotter, callback=point_hovered, hover=True
        )

        self._interactor_styles = InteractorStyles(
            self._view.main_plotter, picking_callback=wrapped_picking_callback, hover_callback=wrapped_hover_callback
        )
        self._update_interactor_style()

    def _update_interactor_style(self):
        if not self._model.is_2d_projection:
            self._view.main_plotter.iren.style = self._interactor_styles.TRACKBALL
            return

        if self._view.is_active_current_overlaid_shape():
            self._view.main_plotter.iren.style = self._interactor_styles.SCROLL_ZOOM_NO_PICKING
            return

        if self._view.is_hover_pick_mode_checked():
            self._view.main_plotter.iren.style = self._interactor_styles.SCROLL_ZOOM_WITH_HOVER
        elif self._view.is_rubberband_zoom_toggled():
            self._view.main_plotter.iren.style = self._interactor_styles.RUBBERBAND_ZOOM
        else:
            self._view.main_plotter.iren.style = self._interactor_styles.SCROLL_ZOOM_WITH_PICKING

    def _get_renderer_for_mode(self, mode: str):
        if mode == self._view._RENDER_MODE_POINTS:
            return self._point_cloud_renderer

        is_sbs = self._model.projection_type == ProjectionType.SIDE_BY_SIDE
        if mode == self._view._RENDER_MODE_RAW_SHAPES:
            return self._sbs_shape_renderer_full if is_sbs else self._shape_renderer_full

        return self._sbs_shape_renderer if is_sbs else self._shape_renderer

    def _on_render_mode_changed(self, mode: str) -> None:
        self._renderer = self._get_renderer_for_mode(mode)
        self.update_plotter()

    def on_render_mode_changed(self, index: int) -> None:
        self._view.store_render_mode_option()
        mode = self._view.get_render_mode_option()
        self._callback_queue.put((self._on_render_mode_changed, (mode,)))

    def _reload_renderers(self) -> None:
        """
        Called when the workspace changes to ensure renderers recompute geometry
        from the new workspace data.
        """
        self._point_cloud_renderer = PointCloudRenderer()
        self._shape_renderer = ShapeRenderer(self._model.workspace)
        self._shape_renderer_full = ShapeRenderer(self._model.workspace, use_optimised_shapes=False)
        self._sbs_shape_renderer = SideBySideShapeRenderer(self._model.workspace)
        self._sbs_shape_renderer_full = SideBySideShapeRenderer(self._model.workspace, use_optimised_shapes=False)
        self._on_render_mode_changed(self._view.get_render_mode_option())

    def _reload_everything(self) -> None:
        """Reload all workspace-dependent data (peaks, masks, groupings) and clear renderer cache.

        Called when workspaces are added to or removed from the ADS.
        """
        if self._closing:
            return
        with SuppressRendering(self._view.main_plotter):
            self._reload_peaks_workspaces()
            self._reload_mask_workspaces()
            self._reload_grouping_workspaces()
            # Reload renderers only after updating the lists in the view
            # Otherwise can trigger a plotter update when lists are not in sync with ADS
            self._reload_renderers()
