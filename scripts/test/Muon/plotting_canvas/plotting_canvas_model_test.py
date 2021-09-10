# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model import PlottingCanvasModel


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


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
