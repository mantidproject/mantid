# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from unittest.mock import MagicMock

import numpy as np

from instrumentview.isisreflectometry.ReflectometryInstrumentViewPresenter import ReflectometryInstrumentViewPresenter
from instrumentview.Projections.ProjectionType import ProjectionType
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel


def _make_presenter():
    """Return a presenter with a fully mocked view."""
    mock_view = MagicMock()
    mock_view.main_plotter = MagicMock()
    mock_view.shape_overlay_manager = None
    return ReflectometryInstrumentViewPresenter(view=mock_view), mock_view


class TestReflectometryInstrumentViewPresenter(unittest.TestCase):
    def setUp(self):
        self._presenter, self._mock_view = _make_presenter()

    def test_view_is_stored(self):
        self.assertIs(self._presenter.view, self._mock_view)

    def test_model_is_none_initially(self):
        self.assertIsNone(self._presenter._model)

    def test_transform_is_none_initially(self):
        self.assertIsNone(self._presenter._transform)

    def test_rect_selected_detector_ids_is_empty_initially(self):
        self.assertEqual(self._presenter._rect_selected_detector_ids, [])

    def test_default_view_created_when_not_provided(self):
        """Passing no view should create a ReflectometryInstrumentViewView."""
        with mock.patch(
            "instrumentview.isisreflectometry.ReflectometryInstrumentViewPresenter.ReflectometryInstrumentViewView"
        ) as mock_view_cls:
            presenter = ReflectometryInstrumentViewPresenter()
            mock_view_cls.assert_called_once()
            self.assertIs(presenter.view, mock_view_cls.return_value)

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewPresenter.ShapeRenderer")
    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewPresenter.FullInstrumentViewModel")
    def test_update_workspace_calls_view_initialise(self, mock_model_cls, mock_renderer_cls):
        mock_ws = MagicMock()
        mock_model_cls.return_value.detector_positions = np.zeros((10, 3))
        mock_model_cls.return_value.flip_z = False
        mock_model_cls.return_value.detector_counts = np.zeros(10)
        mock_model_cls.return_value.is_2d_projection = True
        mock_renderer_cls.return_value.build_detector_mesh.return_value = MagicMock(bounds=(0, 1, 0, 1, 0, 1), transform=MagicMock())
        self._presenter.update_workspace(mock_ws)
        self._mock_view.initialise.assert_called_once()

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewPresenter.ShapeRenderer")
    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewPresenter.FullInstrumentViewModel")
    def test_update_workspace_sets_model(self, mock_model_cls, mock_renderer_cls):
        mock_ws = MagicMock()
        mock_model_cls.return_value.detector_positions = np.zeros((10, 3))
        mock_model_cls.return_value.flip_z = False
        mock_model_cls.return_value.detector_counts = np.zeros(10)
        mock_model_cls.return_value.is_2d_projection = True
        mock_renderer_cls.return_value.build_detector_mesh.return_value = MagicMock(bounds=(0, 1, 0, 1, 0, 1), transform=MagicMock())
        self._presenter.update_workspace(mock_ws)
        mock_model_cls.assert_called_once_with(mock_ws)
        self.assertIsNotNone(self._presenter._model)

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewPresenter.ShapeRenderer")
    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewPresenter.FullInstrumentViewModel")
    def test_update_workspace_sets_cylindrical_projection(self, mock_model_cls, mock_renderer_cls):
        mock_ws = MagicMock()
        mock_model_cls.return_value.detector_positions = np.zeros((10, 3))
        mock_model_cls.return_value.flip_z = False
        mock_model_cls.return_value.detector_counts = np.zeros(10)
        mock_model_cls.return_value.is_2d_projection = True
        mock_renderer_cls.return_value.build_detector_mesh.return_value = MagicMock(bounds=(0, 1, 0, 1, 0, 1), transform=MagicMock())
        self._presenter.update_workspace(mock_ws)
        self.assertEqual(mock_model_cls.return_value.projection_type, ProjectionType.CYLINDRICAL_Y)

    def test_reset_clears_model(self):
        self._presenter._model = MagicMock()
        self._presenter.reset()
        self.assertIsNone(self._presenter._model)
        self._mock_view.main_plotter.clear.assert_called_once()

    def test_reset_no_plotter_does_not_raise(self):
        self._mock_view.main_plotter = None
        self._presenter.reset()  # should not raise

    def test_plot_calls_render_when_model_set(self):
        self._presenter._model = MagicMock()
        self._presenter._render = MagicMock()
        self._presenter.plot()
        self._presenter._render.assert_called_once()

    def test_plot_no_op_when_no_model(self):
        self._presenter._render = MagicMock()
        self._presenter.plot()
        self._presenter._render.assert_not_called()

    def test_set_zoom_mode_removes_shape(self):
        self._presenter._model = MagicMock()
        self._presenter._model.is_2d_projection = True
        self._presenter._renderer = MagicMock()
        self._presenter.set_zoom_mode()
        self._mock_view.remove_shape.assert_called_once()

    def test_set_zoom_mode_calls_set_interactive_style(self):
        self._presenter._model = MagicMock()
        self._presenter._model.is_2d_projection = True
        self._presenter._renderer = MagicMock()
        self._presenter.set_zoom_mode()
        self._presenter._renderer.set_interactive_style.assert_called_once_with(self._mock_view.main_plotter, True)

    def test_set_zoom_mode_no_op_when_no_plotter(self):
        self._mock_view.main_plotter = None
        self._presenter._model = MagicMock()
        self._presenter._renderer = MagicMock()
        self._presenter.set_zoom_mode()
        self._presenter._renderer.set_interactive_style.assert_not_called()

    def test_set_zoom_mode_no_op_when_no_model(self):
        self._presenter._renderer = MagicMock()
        self._presenter.set_zoom_mode()
        self._presenter._renderer.set_interactive_style.assert_not_called()

    def test_set_select_rect_mode_calls_overlay_rectangle(self):
        self._presenter.set_select_rect_mode()
        self._mock_view.overlay_rectangle.assert_called_once()

    def test_set_select_rect_mode_passes_callback(self):
        self._presenter.set_select_rect_mode()
        call_kwargs = self._mock_view.overlay_rectangle.call_args[1]
        self.assertIn("on_shape_changed", call_kwargs)
        self.assertEqual(call_kwargs["on_shape_changed"], self._presenter._on_rect_shape_changed)

    def test_set_select_rect_mode_no_op_when_no_plotter(self):
        self._mock_view.main_plotter = None
        self._presenter.set_select_rect_mode()
        self._mock_view.overlay_rectangle.assert_not_called()

    def test_get_rect_selected_detector_ids_returns_copy(self):
        self._presenter._rect_selected_detector_ids = [1, 2, 3]
        result = self._presenter.get_rect_selected_detector_ids()
        self.assertEqual(result, [1, 2, 3])
        result.append(4)
        self.assertEqual(self._presenter._rect_selected_detector_ids, [1, 2, 3])

    def test_get_rect_selected_ids_empty_by_default(self):
        self.assertEqual(self._presenter.get_rect_selected_detector_ids(), [])

    def test_selected_detector_ids_returns_stored_ids(self):
        self._presenter._rect_selected_detector_ids = [10, 20]
        self.assertEqual(self._presenter.selected_detector_ids(), [10, 20])

    def test_on_rect_shape_changed_no_op_when_no_model(self):
        self._presenter._transform = np.eye(4)
        self._mock_view.shape_overlay_manager = MagicMock()
        self._presenter._on_rect_shape_changed()
        self.assertEqual(self._presenter._rect_selected_detector_ids, [])

    def test_on_rect_shape_changed_no_op_when_no_transform(self):
        self._presenter._model = MagicMock()
        self._mock_view.shape_overlay_manager = MagicMock()
        self._presenter._on_rect_shape_changed()
        self.assertEqual(self._presenter._rect_selected_detector_ids, [])

    def test_on_rect_shape_changed_no_op_when_no_manager(self):
        self._presenter._model = MagicMock()
        self._presenter._transform = np.eye(4)
        self._mock_view.shape_overlay_manager = None
        self._presenter._on_rect_shape_changed()
        self.assertEqual(self._presenter._rect_selected_detector_ids, [])

    def test_on_rect_shape_changed_empty_positions_gives_no_ids(self):
        mock_model = MagicMock()
        mock_model.detector_positions = np.zeros((0, 3))
        self._presenter._model = mock_model
        self._presenter._transform = np.eye(4)
        mock_manager = MagicMock()
        self._mock_view.shape_overlay_manager = mock_manager
        self._presenter._on_rect_shape_changed()
        self.assertEqual(self._presenter._rect_selected_detector_ids, [])

    def test_on_rect_shape_changed_selects_masked_detectors(self):
        """Detectors where get_shape_mask returns True should appear in the result."""
        positions = np.array([[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [2.0, 0.0, 0.0]])
        detector_ids = np.array([101, 102, 103])

        mock_model = MagicMock()
        mock_model.detector_positions = positions
        mock_model.all_detector_ids = detector_ids
        self._presenter._model = mock_model
        self._presenter._transform = np.eye(4)

        mock_manager = MagicMock()
        # Only the first and third detectors are inside the shape
        mock_manager.get_shape_mask.return_value = np.array([True, False, True])
        self._mock_view.shape_overlay_manager = mock_manager
        # No relay child expected
        self._mock_view.findChild.return_value = None

        self._presenter._on_rect_shape_changed()

        self.assertEqual(self._presenter._rect_selected_detector_ids, [101, 103])

    def test_on_rect_shape_changed_applies_transform(self):
        """The identity transform should leave positions unchanged."""
        positions = np.array([[1.0, 2.0, 0.0]])
        mock_model = MagicMock()
        mock_model.detector_positions = positions
        mock_model.all_detector_ids = np.array([55])
        self._presenter._model = mock_model
        self._presenter._transform = np.eye(4)

        mock_manager = MagicMock()
        mock_manager.get_shape_mask.return_value = np.array([True])
        self._mock_view.shape_overlay_manager = mock_manager
        self._mock_view.findChild.return_value = None

        self._presenter._on_rect_shape_changed()

        # Verify the coordinates passed to get_shape_mask are [position, 0] (no z component)
        called_coords = mock_manager.get_shape_mask.call_args[0][0]
        np.testing.assert_allclose(called_coords[0, :3], [1.0, 2.0, 0.0])
        self.assertEqual(self._presenter._rect_selected_detector_ids, [55])

    def test_render_registers_fill_transform_callback(self):
        """After _render, the view's resize callback is set to _apply_fill_transform."""
        self._presenter._model = MagicMock()
        self._presenter._renderer = MagicMock()
        self._presenter._renderer.build_detector_mesh.return_value = MagicMock(bounds=(0, 1, 0, 1, 0, 1))
        self._presenter._render()
        self._mock_view.set_on_resize_callback.assert_called_once_with(self._presenter._apply_fill_transform)

    def test_apply_fill_transform_scales_mesh_to_fill_viewport(self):
        """Fill transform should scale mesh so both dimensions fill the viewport."""
        mesh = MagicMock()
        mesh.bounds = (0, 2, 0, 1, 0, 0)  # mesh 2 wide, 1 tall
        self._presenter._detector_mesh = mesh
        self._presenter._original_mesh_bounds = (0, 2, 0, 1, 0, 0)
        self._presenter._transform = np.eye(4)
        self._mock_view.main_plotter.ren_win.GetSize.return_value = (400, 300)
        # parallel_scale is read directly from the camera (no reset_camera call)
        self._mock_view.main_plotter.camera.parallel_scale = 0.75

        self._presenter._apply_fill_transform()

        # reset_camera must NOT be called — calling it would render the unfilled mesh
        # and cause a visible flash.
        self._mock_view.main_plotter.reset_camera.assert_not_called()
        # Fill transform applied to mesh as a single combined operation
        mesh.transform.assert_called_once()
        transform = mesh.transform.call_args[0][0]
        # parallel_scale=0.75 → visible_height=1.5, visible_width=1.5*(400/300)=2.0
        # scale_x = 2.0/2 = 1.0, scale_y = 1.5/1 = 1.5
        np.testing.assert_allclose(transform[0, 0], 1.0, atol=1e-6)
        np.testing.assert_allclose(transform[1, 1], 1.5, atol=1e-6)

    def test_apply_fill_transform_renders_once_at_end(self):
        """A single explicit Render() must be called after the fill transform."""
        mesh = MagicMock()
        self._presenter._detector_mesh = mesh
        self._presenter._original_mesh_bounds = (0, 2, 0, 1, 0, 0)
        self._presenter._transform = np.eye(4)
        self._mock_view.main_plotter.ren_win.GetSize.return_value = (400, 300)
        self._mock_view.main_plotter.camera.parallel_scale = 0.75

        self._presenter._apply_fill_transform()

        self._mock_view.main_plotter.render_window.Render.assert_called_once()

    def test_apply_fill_transform_uses_combined_delta_for_second_call(self):
        """On subsequent calls the undo+apply must happen as one combined transform,
        not two separate calls that risk an intermediate render of the unfilled mesh."""
        mesh = MagicMock()
        self._presenter._detector_mesh = mesh
        self._presenter._original_mesh_bounds = (0, 2, 0, 1, 0, 0)
        # Simulate a previously-applied non-identity fill transform
        self._presenter._transform = np.diag([2.0, 3.0, 1.0, 1.0])
        self._mock_view.main_plotter.ren_win.GetSize.return_value = (400, 300)
        self._mock_view.main_plotter.camera.parallel_scale = 0.75

        self._presenter._apply_fill_transform()

        # Must be exactly one transform call (the combined delta), not two.
        self.assertEqual(mesh.transform.call_count, 1)

    def test_apply_fill_transform_updates_interactor_style_defaults(self):
        """_apply_fill_transform should call update_default_camera_state on the interactor style."""
        mesh = MagicMock()
        mesh.bounds = (0, 2, 0, 1, 0, 0)
        self._presenter._detector_mesh = mesh
        self._presenter._original_mesh_bounds = (0, 2, 0, 1, 0, 0)
        self._presenter._transform = np.eye(4)
        self._mock_view.main_plotter.ren_win.GetSize.return_value = (400, 300)
        self._mock_view.main_plotter.camera.parallel_scale = 0.75
        mock_style = MagicMock(spec=["update_default_camera_state"])
        self._mock_view.main_plotter.iren.style = mock_style

        self._presenter._apply_fill_transform()

        mock_style.update_default_camera_state.assert_called_once()

    def test_apply_fill_transform_skips_style_update_when_style_has_no_method(self):
        """_apply_fill_transform should not fail when the style lacks update_default_camera_state."""
        mesh = MagicMock()
        mesh.bounds = (0, 2, 0, 1, 0, 0)
        self._presenter._detector_mesh = mesh
        self._presenter._original_mesh_bounds = (0, 2, 0, 1, 0, 0)
        self._presenter._transform = np.eye(4)
        self._mock_view.main_plotter.ren_win.GetSize.return_value = (400, 300)
        self._mock_view.main_plotter.camera.parallel_scale = 0.75
        # Style with no update_default_camera_state attribute
        self._mock_view.main_plotter.iren.style = object()

        # Should not raise
        self._presenter._apply_fill_transform()

    def test_reset_clears_resize_callback(self):
        self._presenter._model = MagicMock()
        self._presenter.reset()
        self._mock_view.set_on_resize_callback.assert_called_with(None)

    def test_scale_matrix_identity_when_scale_1(self):
        matrix = ReflectometryInstrumentViewPresenter._scale_matrix_relative_to_centre(np.array([0.0, 0.0, 0.0]), scale_x=1.0, scale_y=1.0)
        np.testing.assert_allclose(matrix, np.eye(4))

    def test_scale_matrix_scales_correctly(self):
        centre = np.array([2.0, 3.0, 0.0])
        matrix = ReflectometryInstrumentViewPresenter._scale_matrix_relative_to_centre(centre, scale_x=2.0, scale_y=3.0)
        # A point at the centre should map to itself
        point = np.append(centre, 1.0)
        result = matrix @ point
        np.testing.assert_allclose(result[:3], centre)

    def test_scale_matrix_scales_origin_point(self):
        """Point at origin should be scaled relative to centre."""
        centre = np.array([1.0, 1.0, 0.0])
        matrix = ReflectometryInstrumentViewPresenter._scale_matrix_relative_to_centre(centre, scale_x=2.0, scale_y=2.0)
        origin = np.array([0.0, 0.0, 0.0, 1.0])
        result = matrix @ origin
        # Origin is 1 unit away from centre; after 2x scale it should be 2 units away
        np.testing.assert_allclose(result[:2], [-1.0, -1.0])

    def test_render_does_not_raise(self):
        """_render must complete without raising.

        The model mock is spec-restricted to only the attributes that
        FullInstrumentViewModel actually exposes (including ``flip_beam``).
        Accessing a non-existent attribute such as the formerly incorrect
        ``flip_z`` would therefore raise AttributeError, catching the
        regression introduced by that typo.
        """
        mock_model = mock.create_autospec(FullInstrumentViewModel, instance=True)
        mock_model.detector_positions = np.zeros((10, 3))
        mock_model.flip_beam = False
        mock_model.detector_counts = np.zeros(10)
        mock_model.is_2d_projection = True

        mock_renderer = MagicMock()
        mock_renderer.build_detector_mesh.return_value = MagicMock(bounds=(0, 1, 0, 1, 0, 1))

        self._presenter._model = mock_model
        self._presenter._renderer = mock_renderer

        # Should not raise
        self._presenter._render()


if __name__ == "__main__":
    unittest.main()
