# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from unittest.mock import call, patch, MagicMock
from mantidqt.widgets.sliceviewer.presenters.base_presenter import SliceViewerBasePresenter
from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText


class SliceViewerBasePresenterShim(SliceViewerBasePresenter):
    """Shim class to implement the abstract methods to allow us to test the base class"""

    def nonorthogonal_axes(self, state: bool) -> None:
        pass

    def new_plot(self, *args, **kwargs):
        pass

    def dimensions_changed(self) -> None:
        pass

    def slicepoint_changed(self) -> None:
        pass

    def canvas_clicked(self, event) -> None:
        pass

    def key_pressed(self, event) -> None:
        pass

    def mouse_moved(self, event) -> None:
        pass

    def zoom_pan_clicked(self, active) -> None:
        pass

    def get_extra_image_info_columns(self):
        return {}

    def is_integer_frame(self):
        return False, False

    def apply_masking_clicked(self):
        pass

    def elli_masking_clicked(self, active):
        pass

    def export_masking_clicked(self):
        pass

    def poly_masking_clicked(self, active):
        pass

    def rect_masking_clicked(self, active):
        pass

    def invert_masking_clicked(self, active):
        pass


class SliceViewerBasePresenterTest(unittest.TestCase):
    def setUp(self):
        self._ws_info_patcher = mock.patch.multiple(
            "mantidqt.widgets.sliceviewer.presenters.base_presenter", Dimensions=mock.DEFAULT, WorkspaceInfo=mock.DEFAULT
        )
        self.patched_deps = self._ws_info_patcher.start()

    def tearDown(self) -> None:
        self._ws_info_patcher.stop()

    @patch("mantidqt.widgets.sliceviewer.presenters.base_presenter.Masking")
    def test_activate_masking(self, mock_masking_cls):
        presenter = SliceViewerBasePresenterShim(None, model=mock.Mock(), data_view=mock.Mock())
        presenter._data_view = MagicMock()
        presenter.model = MagicMock()
        presenter.model.ws.name.return_value = "test_ws"
        mock_masking = MagicMock()
        mock_masking_cls.return_value = mock_masking

        presenter._activate_masking()
        presenter._data_view.deactivate_and_disable_tool.assert_has_calls(
            [
                call(ToolItemText.ZOOM),
                call(ToolItemText.PAN),
                call(ToolItemText.REGIONSELECTION),
            ]
        )
        mock_masking_cls.assert_called_once_with(
            presenter._data_view,
            "test_ws",
            auto_update_mask_file=False,
        )
        self.assertEqual(presenter._data_view.masking, mock_masking)
        mock_masking.new_selector.assert_called_once_with(ToolItemText.RECT_MASKING)
        presenter._data_view.activate_tool.assert_called_once_with(
            ToolItemText.RECT_MASKING,
            True,
        )

    def test_data_limits_changed_creates_new_plot_if_dynamic_rebinning_supported(self):
        presenter = SliceViewerBasePresenterShim(None, model=mock.Mock(), data_view=mock.Mock())
        self.patched_deps["WorkspaceInfo"].can_support_dynamic_rebinning.return_value = True
        new_plot_mock = mock.MagicMock()
        presenter.new_plot = new_plot_mock

        presenter.data_limits_changed()

        new_plot_mock.assert_called_once()

    def test_data_limits_changed_does_not_create_new_plot_if_dynamic_rebinning_not_supported(self):
        presenter = SliceViewerBasePresenterShim(None, model=mock.Mock(), data_view=mock.Mock())
        self.patched_deps["WorkspaceInfo"].can_support_dynamic_rebinning.return_value = False
        new_plot_mock = mock.MagicMock()
        presenter.new_plot = new_plot_mock

        presenter.data_limits_changed()

        new_plot_mock.assert_not_called()

    def test_set_axes_limits_transforms_all_four_corners_for_nonorthogonal_view(self):
        data_view_mock = mock.Mock()
        data_view_mock.nonorthogonal_mode = True

        class Transform:
            def tr(self, x, y):
                return x + 2 * y, 3 * y

        data_view_mock.nonortho_transform = Transform()
        data_view_mock.set_axes_limits = mock.Mock()
        presenter = SliceViewerBasePresenterShim(None, model=mock.Mock(), data_view=data_view_mock)
        presenter.new_plot = mock.Mock()

        presenter.set_axes_limits((-1, 1), (-2, 2))

        data_view_mock.set_axes_limits.assert_called_once_with((-5.0, 5.0), (-6.0, 6.0))
        presenter.new_plot.assert_called_once()

    def test_request_to_show_all_data_sets_correct_limits_on_view_MD(self):
        model_mock = mock.Mock()
        data_view_mock = mock.Mock()
        presenter = SliceViewerBasePresenterShim(None, model=model_mock, data_view=data_view_mock)
        self.patched_deps["WorkspaceInfo"].is_ragged_matrix_workspace.return_value = False
        dimensions = self.patched_deps["Dimensions"]
        dimensions.get_dim_limits.return_value = ((-1, 1), (-2, 2))
        presenter.set_axes_limits = mock.Mock()

        presenter.show_all_data_clicked()

        dimensions.get_dim_limits.assert_called_once_with(
            model_mock.ws, data_view_mock.dimensions.get_slicepoint(), data_view_mock.dimensions.transpose
        )
        data_view_mock.get_full_extent.assert_not_called()
        presenter.set_axes_limits.assert_called_once_with((-1, 1), (-2, 2))

    def test_request_to_show_all_data_sets_correct_limits_on_view_ragged_matrix(self):
        model_mock = mock.Mock()
        data_view_mock = mock.Mock()
        presenter = SliceViewerBasePresenterShim(None, model=model_mock, data_view=data_view_mock)
        self.patched_deps["WorkspaceInfo"].is_ragged_matrix_workspace.return_value = True
        data_view_mock.get_full_extent.return_value = [-1, 1, -2, 2]
        presenter.set_axes_limits = mock.Mock()

        presenter.show_all_data_clicked()

        dimensions = self.patched_deps["Dimensions"]
        dimensions.get_dim_limits.assert_not_called()
        presenter.set_axes_limits.assert_called_once_with((-1, 1), (-2, 2))


if __name__ == "__main__":
    unittest.main()
