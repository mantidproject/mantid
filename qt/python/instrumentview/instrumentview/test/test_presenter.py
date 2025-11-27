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
from instrumentview.Projections.ProjectionType import ProjectionType

import numpy as np
from mantid.simpleapi import CreateSampleWorkspace

import unittest
from unittest import mock
from unittest.mock import MagicMock


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

    def _create_detector_peaks(self, det_id: int, location: np.ndarray) -> DetectorPeaks:
        return DetectorPeaks([Peak(det_id, location, (1, 1, 1), 100, 1000, 100, 100)])

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

    def test_update_picked_detectors(self):
        self._model._workspace_indices = np.array([0, 1, 2])
        self._model._is_valid = np.array([True, True, True])
        self._model._is_masked = np.array([False, False, False])
        self._model._detector_is_picked = np.array([True, True, False])
        self._model._detector_ids = np.array([1, 2, 3])
        self._model.picked_detectors_info_text = MagicMock(return_value=["a", "a"])
        self._model.extract_spectra_for_line_plot = MagicMock()
        self._presenter._pickable_mesh = {}
        self._presenter._pickable_projection_mesh = {}
        self._mock_view.current_selected_unit.return_value = "TOF"
        self._mock_view.sum_spectra_selected.return_value = True
        self._presenter.update_picked_detectors([])
        np.testing.assert_allclose(self._presenter._pickable_mesh[self._presenter._visible_label], self._model._detector_is_picked)
        self._mock_view.show_plot_for_detectors.assert_called_once_with(self._model.line_plot_workspace)
        self._mock_view.set_selected_detector_info.assert_called_once_with(["a", "a"])
        self._model.extract_spectra_for_line_plot.assert_called_once_with("TOF", True)

    def test_generate_single_colour(self):
        green_vector = self._presenter.generate_single_colour(2, 0, 1, 0, 0)
        self.assertEqual(len(green_vector), 2)
        self.assertTrue(green_vector.all(where=[0, 1, 0, 0]))

    def test_set_multi_select_enabled(self):
        self._mock_view.is_multi_picking_checkbox_checked.return_value = True
        self._presenter.update_detector_picker()
        self._mock_view.enable_rectangle_picking.assert_called_once()
        self._mock_view.enable_point_picking.assert_not_called()

    def test_set_multi_select_disabled(self):
        self._mock_view.is_multi_picking_checkbox_checked.return_value = False
        self._presenter.update_detector_picker()
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
        self.assertEqual(["No units"], units)

    @mock.patch.object(FullInstrumentViewModel, "has_unit", new_callable=mock.PropertyMock)
    def test_available_units_has_units(self, mock_has_unit):
        mock_has_unit.return_value = True
        units = self._presenter.available_unit_options()
        mock_has_unit.assert_called_once()
        self.assertEqual(self._presenter._UNIT_OPTIONS, units)

    def test_model_refresh_on_correct_ws_replace(self):
        self._model.setup = MagicMock()
        ws_name = self._model.workspace.name()
        self._presenter.replace_workspace_callback(ws_name, None)
        self._model.setup.assert_called_once()
        self._mock_view.setup.reset_mock()
        self._presenter.replace_workspace_callback("not_my_workspace", None)
        self._mock_view.setup.assert_not_called()

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

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.set_peaks_workspaces")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.refresh_lineplot_peaks")
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.peak_overlay_points")
    def test_on_peaks_workspace_selected(self, mock_peak_overlay_points, mock_refresh_lineplot_peaks, mock_set_peaks_workspaces):
        mock_peak_overlay_points.return_value = [[self._create_detector_peaks(50, np.zeros(3))]]
        self._model._calculate_projection = MagicMock(return_value=np.array([np.zeros(3), np.zeros(3)]))
        self._model._detector_ids = np.array([50, 52])
        self._model._is_valid = np.array([True, True])
        self._model._is_masked = np.array([False, False])
        self._presenter.on_peaks_workspace_selected()
        mock_refresh_lineplot_peaks.assert_called_once()
        self._mock_view.clear_overlay_meshes.assert_called_once()
        mock_set_peaks_workspaces.assert_called_once()
        self._mock_view.plot_overlay_mesh.assert_called_once()

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


if __name__ == "__main__":
    unittest.main()
