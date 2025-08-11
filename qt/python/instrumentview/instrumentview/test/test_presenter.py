# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateSampleWorkspace
from instrumentview.FullInstrumentViewPresenter import FullInstrumentViewPresenter
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
import unittest
from unittest import mock
from unittest.mock import MagicMock


class TestFullInstrumentViewPresenter(unittest.TestCase):
    def setUp(self):
        self._mock_view = MagicMock()
        self._ws = CreateSampleWorkspace(OutputWorkspace="TestFullInstrumentViewPresenter")
        self._model = FullInstrumentViewModel(self._ws)
        self._presenter = FullInstrumentViewPresenter(self._mock_view, self._model, model_setup_on_separate_thread=False)
        self._mock_view.reset_mock()

    def tearDown(self):
        self._ws.delete()
        pass

    def test_projection_combo_options(self):
        _, projections = self._presenter.projection_combo_options()
        self.assertGreater(len(projections), 0)
        self.assertTrue("Spherical X" in projections)

    def test_projection_option_selected(self):
        self._presenter.on_projection_option_selected(1)
        self._mock_view.add_projection_mesh.assert_called()

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

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.set_tof_limits")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._parse_min_max")
    def test_on_tof_limits_updated_true(self, mock_parse_min_max, mock_set_tof_limits):
        mock_parse_min_max.return_value = (0, 100)
        self._presenter.on_tof_limits_updated()
        mock_parse_min_max.assert_called_once()
        mock_set_tof_limits.assert_called_once_with(0, 100)

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.set_tof_limits")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._parse_min_max")
    def test_on_tof_limits_updated_false(self, mock_parse_min_max, mock_set_tof_limits):
        mock_parse_min_max.return_value = ()
        self._presenter.on_tof_limits_updated()
        mock_parse_min_max.assert_called_once()
        mock_set_tof_limits.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.set_contour_limits")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._parse_min_max")
    def test_on_contour_limits_updated_false(self, mock_parse_min_max, mock_set_contour_limits):
        mock_parse_min_max.return_value = ()
        self._presenter.on_contour_limits_updated()
        mock_set_contour_limits.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.set_contour_limits")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter._parse_min_max")
    def test_on_contour_limits_updated_true(self, mock_parse_min_max, mock_set_contour_limits):
        mock_parse_min_max.return_value = (0, 100)
        self._presenter.on_contour_limits_updated()
        mock_set_contour_limits.assert_called_once_with(0, 100)

    def _do_test_parse_min_max_valid(self, min: str | float, max: str | float, exp_is_valid: bool, exp_min: int, exp_max: int) -> None:
        limits = self._presenter._parse_min_max(min, max)
        self.assertEqual(exp_is_valid, bool(limits))
        if limits:
            min, max = limits
            self.assertEqual(exp_min, min)
            self.assertEqual(exp_max, max)

    def test_parse_min_max_valid(self):
        self._do_test_parse_min_max_valid("1", "200", True, 1, 200)

    def test_parse_min_max_int_valid(self):
        self._do_test_parse_min_max_valid(1, 200, True, 1, 200)

    def test_parse_min_max_float_valid(self):
        self._do_test_parse_min_max_valid(1.6, 200.8, True, 1, 200)

    def test_parse_min_max_reverse(self):
        self._do_test_parse_min_max_valid("100", "5", False, 0, 0)

    def test_parse_min_max_invalid_min(self):
        self._do_test_parse_min_max_valid("boom", "5", False, 0, 0)

    def test_parse_min_max_invalid_max(self):
        self._do_test_parse_min_max_valid("5", "boom", False, 0, 0)

    def test_set_contour_limits(self):
        self._presenter.set_contour_limits(0, 100)
        self._mock_view.set_plotter_scalar_bar_range.assert_called_once_with([0, 100], self._presenter._counts_label)

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.update_time_of_flight_range")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.set_contour_limits")
    def test_set_tof_limits(self, mock_set_contour_limits, mock_update_time_of_flight_range):
        self._presenter.set_tof_limits(0, 100)
        mock_update_time_of_flight_range.assert_called_once()
        mock_set_contour_limits.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.update_picked_detectors")
    def test_point_picked(self, mock_update_picked_detectors):
        mock_picker = MagicMock()

        def get_point_id():
            return 1

        mock_picker.GetPointId = get_point_id
        self._presenter.point_picked(MagicMock(), mock_picker)
        mock_update_picked_detectors.assert_called_with([1])

    def test_update_picked_detectors(self):
        mock_model = MagicMock()
        self._presenter._model = mock_model

        def picked_visibility():
            return [True, True, False]

        def picked_workspace_indices():
            return [0, 1]

        def picked_detectors_info_text():
            return ["a", "a"]

        mock_model.picked_visibility = picked_visibility
        mock_model.picked_workspace_indices = picked_workspace_indices
        mock_model.picked_detectors_info_text = picked_detectors_info_text

        self._presenter._pickable_main_mesh = {}
        self._presenter._pickable_projection_mesh = {}

        self._presenter._model = mock_model

        self._presenter.update_picked_detectors([])

        self.assertEqual(self._presenter._pickable_main_mesh["visibility"], [True, True, False])
        self.assertEqual(self._presenter._pickable_projection_mesh["visibility"], [True, True, False])

        self._mock_view.set_plot_for_detectors.assert_called_once_with(mock_model.workspace(), [0, 1])
        self._mock_view.set_selected_detector_info.assert_called_once_with(["a", "a"])

    def test_generate_single_colour(self):
        green_vector = self._presenter.generate_single_colour(2, 0, 1, 0, 0)
        self.assertEqual(len(green_vector), 2)
        self.assertTrue(green_vector.all(where=[0, 1, 0, 0]))

    def test_set_multi_select_enabled(self):
        self._presenter.on_multi_select_detectors_clicked(2)
        self._mock_view.enable_rectangle_picking.assert_called_once()
        self._mock_view.enable_point_picking.assert_not_called()

    def test_set_multi_select_disabled(self):
        self._presenter.on_multi_select_detectors_clicked(1)
        self._mock_view.enable_rectangle_picking.assert_not_called()
        self._mock_view.enable_point_picking.assert_called_once()
