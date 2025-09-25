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


class FullInstrumentViewPresenter:
    """Presenter for the Instrument View window"""

    _FULL_3D = "3D"
    _SPHERICAL_X = "Spherical X"
    _SPHERICAL_Y = "Spherical Y"
    _SPHERICAL_Z = "Spherical Z"
    _CYLINDRICAL_X = "Cylindrical X"
    _CYLINDRICAL_Y = "Cylindrical Y"
    _CYLINDRICAL_Z = "Cylindrical Z"
    _PROJECTION_OPTIONS = [_FULL_3D, _SPHERICAL_X, _SPHERICAL_Y, _SPHERICAL_Z, _CYLINDRICAL_X, _CYLINDRICAL_Y, _CYLINDRICAL_Z]

    _TIME_OF_FLIGHT = "TOF"
    _D_SPACING = "dSpacing"
    _WAVELENGTH = "Wavelength"
    _MOMENTUM_TRANSFER = "MomentumTransfer"
    _UNIT_OPTIONS = [_TIME_OF_FLIGHT, _D_SPACING, _WAVELENGTH, _MOMENTUM_TRANSFER]

    def __init__(self, view: FullInstrumentViewWindow, model: FullInstrumentViewModel):
        """For the given workspace, use the data from the model to plot the detectors. Also include points at the origin and
        any monitors."""
        self._view = view
        self._model = model
        self._model.setup()
        self.setup()

    def setup(self):
        self._view.subscribe_presenter(self)
        default_index, options = self.projection_combo_options()
        self._view.set_projection_combo_options(default_index, options)
        self._view.setup_connections_to_presenter()
        self._view.set_contour_range_limits(self._model.counts_limits)
        self._view.set_tof_range_limits(self._model.tof_limits)

        if len(self._model.monitor_positions) > 0:
            monitor_point_cloud = self.create_poly_data_mesh(self._model.monitor_positions)
            monitor_point_cloud["colours"] = self.generate_single_colour(len(self._model.monitor_positions), 1, 0, 0, 1)
            self._view.add_rgba_mesh(monitor_point_cloud, scalars="colours")

        self._counts_label = "Integrated Counts"
        self._visible_label = "Visible Picked"

        self._view.show_axes()
        self.on_projection_option_selected(default_index)

        if self._model.workspace_x_unit in self._UNIT_OPTIONS:
            self._view.set_unit_combo_box_index(self._UNIT_OPTIONS.index(self._model.workspace_x_unit))

        self._view.hide_status_box()
        self._ads_observer = InstrumentViewADSObserver(
            delete_callback=self.delete_workspace_callback,
            rename_callback=self.rename_workspace_callback,
            clear_callback=self.clear_workspace_callback,
            replace_callback=self.replace_workspace_callback,
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
        return default_index, self._PROJECTION_OPTIONS

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

    def on_tof_limits_updated(self) -> None:
        """When TOF limits are changed, read the new limits and tell the presenter to update the colours accordingly"""
        self._model.tof_limits = self._view.get_tof_limits()
        self.set_view_tof_limits()

    def set_view_tof_limits(self) -> None:
        self._detector_mesh[self._counts_label] = self._model.detector_counts

    def on_contour_limits_updated(self) -> None:
        """When contour limits are changed, read the new limits and tell the presenter to update the colours accordingly"""
        self._model.counts_limits = self._view.get_contour_limits()
        self.set_view_contour_limits()

    def set_view_contour_limits(self) -> None:
        self._view.set_plotter_scalar_bar_range(self._model.counts_limits, self._counts_label)

    def on_projection_option_selected(self, selected_index: int) -> None:
        """Update the projection based on the selected option."""
        projection_type = self._PROJECTION_OPTIONS[selected_index]

        if projection_type.startswith("3D"):
            # Plot orange sphere at the origin
            # origin = pv.Sphere(radius=0.1, center=[0, 0, 0])
            # self._view.add_simple_shape(origin, colour="orange", pickable=False)
            self._view.enable_rectangle_picking_checkbox()
            self._update_view_main_plotter(self._model.detector_positions, is_projection=False)
            return

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
        self._view.disable_rectangle_picking_checkbox()
        self._update_view_main_plotter(self._model.detector_projection_positions, is_projection=True)

    def _update_view_main_plotter(self, positions: np.ndarray, is_projection: bool):
        self._detector_mesh = self.create_poly_data_mesh(positions)
        self._detector_mesh[self._counts_label] = self._model.detector_counts
        self._view.add_main_mesh(self._detector_mesh, is_projection=is_projection, scalars=self._counts_label)

        self._pickable_main_mesh = self.create_poly_data_mesh(positions)
        self._pickable_main_mesh[self._visible_label] = self._model.picked_visibility
        self._view.add_pickable_main_mesh(self._pickable_main_mesh, scalars=self._visible_label)

        self._view.enable_point_picking(callback=self.point_picked)
        self.set_view_contour_limits()
        self.set_view_tof_limits()
        self._view.reset_camera()

    def on_multi_select_detectors_clicked(self, state: int) -> None:
        """Change between single and multi point picking"""
        if state == 2:
            self._view.enable_rectangle_picking(callback=self.rectangle_picked)
        else:
            self._view.enable_point_picking(callback=self.point_picked)

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

    def delete_workspace_callback(self, ws_name):
        if self._model._workspace.name() != ws_name:
            return
        self._view.close()
        logger.warning(f"Workspace {ws_name} deleted, closed Experimental Instrument View.")

    def rename_workspace_callback(self, ws_old_name, ws_new_name):
        if self._model._workspace.name() != ws_old_name:
            return
        self._model._workspace = mtd[ws_new_name]
        logger.warning("Workspace renamed, updated Experimental Instrument View.")

    def clear_workspace_callback(self):
        self._view.close()

    def replace_workspace_callback(self, ws_name, ws):
        self._view.close()

    def handle_close(self):
        # The observers are unsubscribed on object deletion, it's safer to manually
        # delete the observer rather than wait for the garbage collector, because
        # we don't want stale workspace references hanging around.
        if hasattr(self, "_ads_observer"):
            del self._ads_observer

    def on_unit_option_selected(self, value) -> None:
        self._update_line_plot_ws_and_draw(self._UNIT_OPTIONS[value])
