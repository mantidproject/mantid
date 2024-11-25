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
from unittest.mock import patch, call
from mantid.api import IMDHistoWorkspace
from mantid.kernel import SpecialCoordinateSystem
from mantidqt.utils.qt.testing import start_qapplication

import matplotlib

# Must be called before anything tries to use matplotlib
matplotlib.use("Agg")
# Mock out simpleapi to import expensive import of something we don't use anyway
sys.modules["mantid.simpleapi"] = mock.MagicMock()

from mantidqt.widgets.sliceviewer.models.model import SliceViewerModel, WS_TYPE  # noqa: E402
from mantidqt.widgets.sliceviewer.presenters.presenter import (  # noqa: E402
    DBLMAX,
    PeaksViewerCollectionPresenter,
    SliceViewer,
    SliceViewXAxisEditor,
    SliceViewYAxisEditor,
)
from mantidqt.widgets.sliceviewer.models.transform import NonOrthogonalTransform  # noqa: E402
from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText  # noqa: E402
from mantidqt.widgets.sliceviewer.views.view import SliceViewerView  # noqa: E402
from mantidqt.widgets.sliceviewer.views.dataview import SliceViewerDataView  # noqa: E402


def _create_presenter(model: SliceViewerModel, view, mock_sliceinfo_cls, enable_nonortho_axes, supports_nonortho):
    data_view_mock = view.data_view
    data_view_mock.plot_MDH = mock.Mock()
    presenter = SliceViewer(None, model=model, view=view)
    # Patch out things from the base presenter
    presenter.show_all_data_clicked = mock.Mock()

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


def create_mdhistoworkspace_mock():
    # Mock out workspace methods needed for SliceViewerModel.__init__
    workspace = mock.Mock(spec=IMDHistoWorkspace)
    workspace.isMDHistoWorkspace = lambda: True
    workspace.getDimension = lambda: mock.MagicMock()
    workspace.getNumDims = lambda: 2
    workspace.getNumNonIntegratedDims = lambda: 2
    workspace.numOriginalWorkspaces = lambda: 0
    workspace.name = lambda: "workspace"
    workspace.readLock = mock.MagicMock()
    workspace.unlock = mock.MagicMock()

    return workspace


@start_qapplication
class SliceViewerTest(unittest.TestCase):
    def createMockDataView(self):
        data_view = mock.Mock(spec=SliceViewerDataView)
        data_view.plot_MDH = mock.Mock()
        data_view.dimensions = mock.Mock()
        data_view.norm_opts = mock.Mock()
        data_view.image_info_widget = mock.Mock()
        data_view.canvas = mock.Mock()
        data_view.help_button = mock.Mock()
        data_view.nonorthogonal_mode = False
        data_view.nonortho_transform = None
        data_view.get_data_limits_to_fill_current_axes.return_value = None
        return data_view

    def setUp(self):
        self.view = mock.Mock(spec=SliceViewerView)
        data_view = self.createMockDataView()
        dimensions = mock.Mock()
        dimensions.get_slicepoint.return_value = [None, None, 0.5]
        dimensions.transpose = False
        dimensions.get_slicerange.return_value = [None, None, (-15, 15)]
        dimensions.qflags = [True, True, True]
        data_view.dimensions = dimensions
        self.view.data_view = data_view

        self.view3D_non_QDim = mock.Mock(spec=SliceViewerView)
        data_view3D_non_QDim = self.createMockDataView()
        dimensions3D_non_QDim = mock.Mock()
        dimensions3D_non_QDim.get_slicepoint.return_value = [None, None, 0.5]
        dimensions3D_non_QDim.transpose = False
        dimensions3D_non_QDim.get_slicerange.return_value = [None, None, (-15, 15)]
        dimensions3D_non_QDim.qflags = [False, True, True]
        data_view3D_non_QDim.dimensions = dimensions3D_non_QDim
        self.view3D_non_QDim.data_view = data_view3D_non_QDim

        self.view4D = mock.Mock(spec=SliceViewerView)
        data_view4D = self.createMockDataView()
        dimensions4D = mock.Mock()
        dimensions4D.get_slicepoint.return_value = [None, None, 0.5, 0.5]
        dimensions4D.transpose = False
        dimensions4D.get_slicerange.return_value = [None, None, (-15, 15), (-15, 15)]
        dimensions4D.qflags = [False, True, True, True]
        data_view4D.dimensions = dimensions4D
        self.view4D.data_view = data_view4D

        self.model = mock.Mock(spec=SliceViewerModel)
        self.model.get_ws = mock.Mock()
        self.model.get_data = mock.Mock()
        self.model.rebin = mock.Mock()
        self.model.workspace_equals = mock.Mock()
        self.model.get_hkl_from_full_point.return_value = [1.0, 1.0, 1.0]
        self.model.get_properties.return_value = {
            "workspace_type": "WS_TYPE.MATRIX",
            "supports_normalise": True,
            "supports_nonorthogonal_axes": False,
            "supports_dynamic_rebinning": False,
            "supports_peaks_overlays": True,
        }
        self.model.check_for_removed_original_workspace.return_value = False

        self._ws_info_patcher = mock.patch.multiple(
            "mantidqt.widgets.sliceviewer.presenters.presenter", Dimensions=mock.DEFAULT, WorkspaceInfo=mock.DEFAULT
        )
        self.patched_deps = self._ws_info_patcher.start()
        self.patched_deps["Dimensions"].return_value.get_dim_limits.return_value = ((-1, 1), (-2, 2))
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MDH

    def tearDown(self) -> None:
        self._ws_info_patcher.stop()

    def test_on_close(self):
        pres = SliceViewer(mock.Mock(), model=mock.MagicMock(), view=mock.MagicMock())
        self.assertIsNotNone(pres.ads_observer)
        pres.clear_observer()
        self.assertEqual(pres.ads_observer, None)

    def test_notify_close_from_qt(self):
        pres = SliceViewer(mock.Mock(), model=mock.MagicMock(), view=mock.MagicMock())
        pres.notify_close()
        self.assertIsNone(pres.view)

    def test_sliceviewer_MDH(self):
        presenter = SliceViewer(None, model=self.model, view=self.view)

        # setup calls
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.model.get_properties.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 2)  # extra call during init of pres
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
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MDE

        presenter = SliceViewer(None, model=self.model, view=self.view)

        # setup calls
        self.assertEqual(self.model.get_ws_MDE.call_count, 1)
        self.assertEqual(self.model.get_properties.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 2)  # extra call during init of pres
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

    def test_sliceviewer_matrix(self):
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MATRIX

        presenter = SliceViewer(None, model=self.model, view=self.view)

        # setup calls
        self.assertEqual(self.model.get_properties.call_count, 1)
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 1)  # extra call during init of pres
        self.assertEqual(self.view.data_view.plot_matrix.call_count, 1)

        # new_plot
        self.model.reset_mock()
        self.view.reset_mock()
        presenter.new_plot()
        self.assertEqual(self.view.data_view.dimensions.get_slicepoint.call_count, 0)
        self.assertEqual(self.view.data_view.plot_matrix.call_count, 1)

    def test_normalization_change_set_correct_normalization(self):
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MATRIX
        self.view.data_view.plot_matrix = mock.Mock()

        presenter = SliceViewer(None, model=self.model, view=self.view)
        presenter.normalization_changed("By bin width")
        self.view.data_view.plot_matrix.assert_called_with(self.model.ws, distribution=False)

    def test_peaks_button_disabled_if_model_cannot_support_it(self):
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MATRIX
        self.model.can_support_peaks_overlays.return_value = False

        SliceViewer(None, model=self.model, view=self.view)

        self.view.data_view.disable_tool_button.assert_any_call(ToolItemText.OVERLAY_PEAKS)

    def test_peaks_button_not_disabled_if_model_can_support_it(self):
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MATRIX
        self.model.can_support_peaks_overlays.return_value = True

        SliceViewer(None, model=self.model, view=self.view)

        assert call(ToolItemText.OVERLAY_PEAKS) not in self.view.data_view.disable_tool_button.mock_calls

    def test_non_orthogonal_axes_toggled_on(self):
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MDE
        self.patched_deps["Dimensions"].return_value.get_dim_limits.return_value = ((-1, 1), (-2, 2))
        data_view_mock = self.view.data_view
        data_view_mock.plot_MDH = mock.Mock()

        presenter = SliceViewer(None, model=self.model, view=self.view)
        presenter.show_all_data_clicked = mock.Mock()
        presenter.new_plot = mock.Mock()
        data_view_mock.plot_MDH.reset_mock()  # clear initial plot call
        data_view_mock.create_axes_orthogonal.reset_mock()

        presenter.nonorthogonal_axes(True)

        data_view_mock.deactivate_and_disable_tool.assert_called_once_with(ToolItemText.REGIONSELECTION)
        data_view_mock.create_axes_nonorthogonal.assert_called_once()
        data_view_mock.create_axes_orthogonal.assert_not_called()
        data_view_mock.disable_tool_button.assert_has_calls([mock.call(ToolItemText.LINEPLOTS)])
        presenter.show_all_data_clicked.assert_called_once()
        presenter.new_plot.assert_called_once()

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceInfo")
    def test_non_orthogonal_axes_toggled_off(self, mock_sliceinfo_cls):
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MDE
        self.model.can_support_non_axis_cuts.return_value = True
        presenter, data_view_mock = _create_presenter(
            self.model, self.view, mock_sliceinfo_cls, enable_nonortho_axes=True, supports_nonortho=True
        )

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
        data_view_mock.enable_tool_button.assert_has_calls((mock.call(ToolItemText.LINEPLOTS), mock.call(ToolItemText.REGIONSELECTION)))

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceInfo")
    def test_non_orthogonal_axes_toggled_off_not_enable_non_axis_cuts_if_not_supported(self, mock_sliceinfo_cls):
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MDE
        self.model.can_support_non_axis_cuts.return_value = False
        presenter, data_view_mock = _create_presenter(
            self.model, self.view, mock_sliceinfo_cls, enable_nonortho_axes=True, supports_nonortho=True
        )

        data_view_mock.enable_tool_button.reset_mock()

        presenter.nonorthogonal_axes(False)

        self.assertTrue(mock.call(ToolItemText.NONAXISALIGNEDCUTS) not in data_view_mock.enable_tool_button.call_args_list)

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceInfo")
    def test_non_orthognoal_axes_toggled_will_hide_or_show_the_signal(self, mock_sliceinfo_cls):
        presenter, data_view_mock = _create_presenter(
            self.model, self.view, mock_sliceinfo_cls, enable_nonortho_axes=True, supports_nonortho=True
        )
        for button_state in {False, True}:
            data_view_mock.image_info_widget.setShowSignal.reset_mock()
            presenter.nonorthogonal_axes(button_state)
            data_view_mock.image_info_widget.setShowSignal.assert_called_once_with(not button_state)

    def test_cut_view_button_disabled_if_model_cannot_support_it(self):
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MATRIX
        self.model.can_support_non_axis_cuts.return_value = False

        SliceViewer(None, model=self.model, view=self.view)

        self.view.data_view.disable_tool_button.assert_has_calls([mock.call(ToolItemText.NONAXISALIGNEDCUTS)])

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.CutViewerPresenter", autospec=True)
    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.CutViewerView", autospec=True)
    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.CutViewerModel", autospec=True)
    def test_cut_view_toggled_on(self, mock_cv_model, mock_cv_view, mock_cv_pres):
        presenter = SliceViewer(None, model=self.model, view=self.view)
        self.view.data_view.track_cursor = mock.MagicMock()

        presenter.non_axis_aligned_cut(True)

        self.assertTrue(presenter._cutviewer_presenter is not None)
        # test correct buttons disabled
        self.view.data_view.deactivate_and_disable_tool.assert_has_calls(
            [mock.call(tool) for tool in (ToolItemText.REGIONSELECTION, ToolItemText.LINEPLOTS)]
        )
        self.view.data_view.deactivate_tool.assert_called_once_with(ToolItemText.ZOOM)
        self.view.data_view.track_cursor.setChecked.assert_called_once_with(False)

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceViewer.get_sliceinfo")
    def test_cut_view_toggled_off(self, mock_get_sliceinfo):
        presenter = SliceViewer(None, model=self.model, view=self.view)
        presenter._cutviewer_presenter = mock.MagicMock()
        self.hide_view_called = False
        presenter._cutviewer_presenter.hide_view.side_effect = lambda: setattr(self, "hide_view_called", True)
        mock_get_sliceinfo.return_value.can_support_nonorthogonal_axes.return_value = True

        presenter.non_axis_aligned_cut(False)

        self.assertTrue(self.hide_view_called)
        # test correct buttons disabled
        self.view.data_view.enable_tool_button.assert_has_calls(
            [mock.call(tool) for tool in (ToolItemText.REGIONSELECTION, ToolItemText.LINEPLOTS, ToolItemText.NONORTHOGONAL_AXES)]
        )
        self.assertTrue(presenter._cutviewer_presenter is None)

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceViewer.new_plot_MDH")
    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceInfo")
    def test_dimensions_changed_when_transpose_2D_MD_workspace(self, mock_sliceinfo_cls, mock_new_plot):
        presenter, data_view_mock = _create_presenter(
            self.model, self.view, mock_sliceinfo_cls, enable_nonortho_axes=False, supports_nonortho=False
        )
        # stop regression of issue #33241
        self.model.get_number_dimensions.return_value = 2
        mock_sliceinfo_cls.slicepoint = [None, None]  # no slicepoint as 2D ws
        data_view_mock.dimensions.get_previous_states.return_value = [0, 1]  # no None that indicates integrated dim

        presenter.dimensions_changed()

        mock_new_plot.assert_called_with(dimensions_transposing=True)

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceInfo")
    def test_changing_dimensions_in_nonortho_mode_switches_to_ortho_when_dim_not_Q(self, mock_sliceinfo_cls):
        presenter, data_view_mock = _create_presenter(
            self.model, self.view, mock_sliceinfo_cls, enable_nonortho_axes=True, supports_nonortho=False
        )
        self.model.get_number_dimensions.return_value = 2

        presenter.dimensions_changed()

        data_view_mock.disable_tool_button.assert_called_once_with(ToolItemText.NONORTHOGONAL_AXES)
        data_view_mock.create_axes_orthogonal.assert_called_once()
        data_view_mock.create_axes_nonorthogonal.assert_not_called()

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceInfo")
    def test_changing_dimensions_in_nonortho_mode_keeps_nonortho_when_dim_is_Q(self, mock_sliceinfo_cls):
        presenter, data_view_mock = _create_presenter(
            self.model, self.view, mock_sliceinfo_cls, enable_nonortho_axes=True, supports_nonortho=True
        )

        self.patched_deps["WorkspaceInfo"].is_ragged_matrix_workspace.return_value = False
        self.model.get_number_dimensions.return_value = 2

        presenter.dimensions_changed()

        data_view_mock.create_axes_nonorthogonal.assert_called_once()
        data_view_mock.disable_tool_button.assert_not_called()
        data_view_mock.create_axes_orthogonal.assert_not_called()

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceInfo")
    def test_changing_dimensions_in_ortho_mode_disables_nonortho_btn_if_not_supported(self, mock_sliceinfo_cls):
        presenter, data_view_mock = _create_presenter(
            self.model, self.view, mock_sliceinfo_cls, enable_nonortho_axes=False, supports_nonortho=False
        )
        self.model.get_number_dimensions.return_value = 2

        presenter.dimensions_changed()

        data_view_mock.disable_tool_button.assert_called_once_with(ToolItemText.NONORTHOGONAL_AXES)

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceInfo")
    def test_changing_dimensions_in_ortho_mode_enables_nonortho_btn_if_supported(self, mock_sliceinfo_cls):
        presenter, data_view_mock = _create_presenter(
            self.model, self.view, mock_sliceinfo_cls, enable_nonortho_axes=False, supports_nonortho=True
        )
        self.model.get_number_dimensions.return_value = 2

        presenter.dimensions_changed()

        data_view_mock.enable_tool_button.assert_called_once_with(ToolItemText.NONORTHOGONAL_AXES)

    @mock.patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.TableWorkspaceDataPresenterStandard")
    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.PeaksViewerCollectionPresenter", spec=PeaksViewerCollectionPresenter)
    def test_overlay_peaks_workspaces_attaches_view_and_draws_peaks(self, mock_peaks_presenter, _):
        for nonortho_axes in (False, True):
            presenter, _ = _create_presenter(self.model, self.view, mock.MagicMock(), nonortho_axes, nonortho_axes)

            presenter.view.query_peaks_to_overlay.side_effect = ["peaks_workspace"]
            presenter.overlay_peaks_workspaces()
            presenter.view.query_peaks_to_overlay.assert_called_once()
            mock_peaks_presenter.assert_called_once()
            mock_peaks_presenter.overlay_peaksworkspaces.asssert_called_once()
            mock_peaks_presenter.reset_mock()
            presenter.view.query_peaks_to_overlay.reset_mock()

    def test_gui_starts_with_zoom_selected(self):
        SliceViewer(None, model=self.model, view=self.view)

        self.view.data_view.activate_tool.assert_called_once_with(ToolItemText.ZOOM)

    def test_replace_workspace_returns_when_the_workspace_is_not_the_model_workspace(self):
        self.model.workspace_equals.return_value = False
        presenter, _ = _create_presenter(self.model, self.view, mock.MagicMock(), enable_nonortho_axes=False, supports_nonortho=False)
        presenter.update_view = mock.Mock()
        presenter._decide_plot_update_methods = mock.Mock()
        other_workspace = mock.Mock()

        presenter.replace_workspace("other_workspace", other_workspace)

        presenter._decide_plot_update_methods.assert_not_called()
        presenter.update_view.assert_not_called()

    def test_replace_workspace_closes_view_when_model_properties_change(self):
        self.model.workspace_equals.return_value = True
        presenter, _ = _create_presenter(self.model, self.view, mock.MagicMock(), enable_nonortho_axes=False, supports_nonortho=False)
        presenter.refresh_view = mock.Mock()
        presenter._decide_plot_update_methods = mock.Mock()

        workspace = create_mdhistoworkspace_mock()

        # Not equivalent to self.model.get_properties()
        new_model_properties = {
            "workspace_type": "WS_TYPE.MDE",
            "supports_normalise": False,
            "supports_nonorthogonal_axes": False,
            "supports_dynamic_rebinning": False,
            "supports_peaks_overlays": True,
        }

        with patch.object(SliceViewerModel, "get_properties", return_value=new_model_properties):
            presenter.replace_workspace("workspace", workspace)

            self.view.emit_close.assert_called_once()
            presenter._decide_plot_update_methods.assert_not_called()
            presenter.refresh_view.assert_not_called()

    def test_replace_workspace_updates_view(self):
        presenter, _ = _create_presenter(self.model, self.view, mock.MagicMock(), enable_nonortho_axes=False, supports_nonortho=False)
        workspace = create_mdhistoworkspace_mock()
        new_model_properties = self.model.get_properties()

        # Patch get_properties so that the properties of the new model match those of self.model
        with patch.object(SliceViewerModel, "get_properties", return_value=new_model_properties):
            presenter.view.data_view.plot_MDH.assert_called_once()
            presenter.view.data_view.plot_MDH.reset_mock()

            presenter.replace_workspace("workspace", workspace)

            presenter.view.emit_close.assert_not_called()
            presenter.view.delayed_refresh.assert_called_once()  # leave it to QTimer to call refresh_view()

    def test_refresh_view(self):
        presenter, _ = _create_presenter(self.model, self.view, mock.MagicMock(), enable_nonortho_axes=False, supports_nonortho=False)
        presenter.new_plot = mock.Mock()

        presenter.refresh_view()

        # There is a call to setWorkspace from the constructor, so expect two
        calls = [mock.call(self.model.ws), mock.call(self.model.ws)]
        self.view.data_view.image_info_widget.setWorkspace.assert_has_calls(calls)
        self.view.setWindowTitle.assert_called_with(self.model.get_title())
        presenter.new_plot.assert_called_once()

    def test_refresh_queued_flag_reset_upon_refresh_view(self):
        presenter, _ = _create_presenter(self.model, self.view, mock.MagicMock(), enable_nonortho_axes=False, supports_nonortho=False)
        presenter.new_plot = mock.Mock()
        presenter.view.refresh_queued = True

        presenter.refresh_view()
        self.assertFalse(presenter.view.refresh_queued)

    def test_clear_observer_peaks_presenter_not_none(self):
        presenter, _ = _create_presenter(self.model, self.view, mock.MagicMock(), enable_nonortho_axes=False, supports_nonortho=False)
        presenter._peaks_presenter = mock.MagicMock()

        presenter.clear_observer()

        presenter._peaks_presenter.clear_observer.assert_called_once()

    def test_clear_observer_peaks_presenter_is_none(self):
        presenter, _ = _create_presenter(self.model, self.view, mock.MagicMock(), enable_nonortho_axes=False, supports_nonortho=False)
        presenter._peaks_presenter = None

        # Will raise exception if misbehaving.
        presenter.clear_observer()

    def test_delete_workspace(self):
        mock_model = mock.MagicMock()
        mock_view = mock.MagicMock()
        pres = SliceViewer(mock.Mock(), model=mock_model, view=mock_view)
        mock_model.workspace_equals.return_value = True
        mock_model.check_for_removed_original_workspace.return_value = False
        pres.delete_workspace("test_name")
        mock_view.emit_close.assert_called_once()

    def test_workspace_not_deleted_with_different_name(self):
        mock_model = mock.MagicMock()
        mock_view = mock.MagicMock()
        pres = SliceViewer(mock.Mock(), model=mock_model, view=mock_view)
        mock_model.workspace_equals.return_value = False
        mock_model.check_for_removed_original_workspace.return_value = False
        pres.delete_workspace("different_name")
        mock_view.emit_close.assert_not_called()

    def test_delete_original_workspace(self):
        mock_model = mock.MagicMock()
        mock_view = mock.MagicMock()
        pres = SliceViewer(mock.Mock(), model=mock_model, view=mock_view)
        mock_model.workspace_equals.return_value = False
        mock_model.check_for_removed_original_workspace.return_value = True
        pres.delete_workspace("original_name")
        mock_view.emit_close.assert_called_once()

    def test_delete_not_original_workspace(self):
        mock_model = mock.MagicMock()
        mock_view = mock.MagicMock()
        pres = SliceViewer(mock.Mock(), model=mock_model, view=mock_view)
        mock_model.workspace_equals.return_value = False
        mock_model.check_for_removed_original_workspace.return_value = False
        pres.delete_workspace("not_original_name")
        mock_view.emit_close.assert_not_called()

    def test_replace_original_workspace(self):
        mock_model = mock.MagicMock()
        mock_view = mock.MagicMock()
        pres = SliceViewer(mock.Mock(), model=mock_model, view=mock_view)
        mock_model.check_for_removed_original_workspace.return_value = True

        pres._close_view_with_message = mock.Mock()
        pres.replace_workspace("test1", "test2")
        pres._close_view_with_message.assert_called_once()

    def test_replace_workspace_does_nothing_if_workspace_is_unchanged(self):
        mock_model = mock.MagicMock()
        mock_view = mock.MagicMock()
        pres = SliceViewer(mock.Mock(), model=mock_model, view=mock_view)
        # TODO The return value here should be True but there is a bug in the
        # presenter where the condition is always incorrect (see the TODO on
        # replace_workspace in the presenter)
        mock_model.check_for_removed_original_workspace.return_value = False
        mock_model.workspace_equals.return_value = False
        pres._close_view_with_message = mock.Mock()

        pres.replace_workspace(mock.NonCallableMock(), mock.NonCallableMock())

        pres._close_view_with_message.assert_not_called()
        self.assertEqual(mock_model, pres.model)

    def test_replace_workspace_replaces_model(self):
        mock_model = mock.MagicMock()
        mock_view = mock.MagicMock()
        pres = SliceViewer(mock.Mock(), model=mock_model, view=mock_view)
        mock_model.check_for_removed_original_workspace.return_value = False
        mock_model.workspace_equals.return_value = True
        with mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceViewerModel") as mock_model_class:
            pres.replace_workspace(mock.NonCallableMock(), mock.NonCallableMock())
            self.assertEqual(mock_model_class.return_value, pres.model)

    def test_rename_workspace(self):
        mock_model = mock.MagicMock()
        mock_view = mock.MagicMock()
        pres = SliceViewer(mock.Mock(), model=mock_model, view=mock_view)
        mock_model.workspace_equals.return_value = True
        pres.rename_workspace("old_name", "new_name")
        mock_model.set_ws_name.assert_called_with("new_name")
        mock_view.emit_rename.assert_called_once_with(mock_model.get_title.return_value)

    def test_rename_workspace_not_renamed_with_different_name(self):
        mock_model = mock.MagicMock()
        mock_view = mock.MagicMock()
        pres = SliceViewer(mock.Mock(), model=mock_model, view=mock_view)
        mock_model.workspace_equals.return_value = False
        pres.rename_workspace("old_name", "new_name")
        mock_model.set_ws_name.assert_not_called()
        mock_view.emit_rename.assert_not_called()

    def test_clear_ADS(self):
        mock_view = mock.MagicMock()
        pres = SliceViewer(mock.Mock(), model=mock.MagicMock(), view=mock_view)
        pres.ADS_cleared()
        mock_view.emit_close.assert_called_once()

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceInfo")
    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.PeaksViewerCollectionPresenter", spec=PeaksViewerCollectionPresenter)
    def test_peak_add_delete_event(self, mock_peaks_presenter, mock_sliceinfo_cls):
        mock_sliceinfo_cls().inverse_transform = mock.Mock(side_effect=lambda pos: pos[::-1])
        mock_sliceinfo_cls().z_value = 3

        presenter, _ = _create_presenter(self.model, self.view, mock_sliceinfo_cls, enable_nonortho_axes=False, supports_nonortho=True)
        presenter._peaks_presenter = mock_peaks_presenter

        event = mock.Mock()
        event.inaxes = True
        event.xdata = 1.0
        event.ydata = 2.0

        presenter.canvas_clicked(event)

        mock_sliceinfo_cls.get_sliceinfo.assert_not_called()

        mock_peaks_presenter.add_delete_peak.assert_called_once_with([3, 2, 1])
        self.view.data_view.canvas.draw_idle.assert_called_once()

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceViewer.get_frame")
    def test_workspace_requests_hkl_for_image_info(self, mock_get_frame):
        self.patched_deps["WorkspaceInfo"].display_indices.return_value = (0, 1)

        def run_requests_hkl_test(pres, ncols, callcount):
            mock_get_frame.return_value = SpecialCoordinateSystem.HKL
            self.model.get_hkl_from_full_point.reset_mock()
            extra_cols = pres.get_extra_image_info_columns(1.0, 2.0)
            self.assertEqual(self.model.get_hkl_from_full_point.call_count, callcount)
            self.assertEqual(len(extra_cols), ncols)

        pres3D = SliceViewer(mock.Mock(), model=self.model, view=self.view)
        run_requests_hkl_test(pres3D, 3, 1)
        pres3D_no_q_dim = SliceViewer(mock.Mock(), model=self.model, view=self.view3D_non_QDim)
        run_requests_hkl_test(pres3D_no_q_dim, 0, 0)
        pres4D = SliceViewer(mock.Mock(), model=self.model, view=self.view4D)
        run_requests_hkl_test(pres4D, 3, 1)

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceViewer.get_frame")
    def test_get_extra_image_info_columns_for_less_than_three_qdims(self, mock_get_frame):
        mock_get_frame.return_value = SpecialCoordinateSystem.HKL
        pres3D_no_q_dim = SliceViewer(mock.Mock(), model=self.model, view=self.view3D_non_QDim)

        hkl = pres3D_no_q_dim.get_extra_image_info_columns(1.0, 2.0)

        self.assertEqual({}, hkl)

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceViewer.get_frame")
    def test_get_extra_image_info_columns_for_double_max(self, mock_get_frame):
        mock_get_frame.return_value = SpecialCoordinateSystem.HKL
        pres3D = SliceViewer(mock.Mock(), model=self.model, view=self.view)

        hkl = pres3D.get_extra_image_info_columns(DBLMAX, DBLMAX)

        self.assertEqual({"H": "-", "K": "-", "L": "-"}, hkl)

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceViewer.get_frame")
    def test_get_extra_image_info_columns_returns_values_with_the_expected_format(self, mock_get_frame):
        self.patched_deps["WorkspaceInfo"].display_indices.return_value = (0, 1)
        mock_get_frame.return_value = SpecialCoordinateSystem.HKL
        self.model.get_hkl_from_full_point.return_value = [1.0, 2.0, 3.0]

        pres3D = SliceViewer(mock.Mock(), model=self.model, view=self.view)

        hkl = pres3D.get_extra_image_info_columns(1.0, 2.0)

        self.assertEqual({"H": "1.0000", "K": "2.0000", "L": "3.0000"}, hkl)

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceViewXAxisEditor")
    def test_x_axes_editor_is_opened_on_x_axes_double_click(self, mock_xaxis_editor):
        presenter = SliceViewer(None, model=self.model, view=self.view)
        event = self._double_left_click_event()
        self._setup_data_view_for_axes_double_click("x", tick_label=False)

        presenter.canvas_clicked(event)

        mock_xaxis_editor.assert_called_once()

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceViewYAxisEditor")
    def test_y_axes_editor_is_opened_on_y_axes_double_click(self, mock_yaxis_editor):
        presenter = SliceViewer(None, model=self.model, view=self.view)
        event = self._double_left_click_event()
        self._setup_data_view_for_axes_double_click("y", tick_label=False)

        presenter.canvas_clicked(event)

        mock_yaxis_editor.assert_called_once()

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceViewXAxisEditor")
    def test_x_axes_editor_is_opened_on_x_axes_tick_label_double_click(self, mock_xaxis_editor):
        presenter = SliceViewer(None, model=self.model, view=self.view)
        event = self._double_left_click_event()
        self._setup_data_view_for_axes_double_click("x", tick_label=True)

        presenter.canvas_clicked(event)

        mock_xaxis_editor.assert_called_once()

    @mock.patch("mantidqt.widgets.sliceviewer.presenters.presenter.SliceViewYAxisEditor")
    def test_y_axes_editor_is_opened_on_y_axes_tick_label_double_click(self, mock_yaxis_editor):
        presenter = SliceViewer(None, model=self.model, view=self.view)
        event = self._double_left_click_event()
        self._setup_data_view_for_axes_double_click("y", tick_label=True)

        presenter.canvas_clicked(event)

        mock_yaxis_editor.assert_called_once()

    def _setup_data_view_for_axes_double_click(self, orientation: str, tick_label: bool):
        self.view.data_view.nonorthogonal_mode = False
        self.view.data_view.ax = mock.MagicMock()
        is_x = orientation == "x"
        is_y = orientation == "y"
        if tick_label:
            tick_label = mock.MagicMock()
            tick_label.contains.return_value = (True, {})
            if is_x:
                self.view.data_view.ax.get_xticklabels.return_value = [tick_label]
            elif is_y:
                self.view.data_view.ax.get_yticklabels.return_value = [tick_label]

            self.view.data_view.ax.xaxis.contains.return_value = (False, {})
            self.view.data_view.ax.yaxis.contains.return_value = (False, {})
        else:
            self.view.data_view.ax.xaxis.contains.return_value = (is_x, {})
            self.view.data_view.ax.yaxis.contains.return_value = (is_y, {})

        self.view.data_view.canvas = mock.MagicMock()
        self.view.data_view.canvas.buttond.get.return_value = 1

    def _double_left_click_event(self):
        event = mock.MagicMock()
        event.dblclick = True
        event.button = 1
        return event

    def test_x_axes_editor_calls_dimensions_changed(self):
        presenter = SliceViewer(None, model=self.model, view=self.view)
        presenter.dimensions_changed = mock.MagicMock()

        mock_axes = mock.MagicMock()
        mock_axes.get_xlim = mock.MagicMock(return_value=[0, 1])
        xaxes_edit = SliceViewXAxisEditor(canvas=mock.MagicMock(), axes=mock_axes, dimensions_changed=presenter.dimensions_changed)

        xaxes_edit.on_ok()

        presenter.dimensions_changed.assert_called_once()

    def test_y_axes_editor_calls_dimensions_changed(self):
        presenter = SliceViewer(None, model=self.model, view=self.view)
        presenter.dimensions_changed = mock.MagicMock()

        mock_axes = mock.MagicMock()
        mock_axes.get_ylim = mock.MagicMock(return_value=[0, 1])
        yaxes_edit = SliceViewYAxisEditor(canvas=mock.MagicMock(), axes=mock_axes, dimensions_changed=presenter.dimensions_changed)

        yaxes_edit.on_ok()

        presenter.dimensions_changed.assert_called_once()


if __name__ == "__main__":
    unittest.main()
