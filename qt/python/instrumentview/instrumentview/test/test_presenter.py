# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.FullInstrumentViewPresenter import FullInstrumentViewPresenter, PeakInteractionStatus
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.Globals import CurrentTab
from instrumentview.Peaks.DetectorPeaks import DetectorPeaks
from instrumentview.Peaks.Peak import Peak
from instrumentview.Projections.ProjectionType import ProjectionType

import numpy as np
from mantid.simpleapi import CreateSampleWorkspace

import unittest
from unittest import mock
from unittest.mock import MagicMock
from enum import Enum


class TestFullInstrumentViewPresenter(unittest.TestCase):
    def setUp(self):
        self._mock_view = MagicMock()
        self._mock_view.current_selected_projection.return_value = ProjectionType.CYLINDRICAL_Y
        self._ws = CreateSampleWorkspace(OutputWorkspace="TestFullInstrumentViewPresenter", EnableLogging=False)
        self._model = FullInstrumentViewModel(self._ws)
        with mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.set_peaks_workspaces"):
            self._presenter = FullInstrumentViewPresenter(self._mock_view, self._model)
        self._mock_view.reset_mock()

    def tearDown(self):
        self._presenter.handle_close()
        self._ws.delete()

    def _create_detector_peaks(self, det_id: int, spec_no: int, location: np.ndarray) -> DetectorPeaks:
        return DetectorPeaks([Peak(det_id, spec_no, location, 0, (1, 1, 1), 100, 1000, 100, 100)])

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.set_peaks_workspaces")
    def test_update_plotter(self, mock_set_peaks_ws):
        self._mock_view.current_selected_projection.return_value = ProjectionType.CYLINDRICAL_X
        self._presenter.update_plotter()
        self.assertEqual(self._model.projection_type, ProjectionType.CYLINDRICAL_X)
        self._mock_view.add_detector_mesh.assert_called()
        self._mock_view.add_pickable_mesh.assert_called()
        self._mock_view.add_masked_mesh.assert_called()
        mock_set_peaks_ws.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.update_integration_range")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.set_view_integration_limits")
    def test_on_integration_limits_updated_true(self, mock_set_view_integration_limits, mock_update_integration_range):
        self._mock_view.get_integration_limits.return_value = (0, 100)
        self._presenter.on_integration_limits_updated()
        mock_update_integration_range.assert_called_with((0, 100))
        mock_set_view_integration_limits.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.set_view_contour_limits")
    def test_on_contour_limits_updated_true(self, mock_set_view_contour_limits):
        self._mock_view.get_contour_limits.return_value = (0, 100)
        self._presenter.on_contour_limits_updated()
        self.assertEqual(self._model._counts_limits, (0, 100))
        mock_set_view_contour_limits.assert_called_once()

    def test_set_view_contour_limits(self):
        self._model._counts_limits = (0, 100)
        self._presenter.set_view_contour_limits()
        self._mock_view.set_plotter_scalar_bar_range.assert_called_once_with((0, 100), self._presenter._counts_label)

    def test_set_view_integration_limits(self):
        self._model._counts = np.zeros(200)
        self._presenter._detector_mesh = {}
        self._presenter.set_view_integration_limits()
        np.testing.assert_allclose(self._presenter._detector_mesh[self._presenter._counts_label], self._model.detector_counts)

    def test_generate_single_colour(self):
        green_vector = self._presenter.generate_single_colour(2, 0, 1, 0, 0)
        self.assertEqual(len(green_vector), 2)
        self.assertTrue(green_vector.all(where=[0, 1, 0, 0]))

    def test_update_detector_picker(self):
        self._mock_view.is_multi_picking_checkbox_checked.return_value = False
        self._presenter.update_detector_picker()
        self._mock_view.enable_point_picking.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.extract_spectra_for_line_plot")
    def test_unit_option_selected(self, mock_extract_spectra):
        self._mock_view.sum_spectra_selected.return_value = True
        self._presenter.on_unit_option_selected(1)
        self._mock_view.show_plot_for_detectors.assert_called_once()
        self._mock_view.set_selected_detector_info.assert_called_once()
        mock_extract_spectra.assert_called_once_with(self._presenter._UNIT_OPTIONS[1], True)

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.save_line_plot_workspace_to_ads")
    def test_export_workspace_clicked(self, mock_save_line_plot_workspace_to_ads):
        self._presenter.on_export_workspace_clicked()
        mock_save_line_plot_workspace_to_ads.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._update_line_plot_ws_and_draw")
    def test_on_sum_spectra_checkbox_clicked(self, mock_update_line_plot_ws_and_draw):
        self._mock_view.current_selected_unit.return_value = "dSpacing"
        self._presenter.on_sum_spectra_checkbox_clicked()
        mock_update_line_plot_ws_and_draw.assert_called_once_with("dSpacing")

    @mock.patch.object(FullInstrumentViewModel, "has_unit", new_callable=mock.PropertyMock)
    def test_available_units_no_units(self, mock_has_unit):
        mock_has_unit.return_value = False
        units = self._presenter.available_unit_options()
        mock_has_unit.assert_called_once()
        self.assertEqual(["No units"], units)

    @mock.patch.object(FullInstrumentViewModel, "has_unit", new_callable=mock.PropertyMock)
    def test_available_units_has_units(self, mock_has_unit):
        mock_has_unit.return_value = True
        units = self._presenter.available_unit_options()
        mock_has_unit.assert_called_once()
        self.assertEqual(self._presenter._UNIT_OPTIONS, units)

    @mock.patch("instrumentview.FullInstrumentViewPresenter.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.update_plotter")
    def test_model_refresh_on_correct_ws_replace(self, mock_update_plotter, mock_ads):
        mock_ads_retrieve = MagicMock()
        mock_ads.retrieve.return_value = mock_ads_retrieve
        self._model.setup = MagicMock()
        ws_name = self._model.workspace.name()
        mock_ads_retrieve.name.return_value = ws_name
        self._presenter._replace_workspace_callback(ws_name, None)
        self._model.setup.assert_called_once()
        mock_update_plotter.assert_called_once()
        self._mock_view.setup.reset_mock()
        mock_update_plotter.reset_mock()
        self._presenter._replace_workspace_callback("not_my_workspace", None)
        self._mock_view.setup.assert_not_called()
        mock_update_plotter.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._update_relative_detector_angle")
    def test_update_picked_detectors(self, mock_update_det_angle):
        self._model._workspace_indices = np.array([0, 1, 2])
        self._model._is_valid = np.array([True, True, True])
        self._model._is_masked = np.array([False, False, False])
        self._model._is_selected_in_tree = np.array([True, True, True])
        self._model._detector_is_picked = np.array([True, True, False])
        self._model._detector_ids = np.array([1, 2, 3])
        self._model.picked_detectors_info_text = MagicMock(return_value=["a", "a"])
        self._model.extract_spectra_for_line_plot = MagicMock()
        self._presenter._pickable_mesh = {}
        self._presenter._pickable_projection_mesh = {}
        self._mock_view.current_selected_unit.return_value = "TOF"
        self._mock_view.sum_spectra_selected.return_value = True
        self._presenter.update_picked_detectors_on_view()
        np.testing.assert_allclose(self._presenter._pickable_mesh[self._presenter._visible_label], self._model._detector_is_picked)
        self._mock_view.show_plot_for_detectors.assert_called_once_with(self._model.line_plot_workspace)
        self._mock_view.set_selected_detector_info.assert_called_once_with(["a", "a"])
        self._model.extract_spectra_for_line_plot.assert_called_once_with("TOF", True)

    def test_on_add_selection_clicked(self):
        mock_implicit_return = np.linspace(-1, 1, self._ws.getNumberHistograms())
        mock_implicit_function = MagicMock(EvaluateFunction=MagicMock(side_effect=mock_implicit_return))
        self._mock_view.get_current_widget_implicit_function.return_value = mock_implicit_function
        self._mock_view.get_current_selected_tab.return_value = CurrentTab.Grouping
        self._model.add_new_detector_key = MagicMock(return_value="mock_key")
        self._presenter._on_add_item_clicked()
        np.testing.assert_allclose(self._model.add_new_detector_key.call_args.args[0], mock_implicit_return < 0)
        self._mock_view.set_new_item_key.assert_called_once_with(CurrentTab.Grouping, "mock_key")

    def test_on_save_mask_to_workspace_clicked(self):
        self._mock_view.get_current_tab.return_value = CurrentTab.Masking
        self._model.save_workspace_to_ads = MagicMock()
        self._presenter._on_save_to_workspace_clicked()
        self._model.save_workspace_to_ads.assert_called_once()
        self._mock_view.on_mask_item_selected.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.on_peaks_workspace_selected")
    def test_reload_peaks_workspaces(self, mock_on_peaks_workspace_selected):
        self._presenter._reload_peaks_workspaces()
        self._mock_view.refresh_peaks_ws_list.assert_called_once()
        mock_on_peaks_workspace_selected.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.get_workspaces_in_ads_of_type")
    def test_peaks_workspaces_in_ads(self, mock_peaks_workspaces_in_ads):
        mock_peaks_workspaces_in_ads.return_value = [self._ws, self._ws]
        workspaces = self._presenter.peaks_workspaces_in_ads()
        mock_peaks_workspaces_in_ads.assert_called_once()
        self.assertEqual([self._ws.name(), self._ws.name()], workspaces)

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._transform_vectors_with_matrix")
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.set_peaks_workspaces")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.refresh_lineplot_peaks")
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.peak_overlay_points")
    def test_on_peaks_workspace_selected(
        self,
        mock_peak_overlay_points,
        mock_refresh_lineplot_peaks,
        mock_set_peaks_workspaces,
        mock_transform,
    ):
        mock_peak_overlay_points.return_value = {"ws1": [self._create_detector_peaks(50, 50, np.zeros(3))]}
        self._model._calculate_projection = MagicMock(return_value=np.array([np.zeros(3), np.zeros(3)]))
        self._model._detector_ids = np.array([50, 52])
        self._model._spectrum_nos = np.array([50, 52])
        self._model._is_valid = np.array([True, True])
        self._model._is_masked = np.array([False, False])
        self._model._is_selected_in_tree = np.array([True, True])
        self._presenter.on_peaks_workspace_selected()
        mock_refresh_lineplot_peaks.assert_called_once()
        self._mock_view.clear_overlay_meshes.assert_called_once()
        mock_set_peaks_workspaces.assert_called_once()
        self._mock_view.plot_overlay_mesh.assert_called_once()
        mock_transform.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.peak_overlay_points")
    @mock.patch.object(FullInstrumentViewModel, "picked_spectrum_nos", new_callable=mock.PropertyMock)
    def test_refresh_lineplot_peaks(self, mock_picked_spectrum_nos, mock_peak_overlay_points):
        mock_peak_overlay_points.return_value = {"ws1": [self._create_detector_peaks(50, 50, np.zeros(3))]}
        mock_picked_spectrum_nos.return_value = [50]
        self._mock_view.current_selected_unit.return_value = self._presenter._TIME_OF_FLIGHT
        self._presenter._update_peaks_workspaces()
        self._presenter.refresh_lineplot_peaks()
        mock_peak_overlay_points.assert_called_once()
        self._mock_view.clear_lineplot_overlays.assert_called_once()
        self._mock_view.redraw_lineplot.assert_called_once()
        self._mock_view.plot_lineplot_overlay.assert_called_once()
        overlay_call_args = self._mock_view.plot_lineplot_overlay.call_args[0]
        self.assertEqual([100], overlay_call_args[0])
        self.assertEqual(["(1, 1, 1)"], overlay_call_args[1])

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.peak_overlay_points")
    @mock.patch.object(FullInstrumentViewModel, "picked_spectrum_nos", new_callable=mock.PropertyMock)
    def test_refresh_lineplot_peaks_q(self, mock_picked_spectrum_nos, mock_peak_overlay_points):
        mock_peak_overlay_points.return_value = {"ws1": [self._create_detector_peaks(50, 50, np.zeros(3))]}
        mock_picked_spectrum_nos.return_value = [50]
        self._mock_view.current_selected_unit.return_value = self._presenter._MOMENTUM_TRANSFER
        self._presenter._update_peaks_workspaces()
        self._presenter.refresh_lineplot_peaks()
        mock_peak_overlay_points.assert_called_once()
        self._mock_view.clear_lineplot_overlays.assert_called_once()
        self._mock_view.redraw_lineplot.assert_called_once()
        self._mock_view.plot_lineplot_overlay.assert_called_once()
        overlay_call_args = self._mock_view.plot_lineplot_overlay.call_args[0]
        self.assertEqual([100], overlay_call_args[0])
        self.assertEqual(["(1, 1, 1)"], overlay_call_args[1])

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.peak_overlay_points")
    @mock.patch.object(FullInstrumentViewModel, "picked_spectrum_nos", new_callable=mock.PropertyMock)
    def test_refresh_lineplot_peaks_no_detector(self, mock_picked_spectrum_nos, mock_peak_overlay_points):
        mock_peak_overlay_points.return_value = {"ws1": [self._create_detector_peaks(50, 50, np.zeros(3))]}
        mock_picked_spectrum_nos.return_value = []
        self._mock_view.current_selected_unit.return_value = self._presenter._TIME_OF_FLIGHT
        self._presenter._update_peaks_workspaces()
        self._presenter.refresh_lineplot_peaks()
        mock_peak_overlay_points.assert_called_once()
        self._mock_view.clear_lineplot_overlays.assert_called_once()
        self._mock_view.redraw_lineplot.assert_called_once()
        self._mock_view.plot_lineplot_overlay.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.peak_overlay_points")
    @mock.patch.object(FullInstrumentViewModel, "picked_spectrum_nos", new_callable=mock.PropertyMock)
    def test_refresh_lineplot_peaks_wrong_unit(self, mock_picked_spectrum_nos, mock_peak_overlay_points):
        mock_peak_overlay_points.return_value = {"ws1": [self._create_detector_peaks(50, 50, np.zeros(3))]}
        mock_picked_spectrum_nos.return_value = [50]
        self._mock_view.current_selected_unit.return_value = "Light Years"
        self._presenter.refresh_lineplot_peaks()
        mock_peak_overlay_points.assert_not_called()
        self._mock_view.clear_lineplot_overlays.assert_called_once()
        self._mock_view.redraw_lineplot.assert_called_once()
        self._mock_view.plot_lineplot_overlay.assert_not_called()

    @mock.patch.object(FullInstrumentViewModel, "picked_detector_ids", new_callable=mock.PropertyMock)
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.relative_detector_angle")
    def test_update_relative_detector_angle_no_dectors(self, mock_relative_detector_angle, mock_picked_detector_ids):
        mock_picked_detector_ids.return_value = []
        self._presenter._update_relative_detector_angle()
        mock_relative_detector_angle.assert_not_called()
        self._mock_view.set_relative_detector_angle.assert_called_once()

    @mock.patch.object(FullInstrumentViewModel, "picked_detector_ids", new_callable=mock.PropertyMock)
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.relative_detector_angle")
    def test_update_relative_detector_angle_two_dectors(self, mock_relative_detector_angle, mock_picked_detector_ids):
        mock_picked_detector_ids.return_value = [11, 12]
        self._presenter._update_relative_detector_angle()
        mock_relative_detector_angle.assert_called_once()
        self._mock_view.set_relative_detector_angle.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.set_peaks_workspaces")
    def test_aspect_ratio_box_visibility_set(self, mock_set_peaks):
        self._model.projection_type = ProjectionType.THREE_D
        self._presenter.update_plotter()
        self._mock_view.enable_or_disable_aspect_ratio_box.assert_called_once()
        self._mock_view.enable_or_disable_aspect_ratio_box.reset_mock()
        self._model.projection_type = ProjectionType.SPHERICAL_X
        self._presenter.update_plotter()
        self._mock_view.enable_or_disable_aspect_ratio_box.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.set_peaks_workspaces")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._update_transform")
    def test_transform_updated_on_redraw(self, mock_update_transform, mock_set_peaks):
        # Anything apart from 3D
        self._model.projection_type = ProjectionType.SPHERICAL_X
        self._presenter.update_plotter()
        mock_update_transform.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._transform_mesh_to_fill_window")
    def test_update_transform(self, mock_transform_mesh):
        self._presenter._update_transform()
        np.testing.assert_allclose(np.eye(4), self._presenter._transform, atol=1e-10)
        mock_transform_mesh.assert_not_called()
        self._mock_view.is_maintain_aspect_ratio_checkbox_checked.return_value = True
        self._presenter._update_transform()
        np.testing.assert_allclose(np.eye(4), self._presenter._transform, atol=1e-10)
        mock_transform_mesh.assert_not_called()
        self._mock_view.is_maintain_aspect_ratio_checkbox_checked.return_value = False
        self._presenter._update_transform()
        mock_transform_mesh.assert_called()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._scale_matrix_relative_to_centre")
    def test_transform_mesh_to_fill_window(self, mock_scale_matrix):
        mock_det_mesh = MagicMock(bounds=[-1, 1, -1, 0, -1, 1])
        mock_mask_mesh = MagicMock(bounds=[0, 1, -1, 1, -1, 1])

        self._mock_view.main_plotter.window_size = (10, 10)

        class mock_vtkCoordinate(MagicMock):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)
                self.GetComputedDisplayValueCount = 0

            def GetComputedDisplayValue(self, _):
                self.GetComputedDisplayValueCount += 1
                if self.GetComputedDisplayValueCount % 2 == 1:
                    return [-4, -4, -4]
                return [4, 4, 4]

        with mock.patch("instrumentview.FullInstrumentViewPresenter.vtkCoordinate") as mock_vtk:
            mock_vtk_instance = mock_vtkCoordinate()
            mock_vtk.return_value = mock_vtk_instance
            self._presenter._detector_mesh = mock_det_mesh
            self._presenter._masked_mesh = mock_mask_mesh
            self._presenter._transform_mesh_to_fill_window()
            mock_vtk.assert_called_once()
            self.assertEquals(2, mock_vtk_instance.GetComputedDisplayValueCount)

        # The mesh width and height in pixels are 8 each, the window is width 10,
        # hence the scale factor should be 10 / 8 = 1.25
        args, _ = mock_scale_matrix.call_args_list[0]
        np.testing.assert_array_equal(args[0], np.zeros(3))
        self.assertEqual(args[1], 1.25)
        self.assertEqual(args[2], 1.25)

    def test_scale_matrix_relative_to_origin(self):
        centre = np.zeros(3)
        expected_transform = np.eye(4)
        expected_transform[0][0] = 2
        expected_transform[1][1] = 3
        transform = self._presenter._scale_matrix_relative_to_centre(centre, 2, 3)
        np.testing.assert_allclose(expected_transform, transform, atol=1e-10)

    def test_scale_matrix_relative_to_point(self):
        centre = np.array([3, 4, 0], dtype=np.float64)
        translation = np.eye(4)
        translation[0][3] = -centre[0]
        translation[1][3] = -centre[1]
        translation[2][3] = -centre[2]
        scale = np.eye(4)
        scale[0][0] = 2
        scale[1][1] = 3
        inverse_translation = np.eye(4)
        inverse_translation[0][3] = centre[0]
        inverse_translation[1][3] = centre[1]
        inverse_translation[2][3] = centre[2]
        expected_transform = inverse_translation @ scale @ translation
        transform = self._presenter._scale_matrix_relative_to_centre(centre, 2, 3)
        np.testing.assert_allclose(expected_transform, transform, atol=1e-10)

    def test_transform_vectors_with_matrix(self):
        vectors = np.array([[1, 0, 0], [0, 1, 0]])
        scale_matrix = np.eye(4)
        scale_matrix[0][0] = 3
        scale_matrix[1][1] = 10
        transformed_vectors = self._presenter._transform_vectors_with_matrix(vectors, scale_matrix)
        expected_vectors = np.array([[3, 0, 0], [0, 10, 0]])
        np.testing.assert_allclose(expected_vectors, transformed_vectors)

    def _setup_on_peak_selected_tests(self):
        self._presenter._model = MagicMock()
        self._presenter._model.workspace_x_unit = "workspace-unit"
        self._presenter._model.picked_spectrum_nos = [42]
        self._presenter._model.picked_detector_ids = [42]
        self._presenter._view.current_selected_unit.return_value = "view-unit"
        self._presenter._view.selected_peaks_workspaces.return_value = ["wsA", "wsB"]
        self._presenter._model.convert_units.return_value = 123.456
        self._presenter._update_peak_buttons = MagicMock()
        self._presenter._view.remove_peak_cursor_from_lineplot = MagicMock()

    def test_add_mode_calls_add_peak_and_select_workspace_and_resets_state(self):
        """In Adding mode: convert -> add_peak -> select_peaks_workspace, then disable & cleanup."""
        self._setup_on_peak_selected_tests()
        self._presenter._peak_interaction_status = PeakInteractionStatus.Adding
        returned_ws = MagicMock(name="ReturnedWorkspace")
        self._presenter._model.add_peak.return_value = returned_ws
        self._presenter.on_peak_selected(3.14)
        self._presenter._model.convert_units.assert_called_once_with("view-unit", "workspace-unit", 0, 3.14)
        self._presenter._model.add_peak.assert_called_once_with(123.456, ["wsA", "wsB"])
        self._presenter._view.select_peaks_workspace.assert_called_once_with(returned_ws)
        self.assertEqual(self._presenter._peak_interaction_status, PeakInteractionStatus.Disabled)
        self._presenter._view.remove_peak_cursor_from_lineplot.assert_called_once()
        self._presenter._update_peak_buttons.assert_called_once()
        self._presenter._model.delete_peak.assert_not_called()

    def test_delete_mode_calls_delete_peak_and_resets_state(self):
        """In Deleting mode: convert -> delete_peak, then disable & cleanup."""
        self._setup_on_peak_selected_tests()
        self._presenter._peak_interaction_status = PeakInteractionStatus.Deleting
        self._presenter.on_peak_selected(9.81)
        self._presenter._model.convert_units.assert_called_once_with("view-unit", "workspace-unit", 0, 9.81)
        self._presenter._model.delete_peak.assert_called_once_with(123.456)
        self._presenter._model.add_peak.assert_not_called()
        self._presenter._view.select_peaks_workspace.assert_not_called()
        self.assertEqual(self._presenter._peak_interaction_status, PeakInteractionStatus.Disabled)
        self._presenter._view.remove_peak_cursor_from_lineplot.assert_called_once()
        self._presenter._update_peak_buttons.assert_called_once()

    def test_unknown_status_raises_and_does_not_change_state_or_cleanup(self):
        """Unknown status -> raises RuntimeError, no cleanup or state changes."""

        # Use a fake/unknown status (not Adding/Deleting)
        class FakeStatus(Enum):
            Unknown = 99

        self._setup_on_peak_selected_tests()
        self._presenter._peak_interaction_status = FakeStatus.Unknown
        with self.assertRaises(RuntimeError):
            self._presenter.on_peak_selected(1.23)
        self._presenter._model.convert_units.assert_called_once_with("view-unit", "workspace-unit", 0, 1.23)
        self._presenter._model.add_peak.assert_not_called()
        self._presenter._model.delete_peak.assert_not_called()
        self._presenter._view.select_peaks_workspace.assert_not_called()
        self._presenter._view.remove_peak_cursor_from_lineplot.assert_not_called()
        self._presenter._update_peak_buttons.assert_not_called()
        self.assertEqual(self._presenter._peak_interaction_status, FakeStatus.Unknown)

    def test_add_mode_uses_selected_peaks_workspaces_from_view(self):
        """Ensures the workspace selection passed to add_peak is sourced from the view."""
        self._setup_on_peak_selected_tests()
        self._presenter._peak_interaction_status = PeakInteractionStatus.Adding
        self._presenter._view.selected_peaks_workspaces.return_value = ["ws1"]
        returned_ws = MagicMock(name="WS")
        self._presenter._model.add_peak.return_value = returned_ws
        self._presenter.on_peak_selected(2.0)
        self._presenter._model.add_peak.assert_called_once_with(123.456, ["ws1"])
        self._presenter._view.select_peaks_workspace.assert_called_once_with(returned_ws)

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._create_and_add_monitor_mesh")
    def test_monitor_mesh_added(self, mock_create_monitor_mesh):
        mock_create_monitor_mesh.return_value = None
        self._presenter._update_view_main_plotter()
        mock_create_monitor_mesh.assert_called_once()
        mock_create_monitor_mesh.reset_mock()

        mock_mesh = MagicMock()
        mock_create_monitor_mesh.return_value = mock_mesh
        self._presenter._update_view_main_plotter()
        mock_create_monitor_mesh.assert_called_once()
        mock_mesh.transform.assert_called_once()

    def test_create_and_add_monitor_mesh(self):
        self._mock_view.is_show_monitors_checkbox_checked.return_value = False
        mesh = self._presenter._create_and_add_monitor_mesh()
        self.assertIsNone(mesh)

        self._mock_view.is_show_monitors_checkbox_checked.return_value = True
        self._model._monitor_positions = [np.zeros(3)]
        self._presenter._create_and_add_monitor_mesh()
        self._mock_view.add_rgba_mesh.assert_called_once()


if __name__ == "__main__":
    unittest.main()
