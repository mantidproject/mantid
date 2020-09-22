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
        self.context = mock.Mock()
        self.model = PlottingCanvasModel(self.context)
        self.context.data_context.instrument = "MUSR"

    def test_create_workspace_plot_information_calls_get_plot_axis_correctly(self):
        self.model._get_workspace_plot_axis = mock.MagicMock(return_value=1)
        test_ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; fwd"]
        test_indies = [0, 0]
        self.model.create_workspace_plot_information(test_ws_names, test_indies, errors=False)

        self.model._get_workspace_plot_axis.assert_any_call(test_ws_names[0])
        self.model._get_workspace_plot_axis.assert_any_call(test_ws_names[1])

    def test_create_workspace_plot_information_calls_create_plot_information_correctly(self):
        self.model._is_tiled = True
        axis = 2
        self.model._get_workspace_plot_axis = mock.MagicMock(return_value=2)
        self.model.create_plot_information = mock.MagicMock()
        test_ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; fwd"]
        test_indies = [0, 0]
        self.model.create_workspace_plot_information(test_ws_names, test_indies, errors=False)

        self.model.create_plot_information.assert_any_call(test_ws_names[0], test_indies[0], axis, False)
        self.model.create_plot_information.assert_any_call(test_ws_names[1], test_indies[1], axis, False)

    def test_get_workspace_plot_axis_returns_correct_axis_for_tiled_plot(self):
        self.context.group_pair_context.get_group_pair_name_and_run_from_workspace_name.return_value = ('fwd', '62260')
        self.model._is_tiled = True
        self.model._tiled_by = "Group/Pair"
        self.model._axes_workspace_map = {"bkwd": 0, "fwd": 1, "top": 2}
        test_ws_name = "MUSR62260; Group; fwd"
        expected_axis = 1

        axis = self.model._get_workspace_plot_axis(test_ws_name)

        self.assertEqual(axis, expected_axis)

    def test_get_workspace_plot_axis_handles_bad_input_workspace_name(self):
        self.context.group_pair_context.get_group_pair_name_and_run_from_workspace_name.return_value = ('bottom', '62260')
        self.model._is_tiled = True
        self.model._tiled_by = "Group/Pair"
        self.model._axes_workspace_map = {"bkwd": 0, "fwd": 1, "top": 2}
        test_ws_name = "MUSR62260; Group; bottom"
        expected_axis = 0

        axis = self.model._get_workspace_plot_axis(test_ws_name)

        self.assertEqual(axis, expected_axis)

    def test_create_workspace_label_returns_expected_label_for_run_workspace(self):
        run_workspace_name = 'MUSR62260; Group; bottom; Asymmetry; MA'
        self.model._is_tiled = False
        expected_label = 'MUSR62260;bottom'

        label = self.model._create_workspace_label(run_workspace_name, 0)

        self.assertEqual(label, expected_label)

    def test_create_workspace_label_returns_expected_label_for_fit_workspace(self):
        fit_workspace_name = 'MUSR62260; Group; bottom; Asymmetry; MA; Fitted; GausOsc; Workspace'
        self.model._is_tiled = False
        expected_calc_label = 'MUSR62260;bottom;GausOsc;Calc'
        expected_diff_label = 'MUSR62260;bottom;GausOsc;Diff'

        label_calc = self.model._create_workspace_label(fit_workspace_name, 1)
        label_diff = self.model._create_workspace_label(fit_workspace_name, 2)

        self.assertEqual(label_calc, expected_calc_label)
        self.assertEqual(label_diff, expected_diff_label)

    def test_create_workspace_label_returns_expected_label_tiled_by_group(self):
        run_workspace_name = 'MUSR62260; Group; bottom; Asymmetry; MA'
        self.model._is_tiled = True
        self.model._tiled_by = "Group/Pair"
        expected_label = '62260'

        label = self.model._create_workspace_label(run_workspace_name, 0)

        self.assertEqual(label, expected_label)

    def test_create_workspace_label_returns_expected_label_tiled_by_run(self):
        run_workspace_name = 'MUSR62260; Group; bottom; Asymmetry; MA'
        self.model._is_tiled = True
        self.model._tiled_by = 'Run'
        expected_label = 'bottom'

        label = self.model._create_workspace_label(run_workspace_name, 0)

        self.assertEqual(label, expected_label)

    def test_that_correctly_assigns_groups_to_axis_in_cases_where_group_name_appears_elsewhere_in_workspace_name_as_substring(self):
        self.context.group_pair_context.get_group_pair_name_and_run_from_workspace_name.return_value = ('fwd', '62260')
        self.model._is_tiled = True
        self.model._tiled_by = "Group/Pair"
        self.model._axes_workspace_map = {"up": 0, "fwd": 1, "top": 2}
        test_ws_name = "MUSR62260; Group; fwd"
        expected_axis = 1

        axis = self.model._get_workspace_plot_axis(test_ws_name)

        self.assertEqual(axis, expected_axis)

    def test_that_correctly_assigns_axis_for_tiling_by_run(self):
        self.context.group_pair_context.get_group_pair_name_and_run_from_workspace_name.return_value = ('fwd', '62262')
        self.model._is_tiled = True
        self.model._tiled_by = "Run"
        self.model._axes_workspace_map = {"62260": 0, "62261": 1, "62262": 2}
        test_ws_name = "MUSR62262; Group; fwd"
        expected_axis = 2

        axis = self.model._get_workspace_plot_axis(test_ws_name)

        self.assertEqual(axis, expected_axis)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
