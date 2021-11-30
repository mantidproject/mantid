# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model import PlottingCanvasModel


class PlottingCanvasModelTest(unittest.TestCase):

    def setUp(self):
        self.util = mock.Mock()
        self.model = PlottingCanvasModel(self.util)

    def test_create_workspace_plot_information_calls_get_plot_axis_correctly(self):
        self.util._get_workspace_plot_axis = mock.MagicMock(return_value=1)
        self.util._is_guess_workspace = mock.MagicMock(return_value=True)
        test_ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; fwd"]
        test_axis = [0, 0]
        self.model._axes_workspace_map = 0
        self.model.create_workspace_plot_information(test_ws_names, test_axis, errors=False)

        self.util._get_workspace_plot_axis.assert_any_call(test_ws_names[0], 0)
        self.util._get_workspace_plot_axis.assert_any_call(test_ws_names[1], 0)

    def test_create_workspace_plot_information_calls_create_plot_information_correctly(self):
        self.model._is_tiled = True
        axis = 2
        self.util._get_workspace_plot_axis = mock.MagicMock(return_value=2)
        self.model.create_plot_information = mock.MagicMock()
        self.util._is_guess_workspace = mock.MagicMock(return_value=False)
        test_ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; fwd"]
        test_indies = [0, 0]
        self.model.create_workspace_plot_information(test_ws_names, test_indies, errors=False)

        self.model.create_plot_information.assert_any_call(test_ws_names[0], test_indies[0], axis, False)
        self.model.create_plot_information.assert_any_call(test_ws_names[1], test_indies[1], axis, False)

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model.retrieve_ws')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model.run_convert_to_points')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model.run_convert_to_histogram')
    def test_get_shade_lines_hist_data(self, hist_mock, point_mock,ws_mock):
        ws = mock.Mock()
        ws.isHistogramData = mock.Mock(return_value=True)
        ws_mock.return_value = ws
        name = "unit"
        axis = 2
        point_mock.return_value = "test"
        self.model._plot_model.get_shade_lines = mock.Mock(return_value=(1,2,3))

        self.assertEqual(self.model.get_shade_lines(name, axis), (1,2,3))
        point_mock.assert_called_once_with(name)
        ws_mock.assert_any_call("test")
        self.assertEqual(ws_mock.call_count, 2)
        self.model._plot_model.get_shade_lines.assert_called_once_with(ws, axis)
        hist_mock.assert_called_once_with("unit")

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model.retrieve_ws')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model.run_convert_to_points')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model.run_convert_to_histogram')
    def test_get_shade_lines_points_data(self, hist_mock, point_mock,ws_mock):
        ws = mock.Mock()
        ws.isHistogramData = mock.Mock(return_value=False)
        ws_mock.return_value = ws
        name = "unit"
        axis = 2
        point_mock.return_value = "test"
        self.model._plot_model.get_shade_lines = mock.Mock(return_value=(1,2,3))

        self.assertEqual(self.model.get_shade_lines(name, axis), (1,2,3))
        point_mock.assert_not_called()
        ws_mock.assert_any_call("unit")
        self.assertEqual(ws_mock.call_count, 1)
        self.model._plot_model.get_shade_lines.assert_called_once_with(ws, axis)
        hist_mock.assert_not_called()


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
