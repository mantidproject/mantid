# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import AnalysisDataService, FrameworkManager
from mantid.simpleapi import CreateSampleWorkspace

from Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel
from Muon.GUI.Common.corrections_tab_widget.background_corrections_model import (BackgroundCorrectionsModel,
                                                                                 DEFAULT_X_LOWER, DEFAULT_X_UPPER)
from Muon.GUI.Common.test_helpers.context_setup import setup_context


class BackgroundCorrectionsModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        context = setup_context()
        self.corrections_model = CorrectionsModel(context.data_context, context.corrections_context)
        self.model = BackgroundCorrectionsModel(self.corrections_model, context)
        self.model.clear_background_corrections_data()

        self.runs = ["84447", "84447", "84447", "84447"]
        self.groups = ["fwd", "bwd", "top", "bottom"]
        self.start_xs = [15.0] * 4
        self.end_xs = [30.0] * 4
        self.a0s = [0.0] * 4
        self.a0_errors = [0.0] * 4

    def tearDown(self):
        self.model = None
        AnalysisDataService.clear()

    def test_that_the_model_and_context_has_been_initialized_with_the_expected_data(self):
        self.assertTrue(self.model.is_background_mode_none())
        self.assertEqual(self.model._corrections_context.selected_function, "Flat Background")
        self.assertEqual(self.model._corrections_context.selected_group, "All")
        self.assertEqual(self.model._corrections_context.show_all_runs, False)

    def test_that_set_background_correction_mode_will_set_the_background_mode_as_expected(self):
        self.model.set_background_correction_mode("Auto")
        self.assertTrue(not self.model.is_background_mode_none())

    def test_that_set_selected_function_will_set_the_selected_function_in_the_context(self):
        function = "Flat Background + Exp Decay"

        self.model.set_selected_function(function)

        self.assertEqual(self.model._corrections_context.selected_function, function)

    def test_that_set_selected_group_will_set_the_selected_group_in_the_context(self):
        group = "fwd"

        self.model.set_selected_group(group)

        self.assertEqual(self.model._corrections_context.selected_group, group)

    def test_that_set_show_all_runs_will_set_show_all_runs_to_true(self):
        show_all_runs = True

        self.model.set_show_all_runs(show_all_runs)

        self.assertEqual(self.model._corrections_context.show_all_runs, show_all_runs)

    def test_that_is_equal_to_n_decimals_will_return_false_if_the_two_numbers_are_not_equal_to_the_given_dp(self):
        value1, value2 = 1.001, 1.0015
        self.assertTrue(not self.model.is_equal_to_n_decimals(value1, value2, 3))

    def test_that_is_equal_to_n_decimals_will_return_true_if_the_two_numbers_are_equal_to_the_given_dp(self):
        value1, value2 = 1.001, 1.0014
        self.assertTrue(self.model.is_equal_to_n_decimals(value1, value2, 3))

    def test_that_populate_background_corrections_data_will_populate_default_background_correction_data(self):
        self.model.x_limits_of_workspace = mock.Mock(return_value=(0.0, 30.0))
        self._populate_background_corrections_data()

        for run, group in zip(self.runs, self.groups):
            correction_data = self.model._corrections_context.background_correction_data[tuple([run, group])]
            self.assertEqual(correction_data.start_x, 15.0)
            self.assertEqual(correction_data.end_x, 30.0)
            self.assertEqual(correction_data.flat_background.getParameterValue("A0"), 0.0)
            self.assertEqual(correction_data.flat_background.getError("A0"), 0.0)

    def test_that_set_start_x_will_set_the_start_x_in_the_background_correction_data_for_a_specific_domain(self):
        run = "84447"

        self._populate_background_corrections_data()

        self.model.set_start_x(run, "fwd", 5.0)
        self.model.set_start_x(run, "bwd", 6.0)
        self.model.set_start_x(run, "top", 7.0)
        self.model.set_start_x(run, "bottom", 8.0)

        self.assertEqual(self.model.start_x(run, "fwd"), 5.0)
        self.assertEqual(self.model.start_x(run, "bwd"), 6.0)
        self.assertEqual(self.model.start_x(run, "top"), 7.0)
        self.assertEqual(self.model.start_x(run, "bottom"), 8.0)

    def test_that_set_end_x_will_set_the_end_x_in_the_background_correction_data_for_a_specific_domain(self):
        run = "84447"

        self._populate_background_corrections_data()

        self.model.set_end_x(run, "fwd", 5.0)
        self.model.set_end_x(run, "bwd", 6.0)
        self.model.set_end_x(run, "top", 7.0)
        self.model.set_end_x(run, "bottom", 8.0)

        self.assertEqual(self.model.end_x(run, "fwd"), 5.0)
        self.assertEqual(self.model.end_x(run, "bwd"), 6.0)
        self.assertEqual(self.model.end_x(run, "top"), 7.0)
        self.assertEqual(self.model.end_x(run, "bottom"), 8.0)

    def test_that_selected_correction_data_returns_all_correction_data_if_all_runs_and_groups_are_selected(self):
        self.model.x_limits_of_workspace = mock.Mock(return_value=(0.0, 30.0))
        self.model.set_show_all_runs(True)

        self._populate_background_corrections_data()
        runs, groups, start_xs, end_xs, a0s, a0_errors = self.model.selected_correction_data()

        self.assertEqual(runs, self.runs)
        self.assertEqual(groups, self.groups)
        self.assertEqual(start_xs, self.start_xs)
        self.assertEqual(end_xs, self.end_xs)
        self.assertEqual(a0s, self.a0s)
        self.assertEqual(a0_errors, self.a0_errors)

    def test_that_selected_correction_data_returns_all_correction_data_for_a_specific_run_and_group(self):
        run = "84447"
        group = "bwd"
        self.model.x_limits_of_workspace = mock.Mock(return_value=(0.0, 30.0))
        self.model.set_selected_group(group)
        self.corrections_model.set_current_run_string(run)

        self._populate_background_corrections_data()
        runs, groups, start_xs, end_xs, a0s, a0_errors = self.model.selected_correction_data()

        self.assertEqual(runs, [run])
        self.assertEqual(groups, [group])
        self.assertEqual(start_xs, [15.0])
        self.assertEqual(end_xs, [30.0])
        self.assertEqual(a0s, [0.0])
        self.assertEqual(a0_errors, [0.0])

    def test_that_x_limits_of_workspace_will_return_the_x_limits_of_the_workspace(self):
        run, group = "84447", "top"
        workspace_name = f"HIFI{run}; Group; {group}; Counts; MA"
        self.model._get_counts_workspace_name = mock.Mock(return_value=workspace_name)

        CreateSampleWorkspace(OutputWorkspace=workspace_name)

        x_lower, x_upper = self.model.x_limits_of_workspace(run, group)

        self.assertEqual(x_lower, 0.0)
        self.assertEqual(x_upper, 20000.0)

    def test_that_x_limits_of_workspace_will_return_the_default_x_values_if_there_are_no_workspaces_loaded(self):
        run, group = "84447", "top"
        workspace_name = f"HIFI{run}; Group; {group}; Counts; MA"
        self.model._get_counts_workspace_name = mock.Mock(return_value=workspace_name)

        x_lower, x_upper = self.model.x_limits_of_workspace(run, group)

        self.assertEqual(x_lower, DEFAULT_X_LOWER)
        self.assertEqual(x_upper, DEFAULT_X_UPPER)

    def _populate_background_corrections_data(self):
        self.corrections_model.run_number_strings = mock.Mock(return_value=self.runs)
        self.mock_group_names = mock.PropertyMock(return_value=self.groups)
        type(self.model._context.group_pair_context).group_names = self.mock_group_names

        self.model.populate_background_corrections_data()


if __name__ == '__main__':
    unittest.main()
