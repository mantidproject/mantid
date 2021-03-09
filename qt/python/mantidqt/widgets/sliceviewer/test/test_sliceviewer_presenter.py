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
from unittest.mock import patch
from mantid.api import MultipleExperimentInfos

import matplotlib

matplotlib.use('Agg')
# Mock out simpleapi to import expensive import of something we don't use anyway
sys.modules['mantid.simpleapi'] = mock.MagicMock()

from mantidqt.widgets.sliceviewer.model import SliceViewerModel, WS_TYPE  # noqa: E402
from mantidqt.widgets.sliceviewer.presenter import (  # noqa: E402
    PeaksViewerCollectionPresenter, SliceViewer)
from mantidqt.widgets.sliceviewer.transform import NonOrthogonalTransform  # noqa: E402
from mantidqt.widgets.sliceviewer.toolbar import ToolItemText  # noqa: E402
from mantidqt.widgets.sliceviewer.view import SliceViewerView, SliceViewerDataView  # noqa: E402


def _create_presenter(model, view, mock_sliceinfo_cls, enable_nonortho_axes, supports_nonortho):
    model.get_ws_type = mock.Mock(return_value=WS_TYPE.MDH)
    model.get_dim_limits.return_value = ((-1, 1), (-2, 2))
    data_view_mock = view.data_view
    data_view_mock.plot_MDH = mock.Mock()
    presenter = SliceViewer(None, model=model, view=view)
    if enable_nonortho_axes:
        data_view_mock.nonorthogonal_mode = True
        data_view_mock.nonortho_transform = mock.MagicMock(NonOrthogonalTransform)
        data_view_mock.nonortho_transform.tr.return_value = (0, 1)
        presenter.nonorthogonal_axes(True)
    else:
        data_view_mock.nonorthogonal_mode = False
        data_view_mock.nonortho_transform = None

    data_view_mock.disable_tool_button.reset_mock()
    data_view_mock.create_axes_orthogonal.reset_mock()
    data_view_mock.create_axes_nonorthogonal.reset_mock()
    mock_sliceinfo_instance = mock_sliceinfo_cls.return_value
    mock_sliceinfo_instance.can_support_nonorthogonal_axes.return_value = supports_nonortho

    return presenter, data_view_mock


def create_workspace_mock():
    # Mock out workspace methods needed for SliceViewerModel.__init__
    workspace = mock.Mock(spec=MultipleExperimentInfos)
    workspace.isMDHistoWorkspace = lambda: False
    workspace.getNumDims = lambda: 2
    workspace.name = lambda: "workspace"

    return workspace


class SliceViewerTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.Mock(spec=SliceViewerView)
        data_view = mock.Mock(spec=SliceViewerDataView)
        data_view.plot_MDH = mock.Mock()
        data_view.dimensions = mock.Mock()
        data_view.norm_opts = mock.Mock()
        data_view.image_info_widget = mock.Mock()
        data_view.nonorthogonal_mode = False
        data_view.nonortho_transform = None
        data_view.get_axes_limits.return_value = None

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
        self.model.rebin = mock.Mock()
        self.model.workspace_equals = mock.Mock()
        self.model.get_properties.return_value = {
            "workspace_type": "WS_TYPE.MATRIX",
            "supports_normalise": True,
            "supports_nonorthogonal_axes": False,
            "supports_dynamic_rebinning": False,
            "supports_peaks_overlays": True
        }

    @patch("sip.isdeleted", return_value=False)
    def test_sliceviewer_MDH(self, _):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MDH)

        presenter = SliceViewer(None, model=self.model, view=self.view)

        # setup calls
        self.assertEqual(self.model.get_dimensions_info.call_count, 0)
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.model.get_properties.call_count, 1)
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

    @patch("sip.isdeleted", return_value=False)
    def test_sliceviewer_MDE(self, _):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MDE)

        presenter = SliceViewer(None, model=self.model, view=self.view)

        # setup calls
        self.assertEqual(self.model.get_dimensions_info.call_count, 0)
        self.assertEqual(self.model.get_ws_MDE.call_count, 1)
        self.assertEqual(self.model.get_properties.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_bin_params.call_count, 1)
        self.assertEqual(self.view.data_view.plot_MDH.call_count, 1)

        # new_plot
        self.model.reset_mock()
        self.view.reset_mock()
        presenter.new_plot()
        self.assertEqual(self.model.get_ws_MDE.call_count, 1)
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

    @patch("sip.isdeleted", return_value=False)
    def test_sliceviewer_matrix(self, _):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MATRIX)

        presenter = SliceViewer(None, model=self.model, view=self.view)

        # setup calls
        self.assertEqual(self.model.get_dimensions_info.call_count, 0)
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.model.get_properties.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 0)
        self.assertEqual(self.view.data_view.plot_matrix.call_count, 1)

        # new_plot
        self.model.reset_mock()
        self.view.reset_mock()
        presenter.new_plot()
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 0)
        self.assertEqual(self.view.data_view.plot_matrix.call_count, 1)

    @patch("sip.isdeleted", return_value=False)
    def test_normalization_change_set_correct_normalization(self, _):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MATRIX)
        self.view.data_view.plot_matrix = mock.Mock()

        presenter = SliceViewer(None, model=self.model, view=self.view)
        presenter.normalization_changed("By bin width")
        self.view.data_view.plot_matrix.assert_called_with(self.model.get_ws(), distribution=False)

    def peaks_button_disabled_if_model_cannot_support_it(self):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MATRIX)
        self.model.can_support_peaks_overlay.return_value = False

        SliceViewer(None, model=self.model, view=self.view)

        self.view.data_view.disable_tool_button.assert_called_once_with(ToolItemText.OVERLAY_PEAKS)

    def peaks_button_not_disabled_if_model_can_support_it(self):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MATRIX)
        self.model.can_support_peaks_overlay.return_value = True

        SliceViewer(None, model=self.model, view=self.view)

        self.view.data_view.disable_tool_button.assert_not_called()

    @patch("sip.isdeleted", return_value=False)
    def test_non_orthogonal_axes_toggled_on(self, _):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MDE)
        self.model.get_dim_limits.return_value = ((-1, 1), (-2, 2))
        data_view_mock = self.view.data_view
        data_view_mock.plot_MDH = mock.Mock()

        presenter = SliceViewer(None, model=self.model, view=self.view)

        data_view_mock.plot_MDH.reset_mock()  # clear initial plot call
        data_view_mock.create_axes_orthogonal.reset_mock()
        presenter.nonorthogonal_axes(True)

        data_view_mock.deactivate_and_disable_tool.assert_called_once_with(
            ToolItemText.REGIONSELECTION)
        data_view_mock.create_axes_nonorthogonal.assert_called_once()
        data_view_mock.create_axes_orthogonal.assert_not_called()
        self.assertEqual(data_view_mock.plot_MDH.call_count, 2)
        data_view_mock.disable_tool_button.assert_has_calls([mock.call(ToolItemText.LINEPLOTS)])

    @patch("sip.isdeleted", return_value=False)
    @mock.patch("mantidqt.widgets.sliceviewer.presenter.SliceInfo")
    def test_non_orthogonal_axes_toggled_off(self, mock_sliceinfo_cls, _):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MDE)
        presenter, data_view_mock = _create_presenter(self.model,
                                                      self.view,
                                                      mock_sliceinfo_cls,
                                                      enable_nonortho_axes=True,
                                                      supports_nonortho=True)

        data_view_mock.plot_MDH.reset_mock()  # clear initial plot call
        data_view_mock.create_axes_orthogonal.reset_mock()
        data_view_mock.create_axes_nonorthogonal.reset_mock()
        data_view_mock.enable_tool_button.reset_mock()
        data_view_mock.disable_tool_button.reset_mock()
        data_view_mock.remove_line_plots.reset_mock()

        presenter.nonorthogonal_axes(False)

        data_view_mock.create_axes_orthogonal.assert_called_once()
        data_view_mock.create_axes_nonorthogonal.assert_not_called()
        data_view_mock.plot_MDH.assert_called_once()
        data_view_mock.enable_tool_button.assert_has_calls(
            (mock.call(ToolItemText.LINEPLOTS), mock.call(ToolItemText.REGIONSELECTION)))

    @patch("sip.isdeleted", return_value=False)
    def test_request_to_show_all_data_sets_correct_limits_on_view(self, _):
        presenter = SliceViewer(None, model=self.model, view=self.view)
        self.model.get_dim_limits.return_value = ((-1, 1), (-2, 2))

        presenter.show_all_data_requested()

        data_view = self.view.data_view
        self.model.get_dim_limits.assert_called_once_with([None, None, 0.5],
                                                          data_view.dimensions.transpose)
        data_view.set_axes_limits.assert_called_once_with((-1, 1), (-2, 2))

    @patch("sip.isdeleted", return_value=False)
    def test_data_limits_changed_creates_new_plot_if_dynamic_rebinning_supported(self, _):
        presenter = SliceViewer(None, model=self.model, view=self.view)
        self.model.can_support_dynamic_rebinning.return_value = True
        new_plot_mock = mock.MagicMock()
        presenter.new_plot = new_plot_mock

        presenter.data_limits_changed()

        new_plot_mock.assert_called_once()

    @patch("sip.isdeleted", return_value=False)
    def test_data_limits_changed_does_not_create_new_plot_if_dynamic_rebinning_not_supported(self, _):
        presenter = SliceViewer(None, model=self.model, view=self.view)
        self.model.can_support_dynamic_rebinning.return_value = False
        new_plot_mock = mock.MagicMock()
        presenter.new_plot = new_plot_mock

        presenter.data_limits_changed()

        new_plot_mock.assert_not_called()

    @patch("sip.isdeleted", return_value=False)
    @mock.patch("mantidqt.widgets.sliceviewer.presenter.SliceInfo")
    def test_changing_dimensions_in_nonortho_mode_switches_to_ortho_when_dim_not_Q(
            self, mock_sliceinfo_cls, is_view_delete):
        presenter, data_view_mock = _create_presenter(self.model,
                                                      self.view,
                                                      mock_sliceinfo_cls,
                                                      enable_nonortho_axes=True,
                                                      supports_nonortho=False)

        presenter.dimensions_changed()

        data_view_mock.disable_tool_button.assert_called_once_with(ToolItemText.NONORTHOGONAL_AXES)
        data_view_mock.create_axes_orthogonal.assert_called_once()
        data_view_mock.create_axes_nonorthogonal.assert_not_called()

    @patch("sip.isdeleted", return_value=False)
    @mock.patch("mantidqt.widgets.sliceviewer.presenter.SliceInfo")
    def test_changing_dimensions_in_nonortho_mode_keeps_nonortho_when_dim_is_Q(
            self, mock_sliceinfo_cls, _):
        presenter, data_view_mock = _create_presenter(self.model,
                                                      self.view,
                                                      mock_sliceinfo_cls,
                                                      enable_nonortho_axes=True,
                                                      supports_nonortho=True)

        presenter.dimensions_changed()

        data_view_mock.create_axes_nonorthogonal.assert_called_once()
        data_view_mock.disable_tool_button.assert_not_called()
        data_view_mock.create_axes_orthogonal.assert_not_called()

    @patch("sip.isdeleted", return_value=False)
    @mock.patch("mantidqt.widgets.sliceviewer.presenter.SliceInfo")
    def test_changing_dimensions_in_ortho_mode_disables_nonortho_btn_if_not_supported(
            self, mock_sliceinfo_cls, _):
        presenter, data_view_mock = _create_presenter(self.model,
                                                      self.view,
                                                      mock_sliceinfo_cls,
                                                      enable_nonortho_axes=False,
                                                      supports_nonortho=False)

        presenter.dimensions_changed()

        data_view_mock.disable_tool_button.assert_called_once_with(ToolItemText.NONORTHOGONAL_AXES)

    @patch("sip.isdeleted", return_value=False)
    @mock.patch("mantidqt.widgets.sliceviewer.presenter.SliceInfo")
    def test_changing_dimensions_in_ortho_mode_enables_nonortho_btn_if_supported(
            self, mock_sliceinfo_cls, _):
        presenter, data_view_mock = _create_presenter(self.model,
                                                      self.view,
                                                      mock_sliceinfo_cls,
                                                      enable_nonortho_axes=False,
                                                      supports_nonortho=True)

        presenter.dimensions_changed()

        data_view_mock.enable_tool_button.assert_called_once_with(ToolItemText.NONORTHOGONAL_AXES)

    @patch("sip.isdeleted", return_value=False)
    @mock.patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.TableWorkspaceDataPresenterStandard")
    @mock.patch("mantidqt.widgets.sliceviewer.presenter.PeaksViewerCollectionPresenter",
                spec=PeaksViewerCollectionPresenter)
    def test_overlay_peaks_workspaces_attaches_view_and_draws_peaks(self, mock_peaks_presenter, *_):
        for nonortho_axes in (False, True):
            presenter, _ = _create_presenter(self.model, self.view, mock.MagicMock(), nonortho_axes,
                                             nonortho_axes)

            presenter.view.query_peaks_to_overlay.side_effect = ["peaks_workspace"]
            presenter.overlay_peaks_workspaces()
            presenter.view.query_peaks_to_overlay.assert_called_once()
            mock_peaks_presenter.assert_called_once()
            mock_peaks_presenter.overlay_peaksworkspaces.asssert_called_once()
            mock_peaks_presenter.reset_mock()
            presenter.view.query_peaks_to_overlay.reset_mock()

    @patch("sip.isdeleted", return_value=False)
    def test_gui_starts_with_zoom_selected(self, _):
        SliceViewer(None, model=self.model, view=self.view)

        self.view.data_view.activate_tool.assert_called_once_with(ToolItemText.ZOOM)

    @patch("sip.isdeleted", return_value=False)
    def test_replace_workspace_returns_when_the_workspace_is_not_the_model_workspace(self, _):
        self.model.workspace_equals.return_value = False
        presenter, _ = _create_presenter(self.model,
                                         self.view,
                                         mock.MagicMock(),
                                         enable_nonortho_axes=False,
                                         supports_nonortho=False)
        presenter.update_view = mock.Mock()
        presenter._decide_plot_update_methods = mock.Mock()
        other_workspace = mock.Mock()

        presenter.replace_workspace('other_workspace', other_workspace)

        presenter._decide_plot_update_methods.assert_not_called()
        presenter.update_view.assert_not_called()

    @patch("sip.isdeleted", return_value=False)
    def test_replace_workspace_closes_view_when_model_properties_change(self, _):
        self.model.workspace_equals.return_value = True
        presenter, _ = _create_presenter(self.model,
                                         self.view,
                                         mock.MagicMock(),
                                         enable_nonortho_axes=False,
                                         supports_nonortho=False)
        presenter.refresh_view = mock.Mock()
        presenter._decide_plot_update_methods = mock.Mock()

        workspace = create_workspace_mock()

        # Not equivalent to self.model.get_properties()
        new_model_properties = {
            "workspace_type": "WS_TYPE.MDE",
            "supports_normalise": False,
            "supports_nonorthogonal_axes": False,
            "supports_dynamic_rebinning": False,
            "supports_peaks_overlays": True
        }

        with patch.object(SliceViewerModel, "get_properties", return_value=new_model_properties):
            presenter.replace_workspace('workspace', workspace)

            self.view.emit_close.assert_called_once()
            presenter._decide_plot_update_methods.assert_not_called()
            presenter.refresh_view.assert_not_called()

    @patch("sip.isdeleted", return_value=False)
    def test_replace_workspace_updates_view(self, _):
        presenter, _ = _create_presenter(self.model,
                                         self.view,
                                         mock.MagicMock(),
                                         enable_nonortho_axes=False,
                                         supports_nonortho=False)
        self.view.delayed_refresh = mock.Mock()
        presenter._decide_plot_update_methods = mock.Mock(
            return_value=(presenter.new_plot_matrix(), presenter.update_plot_data_matrix()))
        workspace = create_workspace_mock()
        new_model_properties = self.model.get_properties()

        # Patch get_properties so that the properties of the new model match those of self.model
        with patch.object(SliceViewerModel, "get_properties", return_value=new_model_properties):
            presenter.replace_workspace('workspace', workspace)

            self.view.emit_close.assert_not_called()
            presenter._decide_plot_update_methods.assert_called_once()
            self.view.delayed_refresh.assert_called_once()

    @patch("sip.isdeleted", return_value=False)
    def test_refresh_view(self, _):
        presenter, _ = _create_presenter(self.model,
                                         self.view,
                                         mock.MagicMock(),
                                         enable_nonortho_axes=False,
                                         supports_nonortho=False)
        presenter.new_plot = mock.Mock()

        presenter.refresh_view()

        self.view.data_view.image_info_widget.setWorkspace.assert_called()
        self.view.setWindowTitle.assert_called_with(self.model.get_title())
        presenter.new_plot.assert_called_once()

    @patch("sip.isdeleted", return_value=True)
    def test_refresh_view_does_nothing_when_view_deleted(self, _):
        presenter, _ = _create_presenter(self.model,
                                         self.view,
                                         mock.MagicMock(),
                                         enable_nonortho_axes=False,
                                         supports_nonortho=False)
        presenter.new_plot = mock.Mock()

        presenter.refresh_view()

        self.view.data_view.image_info_widget.setWorkspace.assert_not_called()
        presenter.new_plot.assert_not_called()

    @patch("sip.isdeleted", return_value=False)
    def test_clear_observer_peaks_presenter_not_none(self, _):
        presenter, _ = _create_presenter(self.model,
                                         self.view,
                                         mock.MagicMock(),
                                         enable_nonortho_axes=False,
                                         supports_nonortho=False)
        presenter._peaks_presenter = mock.MagicMock()

        presenter.clear_observer()

        presenter._peaks_presenter.clear_observer.assert_called_once()

    @patch("sip.isdeleted", return_value=False)
    def test_clear_observer_peaks_presenter_is_none(self, _):
        presenter, _ = _create_presenter(self.model,
                                         self.view,
                                         mock.MagicMock(),
                                         enable_nonortho_axes=False,
                                         supports_nonortho=False)
        presenter._peaks_presenter = None

        # Will raise exception if misbehaving.
        presenter.clear_observer()


if __name__ == '__main__':
    unittest.main()
