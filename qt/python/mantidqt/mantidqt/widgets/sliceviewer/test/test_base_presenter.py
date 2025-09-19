# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqt.widgets.sliceviewer.presenters.base_presenter import SliceViewerBasePresenter


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


class SliceViewerBasePresenterTest(unittest.TestCase):
    def setUp(self):
        self._ws_info_patcher = mock.patch.multiple(
            "mantidqt.widgets.sliceviewer.presenters.base_presenter", Dimensions=mock.DEFAULT, WorkspaceInfo=mock.DEFAULT
        )
        self.patched_deps = self._ws_info_patcher.start()

    def tearDown(self) -> None:
        self._ws_info_patcher.stop()

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
