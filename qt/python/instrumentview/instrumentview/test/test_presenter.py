# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateSampleWorkspace
from instrumentview.FullInstrumentViewPresenter import FullInstrumentViewPresenter
import unittest
from unittest import mock
from unittest.mock import MagicMock


class TestFullInstrumentViewPresenter(unittest.TestCase):
    def setUp(self):
        self._mock_view = MagicMock()
        self._ws = CreateSampleWorkspace(OutputWorkspace="TestFullInstrumentViewPresenter")
        self._presenter = FullInstrumentViewPresenter(self._mock_view, self._ws)
        self._mock_view.reset_mock()

    def tearDown(self):
        self._ws.delete()

    def test_projection_combo_options(self):
        projections = self._presenter.projection_combo_options()
        self.assertGreater(len(projections), 0)
        self.assertTrue("Spherical X" in projections)

    def test_projection_option_selected(self):
        self._presenter.projection_option_selected(1)
        self._mock_view.add_projection_mesh.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.createPolyDataMesh")
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.calculate_projection")
    def test_projection_option_axis(self, mock_calculate_projection, mock_createPolyDataMesh):
        for option_index in range(len(self._presenter.projection_combo_options())):
            option = self._presenter.projection_combo_options()[option_index]
            if option.endswith("X"):
                axis = [1, 0, 0]
            elif option.endswith("Y"):
                axis = [0, 1, 0]
            elif option.endswith("Z"):
                axis = [0, 0, 1]
            else:
                return
            self._presenter.projection_option_selected(option_index)
            mock_calculate_projection.assert_called_once_with(option.startswith("Spherical"), axis)
            mock_createPolyDataMesh.assert_called_once()
            mock_calculate_projection.reset_mock()
            mock_createPolyDataMesh.reset_mock()

    def test_set_contour_limits(self):
        self._presenter.set_contour_limits(0, 100)
        self._mock_view.update_scalar_range.assert_called_once_with([0, 100], self._presenter._counts_label)

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.update_time_of_flight_range")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.set_contour_limits")
    def test_set_tof_limits(self, mock_set_contour_limits, mock_update_time_of_flight_range):
        self._presenter.set_tof_limits(0, 100)
        mock_update_time_of_flight_range.assert_called_once()
        mock_set_contour_limits.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.show_info_text_for_detectors")
    @mock.patch("instrumentview.FullInstrumentViewPresenter.FullInstrumentViewPresenter.show_plot_for_detectors")
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.detector_index")
    def test_point_picked(self, mock_detector_index, mock_show_plot, mock_show_info_text):
        mock_detector_index.return_value = 10
        mock_picker = MagicMock()

        def get_point_id():
            return 1

        mock_picker.GetPointId = get_point_id
        self._presenter.point_picked(MagicMock(), mock_picker)
        mock_show_plot.assert_called_once_with([10])
        mock_show_info_text.assert_called_once_with([10])

    def test_generate_single_colour(self):
        green_vector = self._presenter.generateSingleColour([[1, 0, 0], [0, 1, 0]], 0, 1, 0, 0)
        self.assertEqual(len(green_vector), 2)
        self.assertTrue(green_vector.all(where=[0, 1, 0, 0]))

    def test_show_plot_for_detectors(self):
        mock_model = MagicMock()
        self._presenter._model = mock_model

        def mock_workspace_index(i):
            return 2 * i

        mock_model.workspace_index_from_detector_index = mock_workspace_index
        self._presenter.show_plot_for_detectors([0, 1, 2])
        self._mock_view.show_plot_for_detectors.assert_called_once_with(mock_model.workspace(), [0, 2, 4])

    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel.get_detector_info_text")
    def test_show_info_text_for_detectors(self, mock_get_detector_info_text):
        mock_get_detector_info_text.return_value = "a"
        self._presenter.show_info_text_for_detectors([0, 1, 2])
        self._mock_view.update_selected_detector_info.assert_called_once_with(["a", "a", "a"])

    def test_set_multi_select_enabled(self):
        self._presenter.set_multi_select_enabled(True)
        self._mock_view.enable_rectangle_picking.assert_called_once()
        self._mock_view.enable_point_picking.assert_not_called()

    def test_set_multi_select_disabled(self):
        self._presenter.set_multi_select_enabled(False)
        self._mock_view.enable_rectangle_picking.assert_not_called()
        self._mock_view.enable_point_picking.assert_called_once()
