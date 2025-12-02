# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.FullInstrumentViewPresenter import FullInstrumentViewPresenter
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.Peaks.DetectorPeaks import DetectorPeaks
from instrumentview.Peaks.Peak import Peak

import numpy as np
from mantid.simpleapi import CreateSampleWorkspace

import unittest
from unittest import mock
from unittest.mock import MagicMock


class TestFullInstrumentViewPresenter(unittest.TestCase):
    def setUp(self):
        self._mock_view = MagicMock()
        self._ws = CreateSampleWorkspace(OutputWorkspace="TestFullInstrumentViewPresenter", EnableLogging=False)
        self._model = FullInstrumentViewModel(self._ws)
        with mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.set_peaks_workspaces"):
            self._presenter = FullInstrumentViewPresenter(self._mock_view, self._model)
        self._mock_view.reset_mock()

    def tearDown(self):
        self._presenter.handle_close()
        self._ws.delete()

    def _create_detector_peaks(self, det_id: int, location: np.ndarray) -> DetectorPeaks:
        return DetectorPeaks([Peak(det_id, location, (1, 1, 1), 100, 1000, 100, 100)])

    def test_projection_combo_options(self):
        _, projections = self._presenter.projection_combo_options()
        self.assertGreater(len(projections), 0)
        self.assertTrue("Spherical X" in projections)

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.set_peaks_workspaces")
    def test_projection_option_selected(self, mock_set_peaks_ws):
        self._presenter.on_projection_option_selected(1)
        self._mock_view.add_main_mesh.assert_called()
        mock_set_peaks_ws.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.reset_cached_projection_positions")
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.set_peaks_workspaces")
    def test_3d_projection_resets_cache(self, mock_set_peaks_ws, mock_reset_cache):
        self.assertEquals("3D", self._model._PROJECTION_OPTIONS[0])
        self._presenter.on_projection_option_selected(0)
        mock_reset_cache.assert_called_once()
        self._mock_view.add_main_mesh.assert_called()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.create_poly_data_mesh")
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.calculate_projection")
    def test_projection_option_axis(self, mock_calculate_projection, mock_create_poly_data_mesh):
        _, options = self._presenter.projection_combo_options()
        for option in options:
            if option.endswith("X"):
                axis = [1, 0, 0]
            elif option.endswith("Y"):
                axis = [0, 1, 0]
            elif option.endswith("Z"):
                axis = [0, 0, 1]
            else:
                return
            self._presenter.on_projection_option_selected(options.index(option))
            mock_calculate_projection.assert_called_once_with(option.startswith("Spherical"), axis)
            mock_create_poly_data_mesh.assert_called()
            mock_calculate_projection.reset_mock()
            mock_create_poly_data_mesh.reset_mock()

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

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.update_picked_detectors")
    def test_point_picked(self, mock_update_picked_detectors):
        mock_picker = MagicMock()

        def get_point_id():
            return 1

        mock_picker.GetPointId = get_point_id
        self._presenter.point_picked(MagicMock(), mock_picker)
        mock_update_picked_detectors.assert_called_with([1])

    def test_update_picked_detectors(self):
        self._model._workspace_indices = np.array([0, 1, 2])
        self._model._is_valid = np.array([True, True, True])
        self._model._detector_is_picked = np.array([True, True, False])
        self._model._detector_ids = np.array([1, 2, 3])
        self._model.picked_detectors_info_text = MagicMock(return_value=["a", "a"])
        self._model.extract_spectra_for_line_plot = MagicMock()
        self._presenter._pickable_main_mesh = {}
        self._presenter._pickable_projection_mesh = {}
        self._mock_view.current_selected_unit.return_value = "TOF"
        self._mock_view.sum_spectra_selected.return_value = True
        self._presenter.update_picked_detectors([])
        np.testing.assert_allclose(self._presenter._pickable_main_mesh[self._presenter._visible_label], self._model._detector_is_picked)
        self._mock_view.show_plot_for_detectors.assert_called_once_with(self._model.line_plot_workspace)
        self._mock_view.set_selected_detector_info.assert_called_once_with(["a", "a"])
        self._model.extract_spectra_for_line_plot.assert_called_once_with("TOF", True)

    def test_generate_single_colour(self):
        green_vector = self._presenter.generate_single_colour(2, 0, 1, 0, 0)
        self.assertEqual(len(green_vector), 2)
        self.assertTrue(green_vector.all(where=[0, 1, 0, 0]))

    def test_set_multi_select_enabled(self):
        self._mock_view.is_multi_picking_checkbox_checked.return_value = True
        self._presenter.on_multi_select_detectors_clicked()
        self._mock_view.enable_rectangle_picking.assert_called_once()
        self._mock_view.enable_point_picking.assert_not_called()

    def test_set_multi_select_disabled(self):
        self._mock_view.is_multi_picking_checkbox_checked.return_value = False
        self._presenter.on_multi_select_detectors_clicked()
        self._mock_view.enable_rectangle_picking.assert_not_called()
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
        self.assertEquals(["No units"], units)

    @mock.patch.object(FullInstrumentViewModel, "has_unit", new_callable=mock.PropertyMock)
    def test_available_units_has_units(self, mock_has_unit):
        mock_has_unit.return_value = True
        units = self._presenter.available_unit_options()
        mock_has_unit.assert_called_once()
        self.assertEquals(self._presenter._UNIT_OPTIONS, units)

    def test_only_close_on_correct_ws_replace(self):
        ws_name = self._model.workspace.name()
        self._presenter.replace_workspace_callback(ws_name, None)
        self._mock_view.close.assert_called_once()
        self._mock_view.close.reset_mock()
        self._presenter.replace_workspace_callback("not_my_workspace", None)
        self._mock_view.close.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.on_peaks_workspace_selected")
    def test_reload_peaks_workspaces(self, mock_on_peaks_workspace_selected):
        self._presenter._reload_peaks_workspaces()
        self._mock_view.refresh_peaks_ws_list.assert_called_once()
        mock_on_peaks_workspace_selected.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.peaks_workspaces_in_ads")
    def test_peaks_workspaces_in_ads(self, mock_peaks_workspaces_in_ads):
        mock_peaks_workspaces_in_ads.return_value = [self._ws, self._ws]
        workspaces = self._presenter.peaks_workspaces_in_ads()
        mock_peaks_workspaces_in_ads.assert_called_once()
        self.assertEqual([self._ws.name(), self._ws.name()], workspaces)

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._transform_vectors_with_matrix")
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.set_peaks_workspaces")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.refresh_lineplot_peaks")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._adjust_points_for_selected_projection")
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.peak_overlay_points")
    def test_on_peaks_workspace_selected(
        self,
        mock_peak_overlay_points,
        mock_adjust_points_projection,
        mock_refresh_lineplot_peaks,
        mock_set_peaks_workspaces,
        mock_transform,
    ):
        mock_peak_overlay_points.return_value = [[self._create_detector_peaks(50, np.zeros(3))]]
        mock_adjust_points_projection.return_value = [mock_peak_overlay_points()[0][0].location]
        self._model._current_projected_positions = np.array([np.zeros(3)])
        self._model._detector_ids = np.array([50, 52])
        self._model._is_valid = np.array([True, True])
        self._presenter.on_peaks_workspace_selected()
        mock_refresh_lineplot_peaks.assert_called_once()
        self._mock_view.clear_overlay_meshes.assert_called_once()
        mock_set_peaks_workspaces.assert_called_once()
        self._mock_view.plot_overlay_mesh.assert_called_once()
        mock_transform.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.peak_overlay_points")
    @mock.patch.object(FullInstrumentViewModel, "picked_detector_ids", new_callable=mock.PropertyMock)
    def test_refresh_lineplot_peaks(self, mock_picked_detector_ids, mock_peak_overlay_points):
        mock_peak_overlay_points.return_value = [[self._create_detector_peaks(50, np.zeros(3))]]
        mock_picked_detector_ids.return_value = [50]
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
    @mock.patch.object(FullInstrumentViewModel, "picked_detector_ids", new_callable=mock.PropertyMock)
    def test_refresh_lineplot_peaks_q(self, mock_picked_detector_ids, mock_peak_overlay_points):
        mock_peak_overlay_points.return_value = [[self._create_detector_peaks(50, np.zeros(3))]]
        mock_picked_detector_ids.return_value = [50]
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
    @mock.patch.object(FullInstrumentViewModel, "picked_detector_ids", new_callable=mock.PropertyMock)
    def test_refresh_lineplot_peaks_no_detector(self, mock_picked_detector_ids, mock_peak_overlay_points):
        mock_peak_overlay_points.return_value = [[self._create_detector_peaks(50, np.zeros(3))]]
        mock_picked_detector_ids.return_value = []
        self._mock_view.current_selected_unit.return_value = self._presenter._TIME_OF_FLIGHT
        self._presenter._update_peaks_workspaces()
        self._presenter.refresh_lineplot_peaks()
        mock_peak_overlay_points.assert_called_once()
        self._mock_view.clear_lineplot_overlays.assert_called_once()
        self._mock_view.redraw_lineplot.assert_called_once()
        self._mock_view.plot_lineplot_overlay.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.peak_overlay_points")
    @mock.patch.object(FullInstrumentViewModel, "picked_detector_ids", new_callable=mock.PropertyMock)
    def test_refresh_lineplot_peaks_wrong_unit(self, mock_picked_detector_ids, mock_peak_overlay_points):
        mock_peak_overlay_points.return_value = [[self._create_detector_peaks(50, np.zeros(3))]]
        mock_picked_detector_ids.return_value = [50]
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

    @mock.patch.object(FullInstrumentViewModel, "default_projection", new_callable=mock.PropertyMock)
    def test_default_projection(self, mock_default_projection):
        mock_default_projection.return_value = "SPHERICAL_Z"
        default_index, _ = self._presenter.projection_combo_options()
        self.assertEqual(3, default_index)

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.reset_cached_projection_positions")
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.set_peaks_workspaces")
    def test_aspect_ratio_box_visibility_set(self, mock_set_peaks, mock_reset_cache):
        self.assertEquals("3D", self._model._PROJECTION_OPTIONS[0])
        self._presenter.on_projection_option_selected(0)
        self._mock_view.set_aspect_ratio_box_visibility.assert_called_once_with(False)
        self._mock_view.set_aspect_ratio_box_visibility.reset_mock()
        self._presenter.on_projection_option_selected(1)
        self._mock_view.set_aspect_ratio_box_visibility.assert_called_once_with(True)

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.reset_cached_projection_positions")
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.set_peaks_workspaces")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._update_transform")
    def test_transform_updated_on_redraw(self, mock_update_transform, mock_set_peaks, mock_reset_cache):
        self.assertEquals("3D", self._model._PROJECTION_OPTIONS[0])
        # Anything apart from 3D
        self._presenter.on_projection_option_selected(2)
        mock_update_transform.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._transform_mesh_to_fill_window")
    def test_update_transform(self, mock_transform_mesh):
        self._presenter._update_transform(False, MagicMock())
        np.testing.assert_allclose(np.eye(4), self._presenter._transform, atol=1e-10)
        mock_transform_mesh.assert_not_called()
        self._mock_view.is_maintain_aspect_ratio_checkbox_checked.return_value = True
        self._presenter._update_transform(False, MagicMock())
        np.testing.assert_allclose(np.eye(4), self._presenter._transform, atol=1e-10)
        mock_transform_mesh.assert_not_called()
        mock_mesh = MagicMock()
        self._mock_view.is_maintain_aspect_ratio_checkbox_checked.return_value = False
        self._presenter._update_transform(True, mock_mesh)
        mock_transform_mesh.assert_called_once_with(mock_mesh)

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._scale_matrix_relative_to_centre")
    def test_transform_mesh_to_fill_window(self, mock_scale_matrix):
        mock_mesh = MagicMock()
        mock_mesh.bounds = [-1, 1, -1, 1, -1, 1]
        mock_mesh.center = np.zeros(3)
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
            self._presenter._transform_mesh_to_fill_window(mock_mesh)
            mock_vtk.assert_called_once()
            self.assertEquals(2, mock_vtk_instance.GetComputedDisplayValueCount)

        # The mesh width and height in pixels are 8 each, the window is width 10,
        # hence the scale factor should be 10 / 8 = 1.25
        mock_scale_matrix.assert_called_once_with(mock_mesh.center, 1.25, 1.25)

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


if __name__ == "__main__":
    unittest.main()
