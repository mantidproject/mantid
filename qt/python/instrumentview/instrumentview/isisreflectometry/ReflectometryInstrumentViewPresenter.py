# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.isisreflectometry.ReflectometryInstrumentViewView import ReflectometryInstrumentViewView
from instrumentview.renderers.shape_renderer import ShapeRenderer
from instrumentview.Projections.ProjectionType import ProjectionType
from qtpy.QtCore import QObject, QMetaObject, Qt
import numpy as np
from typing import Optional


class ReflectometryInstrumentViewPresenter:
    """Presenter that wraps a pyvista-based instrument view for the
    ISISReflectometry preview tab.

    Renders the instrument directly using FullInstrumentViewModel and
    PointCloudRenderer, without the full FullInstrumentViewPresenter
    (no peaks, masking, grouping, component tree, etc.).
    """

    _COUNTS_LABEL = "Integrated Counts"
    _VISIBLE_LABEL = "Visible Picked"

    def __init__(self, view: Optional[ReflectometryInstrumentViewView] = None):
        self.view = view or ReflectometryInstrumentViewView()
        self._model: Optional[FullInstrumentViewModel] = None
        self._transform: Optional[np.ndarray] = None
        self._original_mesh_bounds: Optional[tuple] = None
        self._rect_selected_detector_ids: list[int] = []

    def update_workspace(self, workspace):
        """Set up the model from the workspace and render the instrument."""
        self.view.initialise()
        self._model = FullInstrumentViewModel(workspace)
        self._model.setup()
        self._model.projection_type = ProjectionType.CYLINDRICAL_Y
        self._renderer = ShapeRenderer(workspace)
        self._renderer.precompute()
        self._render()

    def reset(self):
        """Clear and re-initialise the view."""
        if self.view.main_plotter is not None:
            self.view.main_plotter.clear()
        self.view.remove_shape()
        self.view.set_on_resize_callback(None)
        self._transform = None
        self._original_mesh_bounds = None
        self._rect_selected_detector_ids = []
        self._model = None

    def plot(self):
        """Re-render the current instrument."""
        if self._model is not None:
            self._render()

    def set_zoom_mode(self):
        """Set the plotter interaction to zoom/pan mode."""
        self.view.remove_shape()
        plotter = self.view.main_plotter
        if plotter is not None and self._model is not None:
            self._renderer.set_interactive_style(plotter, self._model.is_2d_projection)

    def set_select_rect_mode(self):
        """Set the plotter interaction to rectangle selection mode."""
        plotter = self.view.main_plotter
        if plotter is not None:
            self.view.overlay_rectangle(on_shape_changed=self._on_rect_shape_changed)

    def get_rect_selected_detector_ids(self) -> list[int]:
        """Return the detector IDs currently within the rectangle selection."""
        return list(self._rect_selected_detector_ids)

    def _on_rect_shape_changed(self) -> None:
        """Recompute which detectors fall inside the rectangle and store their IDs."""
        if self._model is None or self._transform is None:
            return
        mgr = self.view.shape_overlay_manager
        if mgr is None:
            return
        positions = self._model.detector_positions
        if len(positions) == 0:
            self._rect_selected_detector_ids = []
            return
        ones = np.ones((len(positions), 1))
        transformed = (self._transform @ np.hstack([positions, ones]).T).T[:, :3]
        mask = mgr.get_shape_mask(transformed)
        all_ids = self._model.all_detector_ids
        self._rect_selected_detector_ids = [int(all_ids[i]) for i in np.where(mask)[0]]

        relay = self.view.findChild(QObject, "ShapeChangedRelay")
        if relay is not None:
            QMetaObject.invokeMethod(relay, "notify", Qt.DirectConnection)

    def selected_detector_ids(self) -> list[int]:
        return self._rect_selected_detector_ids

    def _render(self):
        """Render the instrument into the view's plotter."""
        if self._model is None:
            return

        plotter = self.view.main_plotter
        plotter.clear()

        self._detector_mesh = self._renderer.build_detector_mesh(self._model.detector_positions, self._model.flip_z, self._model)
        self._renderer.set_detector_scalars(self._detector_mesh, self._model.detector_counts, self._COUNTS_LABEL)
        self._renderer.add_detector_mesh_to_plotter(plotter, self._detector_mesh, scalars=self._COUNTS_LABEL, show_scalar_bar=False)

        # Store the original (pre-fill-transform) bounds so _apply_fill_transform
        # can undo/redo the fill when the dock is resized.
        self._original_mesh_bounds = self._detector_mesh.bounds
        self._transform = np.eye(4)

        self._renderer.set_parallel_view(plotter)
        plotter.reset_camera()
        self._renderer.set_interactive_style(plotter, self._model.is_2d_projection)

        # Schedule the fill transform via resizeEvent so it runs after Qt/VTK
        # have applied the correct dock dimensions to the VTK render window.
        self.view.set_on_resize_callback(self._apply_fill_transform)

    def _apply_fill_transform(self):
        """Stretch the detector mesh to fill the viewport.

        Called from the view's resizeEvent (deferred by one event-loop cycle so
        QVTKRenderWindowInteractor.resizeEvent has already updated the VTK
        render window dimensions via vtkRenderWindow.SetSize).
        """
        if self._detector_mesh is None or self._original_mesh_bounds is None:
            return
        plotter = self.view.main_plotter
        if plotter is None:
            return

        w, h = plotter.ren_win.GetSize()
        if w <= 0 or h <= 0:
            return

        xmin, xmax, ymin, ymax, zmin, zmax = self._original_mesh_bounds
        mesh_width = xmax - xmin
        mesh_height = ymax - ymin
        if mesh_width <= 0 or mesh_height <= 0:
            return

        # In VTK parallel projection, parallel_scale is the world-space half-height of
        # the viewport.  VTK does NOT change it when the pixel dimensions change (it
        # is a camera property, not a viewport property), so the value established by
        # reset_camera() in _render() stays valid after every resize.  We read it
        # directly here rather than calling reset_camera() again, because reset_camera()
        # internally calls Render() which would paint the unfilled mesh on screen and
        # cause a visible flash.
        parallel_scale = plotter.camera.parallel_scale

        # Derive the visible world rectangle from the (unchanged) camera.
        # In parallel projection: visible_height = 2 * parallel_scale
        # visible_width = visible_height * (viewport_width / viewport_height)
        visible_height = 2.0 * parallel_scale
        visible_width = visible_height * w / h

        centre = np.array([(xmin + xmax) / 2, (ymin + ymax) / 2, (zmin + zmax) / 2])
        new_transform = self._scale_matrix_relative_to_centre(centre, visible_width / mesh_width, visible_height / mesh_height)

        # Compute the delta transform (undo old fill, apply new fill) as a single
        # matrix and apply it in ONE operation.  Splitting this into two separate
        # transform() calls would risk an intermediate Render() (from PyVista's
        # background timer or a Qt paint event) showing the unfilled mesh and causing
        # a visible flash.
        if not np.allclose(self._transform, np.eye(4)):
            combined = new_transform @ np.linalg.inv(self._transform)
        else:
            combined = new_transform
        self._transform = new_transform
        self._detector_mesh.transform(combined, inplace=True)

        # Render once with the correct filled state.
        plotter.render_window.Render()

        # Update the cached default in CursorZoomInteractorStyle so that right-clicking
        # resets to this state rather than the stale pre-fill-transform scale.
        style = plotter.iren.style
        if hasattr(style, "update_default_camera_state"):
            style.update_default_camera_state()

    @staticmethod
    def _scale_matrix_relative_to_centre(centre, scale_x=1.0, scale_y=1.0) -> np.ndarray:
        # Translate to centre, scale, translate back
        # The matrix below is the product of those three transformations
        c_x, c_y, _ = centre
        return np.array([[scale_x, 0, 0, c_x * (1 - scale_x)], [0, scale_y, 0, c_y * (1 - scale_y)], [0, 0, 1, 0], [0, 0, 0, 1]])
