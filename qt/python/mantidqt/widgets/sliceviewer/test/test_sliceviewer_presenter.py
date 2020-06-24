# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import sys
import unittest
from unittest import mock

import matplotlib
matplotlib.use('Agg')  # noqa: E402
# Mock out simpleapi to import expensive import of something we don't use anyway
sys.modules['mantid.simpleapi'] = mock.MagicMock()  # noqa: E402

import mantid.api
from mantidqt.widgets.sliceviewer.model import SliceViewerModel, WS_TYPE
from mantidqt.widgets.sliceviewer.presenter import SliceViewer
from mantidqt.widgets.sliceviewer.view import SliceViewerView, SliceViewerDataView
from mantidqt.widgets.sliceviewer.presenter import PeaksViewerCollectionPresenter


def _create_presenter(model, view, mock_sliceinfo_cls, enable_nonortho_axes, supports_nonortho):
    model.get_ws_type = mock.Mock(return_value=WS_TYPE.MDH)
    data_view_mock = view.data_view
    data_view_mock.plot_MDH = mock.Mock()
    presenter = SliceViewer(None, model=model, view=view)
    if enable_nonortho_axes:
        presenter.nonorthogonal_axes(True)
    else:
        data_view_mock.nonorthogonal_mode = False
    data_view_mock.create_axes_orthogonal.reset_mock()
    data_view_mock.create_axes_nonorthogonal.reset_mock()
    mock_sliceinfo_instance = mock_sliceinfo_cls.return_value
    mock_sliceinfo_instance.can_support_nonorthogonal_axes.return_value = supports_nonortho

    return presenter, data_view_mock


class SliceViewerTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.Mock(spec=SliceViewerView)
        data_view = mock.Mock(spec=SliceViewerDataView)
        data_view.plot_MDH = mock.Mock()
        data_view.dimensions = mock.Mock()
        data_view.norm_opts = mock.Mock()
        data_view.image_info_widget = mock.Mock()
        dimensions = mock.Mock()
        dimensions.get_slicepoint.return_value = [None, None, 0.5]
        dimensions.transpose = False
        dimensions.get_slicerange.return_value = [None, None, (-15, 15)]
        dimensions.qflags = [True, True, True]
        data_view.dimensions = dimensions
        self.view.data_view = data_view

        self.model = mock.Mock(spec=SliceViewerModel)
        self.model.get_ws = mock.Mock()
        self.model.get_data = mock.Mock()

    def test_sliceviewer_MDH(self):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MDH)

        presenter = SliceViewer(None, model=self.model, view=self.view)

        # setup calls
        self.assertEqual(self.model.get_dimensions_info.call_count, 0)
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 1)
        self.assertEqual(self.view.data_view.plot_MDH.call_count, 1)

        # new_plot
        self.model.reset_mock()
        self.view.reset_mock()
        presenter.new_plot()
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 1)
        self.assertEqual(self.view.data_view.plot_MDH.call_count, 1)

        # update_plot_data
        self.model.reset_mock()
        self.view.reset_mock()
        presenter.update_plot_data()
        self.assertEqual(self.model.get_data.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 1)
        self.assertEqual(self.view.data_view.update_plot_data.call_count, 1)

    def test_sliceviewer_MDE(self):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MDE)

        presenter = SliceViewer(None, model=self.model, view=self.view)

        # setup calls
        self.assertEqual(self.model.get_dimensions_info.call_count, 0)
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_bin_params.call_count, 1)
        self.assertEqual(self.view.data_view.plot_MDH.call_count, 1)

        # new_plot
        self.model.reset_mock()
        self.view.reset_mock()
        presenter.new_plot()
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_bin_params.call_count, 1)
        self.assertEqual(self.view.data_view.plot_MDH.call_count, 1)

        # update_plot_data
        self.model.reset_mock()
        self.view.reset_mock()
        presenter.update_plot_data()
        self.assertEqual(self.model.get_data.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_bin_params.call_count, 1)
        self.assertEqual(self.view.data_view.update_plot_data.call_count, 1)

    def test_sliceviewer_matrix(self):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MATRIX)

        presenter = SliceViewer(None, model=self.model, view=self.view)

        # setup calls
        self.assertEqual(self.model.get_dimensions_info.call_count, 0)
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 0)
        self.assertEqual(self.view.data_view.plot_matrix.call_count, 1)

        # new_plot
        self.model.reset_mock()
        self.view.reset_mock()
        presenter.new_plot()
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 0)
        self.assertEqual(self.view.data_view.plot_matrix.call_count, 1)

    def test_normalization_change_set_correct_normalization(self):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MATRIX)
        self.view.data_view.plot_matrix = mock.Mock()

        presenter = SliceViewer(None, model=self.model, view=self.view)
        presenter.normalization_changed("By bin width")
        self.view.data_view.plot_matrix.assert_called_with(
            self.model.get_ws(), normalize=mantid.api.MDNormalization.VolumeNormalization)

    def peaks_button_disabled_if_model_cannot_support_it(self):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MATRIX)
        self.model.can_support_peaks_overlay.return_value = False

        SliceViewer(None, model=self.model, view=self.view)

        self.view.data_view.disable_peaks_button.assert_called_once()

    def peaks_button_not_disabled_if_model_can_support_it(self):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MATRIX)
        self.model.can_support_peaks_overlay.return_value = True

        SliceViewer(None, model=self.model, view=self.view)

        self.view.data_view.disable_peaks_button.assert_not_called()

    def test_non_orthogonal_axes_toggled_on(self):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MATRIX)
        data_view_mock = self.view.data_view
        data_view_mock.plot_matrix = mock.Mock()

        presenter = SliceViewer(None, model=self.model, view=self.view)
        data_view_mock.plot_matrix.reset_mock()  # clear initial plot call
        data_view_mock.create_axes_orthogonal.reset_mock()
        presenter.nonorthogonal_axes(True)

        data_view_mock.remove_line_plots.assert_called_once()
        data_view_mock.create_axes_nonorthogonal.assert_called_once()
        data_view_mock.create_axes_orthogonal.assert_not_called()
        data_view_mock.plot_matrix.assert_called_once()
        data_view_mock.disable_lineplots_button.assert_called_once()
        data_view_mock.disable_peaks_button.assert_called_once()

    def test_non_orthogonal_axes_toggled_off(self):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MATRIX)
        data_view_mock = self.view.data_view
        data_view_mock.plot_matrix = mock.Mock()
        presenter = SliceViewer(None, model=self.model, view=self.view)
        presenter.nonorthogonal_axes(True)
        data_view_mock.plot_matrix.reset_mock()  # clear initial plot call
        data_view_mock.create_axes_orthogonal.reset_mock()
        data_view_mock.create_axes_nonorthogonal.reset_mock()
        data_view_mock.remove_line_plots.reset_mock()

        presenter.nonorthogonal_axes(False)

        data_view_mock.create_axes_orthogonal.assert_called_once()
        data_view_mock.create_axes_nonorthogonal.assert_not_called()
        data_view_mock.plot_matrix.assert_called_once()
        data_view_mock.enable_lineplots_button.assert_called_once()
        data_view_mock.enable_peaks_button.assert_called_once()

    def test_request_to_show_all_data_sets_correct_limits_on_view(self):
        presenter = SliceViewer(None, model=self.model, view=self.view)
        self.model.get_dim_limits.return_value = ((-1, 1), (-2, 2))

        presenter.show_all_data_requested()

        data_view = self.view.data_view
        self.model.get_dim_limits.assert_called_once_with([None, None, 0.5],
                                                          data_view.dimensions.transpose)
        data_view.set_axes_limits.assert_called_once_with((-1, 1), (-2, 2))

    @mock.patch("mantidqt.widgets.sliceviewer.presenter.SliceInfo")
    def test_changing_dimensions_in_nonortho_mode_switches_to_ortho_when_dim_not_Q(
            self, mock_sliceinfo_cls):
        presenter, data_view_mock = _create_presenter(self.model,
                                                      self.view,
                                                      mock_sliceinfo_cls,
                                                      enable_nonortho_axes=True,
                                                      supports_nonortho=False)

        presenter.dimensions_changed()

        data_view_mock.disable_nonorthogonal_axes_button.assert_called_once()
        data_view_mock.create_axes_orthogonal.assert_called_once()
        data_view_mock.create_axes_nonorthogonal.assert_not_called()

    @mock.patch("mantidqt.widgets.sliceviewer.presenter.SliceInfo")
    def test_changing_dimensions_in_nonortho_mode_keeps_nonortho_when_dim_is_Q(
            self, mock_sliceinfo_cls):
        presenter, data_view_mock = _create_presenter(self.model,
                                                      self.view,
                                                      mock_sliceinfo_cls,
                                                      enable_nonortho_axes=True,
                                                      supports_nonortho=True)

        presenter.dimensions_changed()

        data_view_mock.create_axes_nonorthogonal.assert_called_once()
        data_view_mock.disable_nonorthogonal_axes_button.assert_not_called()
        data_view_mock.create_axes_orthogonal.assert_not_called()

    @mock.patch("mantidqt.widgets.sliceviewer.presenter.SliceInfo")
    def test_changing_dimensions_in_ortho_mode_disables_nonortho_btn_if_not_supported(
            self, mock_sliceinfo_cls):
        presenter, data_view_mock = _create_presenter(self.model,
                                                      self.view,
                                                      mock_sliceinfo_cls,
                                                      enable_nonortho_axes=False,
                                                      supports_nonortho=False)

        presenter.dimensions_changed()

        data_view_mock.disable_nonorthogonal_axes_button.assert_called_once()

    @mock.patch("mantidqt.widgets.sliceviewer.presenter.SliceInfo")
    def test_changing_dimensions_in_ortho_mode_enables_nonortho_btn_if_supported(
            self, mock_sliceinfo_cls):
        presenter, data_view_mock = _create_presenter(self.model,
                                                      self.view,
                                                      mock_sliceinfo_cls,
                                                      enable_nonortho_axes=False,
                                                      supports_nonortho=True)

        presenter.dimensions_changed()

        data_view_mock.enable_nonorthogonal_axes_button.assert_called_once()

    @mock.patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.TableWorkspaceDataPresenter")
    @mock.patch("mantidqt.widgets.sliceviewer.presenter.PeaksViewerCollectionPresenter",
                spec=PeaksViewerCollectionPresenter)
    def test_overlay_peaks_workspaces_attaches_view_and_draws_peaks(self, mock_peaks_presenter, _):
        presenter = SliceViewer(None, model=self.model, view=self.view)

        presenter.view.query_peaks_to_overlay.side_effect = ["peaks_workspace"]
        presenter.overlay_peaks_workspaces()
        presenter.view.query_peaks_to_overlay.assert_called_once()
        mock_peaks_presenter.assert_called_once()
        mock_peaks_presenter.overlay_peaksworkspaces.asssert_called_once()

    def test_gui_starts_with_zoom_selected(self):
        SliceViewer(None, model=self.model, view=self.view)
        self.assertEqual(1, self.view.data_view.select_zoom.call_count)


if __name__ == '__main__':
    unittest.main()
