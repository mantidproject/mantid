# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_presenter import ShowSamplePresenter

dir_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_presenter"


class TestShowSamplePresenter(unittest.TestCase):
    def setUp(self):
        self.model = MagicMock()
        self.view = MagicMock()
        self.include_gv = True
        self.presenter = ShowSamplePresenter(self.model, self.view, self.include_gv)

    def test_init_connects_signal(self):
        self.view.sig_view_shape_requested.connect.assert_called_once_with(self.presenter._on_view_shape_clicked)

    @patch(dir_path + ".sample_shape.plot_sample_container_and_components")
    @patch(dir_path + ".output_settings.get_texture_axes_transform")
    def test_on_view_shape_clicked_calls_plotting(self, mock_get_axes, mock_plot_shape):
        fig_mock = MagicMock()
        mock_plot_shape.return_value = fig_mock
        ax_transform = MagicMock()
        ax_labels = MagicMock()
        mock_get_axes.return_value = (ax_transform, ax_labels)

        self.presenter._on_view_shape_clicked("ws")

        mock_plot_shape.assert_called_once_with("ws", alpha=0.5, custom_color="grey")
        self.model.plot_sample_directions.assert_called_once_with(fig_mock, "ws", ax_transform, ax_labels, True)
        self.model.plot_gauge_vol.assert_called_once_with(self.view.get_shape_method(), self.view.get_custom_shape(), fig_mock)

    @patch(dir_path + ".sample_shape.plot_sample_container_and_components")
    @patch(dir_path + ".output_settings.get_texture_axes_transform")
    def test_on_view_reference_shape_clicked(self, mock_get_axes, mock_plot_shape):
        fig_mock = MagicMock()
        mock_plot_shape.return_value = fig_mock
        self.model.reference_ws = "ref_ws"
        mock_get_axes.return_value = (MagicMock(), MagicMock())

        self.presenter.on_view_reference_shape_clicked()

        mock_plot_shape.assert_called_once_with("ref_ws", alpha=0.5, custom_color="grey")
        self.model.plot_sample_directions.assert_called_once()
        self.model.plot_gauge_vol.assert_called_once()

    @patch(dir_path + ".logger")
    @patch(dir_path + ".sample_shape.plot_sample_container_and_components", side_effect=Exception("Oops"))
    def test_on_view_shape_logs_warning_on_exception(self, mock_plot, mock_logger):
        self.presenter._on_view_shape_clicked("bad_ws")

        mock_logger.warning.assert_called_once()
        warning_msg = mock_logger.warning.call_args[0][0]
        self.assertIn("Could not show sample shape", warning_msg)
        self.assertIn("bad_ws", warning_msg)


if __name__ == "__main__":
    unittest.main()
