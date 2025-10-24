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
        self.view.set_on_view_shape_requested.assert_called_once_with(self.presenter._on_view_shape_clicked)

    @patch(dir_path + ".ShowSamplePresenter._view_shape")
    def test_on_view_shape_clicked(self, mock_view_shape):
        ws_name = "test"
        self.presenter._on_view_shape_clicked(ws_name)
        mock_view_shape.assert_called_with(ws_name, True)

    @patch(dir_path + ".ShowSamplePresenter._view_shape")
    def test_on_view_reference_shape_clicked(self, mock_view_shape):
        self.model.get_reference_ws.return_value = "ref_ws"
        self.presenter.on_view_reference_shape_clicked()
        mock_view_shape.assert_called_with("ref_ws", False)

    @patch(dir_path + ".output_settings")
    @patch(dir_path + ".ShowSamplePresenter._set_gauge_vol_str")
    def test_view_shape(self, mock_set_gv, mock_settings):
        ws_name, fix_axes = "ws", True
        mock_settings.get_texture_axes_transform.return_value = None, ("a", "b", "c")
        self.presenter.sample_model = MagicMock()
        self.presenter._view_shape(ws_name, fix_axes)

        self.presenter.sample_model.set_ws_name.assert_called_with(ws_name)
        self.presenter.sample_model.set_fix_axes_to_sample.assert_called_with(fix_axes)
        mock_set_gv.assert_called_once()
        self.presenter.sample_model.show_shape_plot.assert_called_with(None, ("a", "b", "c"))

    def test_set_gauge_vol_str(self):
        self.presenter.include_gauge_volume = True
        gv_string = "gv"
        # set up mocks
        self.presenter.sample_model = MagicMock()
        shape_method, custom_shape = MagicMock(), MagicMock()
        self.view.get_shape_method.return_value = shape_method
        self.view.get_custom_shape.return_value = custom_shape
        self.model.get_gauge_vol_str.return_value = gv_string

        # run
        self.presenter._set_gauge_vol_str()

        self.model.get_gauge_vol_str.assert_called_with(shape_method, custom_shape)
        self.presenter.sample_model.set_gauge_vol_str.assert_called_with(gv_string)


if __name__ == "__main__":
    unittest.main()
