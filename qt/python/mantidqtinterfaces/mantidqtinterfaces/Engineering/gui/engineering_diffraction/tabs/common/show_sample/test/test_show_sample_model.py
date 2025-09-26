# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright © 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock, call
import numpy as np

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_model import ShowSampleModel

model_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_model"


class TestShowSampleModel_Setters(unittest.TestCase):
    def setUp(self):
        self.model = ShowSampleModel()

    def test_basic_setters_assign_values(self):
        self.model.set_ws_name("ws1")
        self.model.set_fix_axes_to_sample(False)
        self.model.set_include_gauge_vol(True)
        self.model.set_gauge_vol_str("<xml/>")

        self.assertEqual(self.model.ws_name, "ws1")
        self.assertFalse(self.model.fix_axes_to_sample)
        self.assertTrue(self.model.include_gauge_vol)
        self.assertEqual(self.model.gauge_vol_str, "<xml/>")


class TestShowSampleModel_ShowShapePlot(unittest.TestCase):
    def setUp(self):
        self.model = ShowSampleModel()
        self.model.set_ws_name("my_ws")

    @patch(model_path + ".logger")
    @patch(model_path + ".sample_shape.plot_sample_container_and_components")
    def test_show_shape_plot_calls_plot_sample_directions_only(self, mock_plot_shape, mock_logger):
        fig = MagicMock()
        mock_plot_shape.return_value = fig

        # spy on the instance method to assert the call
        self.model.plot_sample_directions = MagicMock()
        self.model.include_gauge_vol = False  # default path

        ax_transform = np.eye(3)
        ax_labels = ["RD", "ND", "TD"]

        self.model.show_shape_plot(ax_transform, ax_labels)

        # figure stored
        self.assertIs(self.model.fig, fig)

        # called with instance's fix_axes_to_sample flag
        self.model.plot_sample_directions.assert_called_once_with(ax_transform, ax_labels, True)
        mock_plot_shape.assert_called_once_with("my_ws", alpha=0.5, custom_color="grey")
        mock_logger.warning.assert_not_called()

    @patch(model_path + ".logger")
    @patch(model_path + ".sample_shape.plot_sample_container_and_components")
    def test_show_shape_plot_calls_plot_gauge_vol_when_enabled(self, mock_plot_shape, mock_logger):
        fig = MagicMock()
        mock_plot_shape.return_value = fig
        self.model.plot_sample_directions = MagicMock()
        self.model.plot_gauge_vol = MagicMock()
        self.model.include_gauge_vol = True

        self.model.show_shape_plot(np.eye(3), ["RD", "ND", "TD"])

        self.model.plot_sample_directions.assert_called_once()
        self.model.plot_gauge_vol.assert_called_once()
        mock_logger.warning.assert_not_called()

    @patch(model_path + ".logger")
    @patch(model_path + ".sample_shape.plot_sample_container_and_components")
    def test_show_shape_plot_logs_warning_on_exception(self, mock_plot_shape, mock_logger):
        mock_plot_shape.side_effect = Exception("boom")
        self.model.show_shape_plot(np.eye(3), ["RD", "ND", "TD"])
        mock_logger.warning.assert_called_once()
        self.assertIn("Could not show sample shape for workspace 'my_ws'", mock_logger.warning.call_args[0][0])
        self.assertIn("boom", mock_logger.warning.call_args[0][0])


class TestShowSampleModel_PlotSampleDirections(unittest.TestCase):
    def setUp(self):
        self.model = ShowSampleModel()
        self.model.set_ws_name("wsX")
        # fake fig/axes for quiver/text calls
        ax = MagicMock()
        fig = MagicMock()
        fig.axes = [ax]
        self.model.fig = fig
        self.ax = ax

    @patch(model_path + ".get_scaled_intrinsic_sample_directions_in_lab_frame")
    @patch(model_path + ".ADS")
    def test_plot_sample_directions_uses_goniometer_when_fixed(self, mock_ads, mock_get_dirs):
        # workspace chain
        ws = MagicMock()
        mock_ads.retrieve.return_value = ws
        run = MagicMock()
        gonio = MagicMock()
        ws.getRun.return_value = run
        run.getGoniometer.return_value = gonio
        R = np.array([[2.0, 0.0, 0.0], [0.0, 2.0, 0.0], [0.0, 0.0, 2.0]])
        gonio.getR.return_value = R

        sample = MagicMock()
        shape = MagicMock()
        ws.sample.return_value = sample
        sample.getShape.return_value = shape
        shape.getMesh.return_value = "mesh"

        # directions and arrow lengths
        rd = np.array([1.0, 0.0, 0.0])
        nd = np.array([0.0, 1.0, 0.0])
        td = np.array([0.0, 0.0, 1.0])
        arrow_lens = np.array([2.0, 3.0, 4.0])
        mock_get_dirs.return_value = (rd, nd, td, arrow_lens)

        self.model.plot_sample_directions(np.eye(3), ["RD", "ND", "TD"], fix_axes_to_sample=True)

        # used goniometer rotation
        args, kwargs = mock_get_dirs.call_args
        self.assertTrue(np.all(args[1] == R))  # rotation_matrix argument

        # quiver/text called with scaled vectors
        expected_quiver_calls = [
            call(0, 0, 0, 2.0, 0.0, 0.0, color="red", length=2.0, normalize=True, arrow_length_ratio=0.05),
            call(0, 0, 0, 0.0, 3.0, 0.0, color="green", length=3.0, normalize=True, arrow_length_ratio=0.05),
            call(0, 0, 0, 0.0, 0.0, 4.0, color="blue", length=4.0, normalize=True, arrow_length_ratio=0.05),
        ]
        self.ax.quiver.assert_has_calls(expected_quiver_calls, any_order=False)

        expected_text_calls = [
            call(2.0, 0.0, 0.0, "RD"),
            call(0.0, 3.0, 0.0, "ND"),
            call(0.0, 0.0, 4.0, "TD"),
        ]
        self.ax.text.assert_has_calls(expected_text_calls, any_order=False)

        # run/goniometer must have been accessed
        ws.getRun.assert_called_once()

    @patch(model_path + ".get_scaled_intrinsic_sample_directions_in_lab_frame")
    @patch(model_path + ".ADS")
    def test_plot_sample_directions_uses_identity_when_not_fixed(self, mock_ads, mock_get_dirs):
        # workspace chain (ensure getRun is not called when not fixed)
        ws = MagicMock()
        mock_ads.retrieve.return_value = ws

        sample = MagicMock()
        shape = MagicMock()
        ws.sample.return_value = sample
        sample.getShape.return_value = shape
        shape.getMesh.return_value = "mesh"

        rd = np.array([1.0, 0.0, 0.0])
        nd = np.array([0.0, 1.0, 0.0])
        td = np.array([0.0, 0.0, 1.0])
        arrow_lens = np.array([1.0, 1.0, 1.0])
        mock_get_dirs.return_value = (rd, nd, td, arrow_lens)

        self.model.plot_sample_directions(np.eye(3), ["RD", "ND", "TD"], fix_axes_to_sample=False)

        # rotation matrix argument should be identity
        args, kwargs = mock_get_dirs.call_args
        self.assertTrue(np.all(args[1] == np.eye(3)))
        ws.getRun.assert_not_called()


class TestShowSampleModel_PlotGaugeVol(unittest.TestCase):
    def setUp(self):
        self.model = ShowSampleModel()
        # fake fig.gca for adding collection
        axes = MagicMock()
        fig = MagicMock()
        fig.gca.return_value = axes
        self.model.fig = fig
        self.axes = axes

    @patch(model_path + ".is_valid_mesh")
    @patch(model_path + ".get_xml_mesh")
    def test_plot_gauge_vol_no_string_does_nothing(self, mock_get_mesh, mock_is_valid):
        self.model.gauge_vol_str = ""
        self.model.plot_gauge_vol()
        mock_get_mesh.assert_not_called()
        mock_is_valid.assert_not_called()
        self.axes.add_collection3d.assert_not_called()

    @patch(model_path + ".Poly3DCollection")
    @patch(model_path + ".is_valid_mesh")
    @patch(model_path + ".get_xml_mesh")
    def test_plot_gauge_vol_adds_collection_for_valid_mesh(self, mock_get_mesh, mock_is_valid, mock_poly):
        self.model.gauge_vol_str = "<mesh/>"
        mock_get_mesh.return_value = "mesh_data"
        mock_is_valid.return_value = True

        self.model.plot_gauge_vol()

        mock_get_mesh.assert_called_once_with("<mesh/>")
        mock_is_valid.assert_called_once_with("mesh_data")
        mock_poly.assert_called_once_with("mesh_data", facecolors="cyan", edgecolors="black", linewidths=0.1, alpha=0.25)
        # added to axes
        self.axes.add_collection3d.assert_called_once_with(mock_poly.return_value)

    @patch(model_path + ".Poly3DCollection")
    @patch(model_path + ".is_valid_mesh")
    @patch(model_path + ".get_xml_mesh")
    def test_plot_gauge_vol_skips_when_mesh_invalid(self, mock_get_mesh, mock_is_valid, mock_poly):
        self.model.gauge_vol_str = "<mesh/>"
        mock_get_mesh.return_value = "mesh_data"
        mock_is_valid.return_value = False

        self.model.plot_gauge_vol()

        mock_poly.assert_not_called()
        self.axes.add_collection3d.assert_not_called()


if __name__ == "__main__":
    unittest.main()
