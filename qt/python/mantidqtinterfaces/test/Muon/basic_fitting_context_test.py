# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import FunctionFactory, FrameworkManager, WorkspaceFactory
from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.basic_fitting_context import BasicFittingContext
from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.fitting_context import FitInformation
from mantidqtinterfaces.Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper


class BasicFittingContextTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        self.fitting_context = BasicFittingContext()

        self.dataset_names = ["Name1", "Name2"]
        self.fit_function = FunctionFactory.createFunction("FlatBackground")
        self.single_fit_functions = [self.fit_function.clone(), self.fit_function.clone()]

    def test_that_allow_double_pulse_fitting_is_set_to_false_by_default(self):
        self.assertFalse(self.fitting_context.allow_double_pulse_fitting)

        self.fitting_context = BasicFittingContext(allow_double_pulse_fitting=True)
        self.assertTrue(self.fitting_context.allow_double_pulse_fitting)

    def test_that_the_context_has_been_instantiated_with_empty_fit_data(self):
        self.assertEqual(self.fitting_context.dataset_names, [])
        self.assertEqual(self.fitting_context.current_dataset_index, None)
        self.assertEqual(self.fitting_context.start_xs, [])
        self.assertEqual(self.fitting_context.end_xs, [])
        self.assertEqual(self.fitting_context.exclude_range, False)
        self.assertEqual(self.fitting_context.exclude_start_xs, [])
        self.assertEqual(self.fitting_context.exclude_end_xs, [])
        self.assertEqual(self.fitting_context.single_fit_functions, [])
        self.assertEqual(self.fitting_context.dataset_indices_for_undo, [])
        self.assertEqual(self.fitting_context.single_fit_functions_for_undo, [])
        self.assertEqual(self.fitting_context.fit_statuses_for_undo, [])
        self.assertEqual(self.fitting_context.chi_squared_for_undo, [])
        self.assertEqual(self.fitting_context.fit_statuses, [])
        self.assertEqual(self.fitting_context.chi_squared, [])
        self.assertEqual(self.fitting_context.plot_guess, False)
        self.assertEqual(self.fitting_context.plot_guess_type, "x from fit range")
        self.assertEqual(self.fitting_context.plot_guess_points, None)
        self.assertEqual(self.fitting_context.plot_guess_start_x, None)
        self.assertEqual(self.fitting_context.plot_guess_end_x, None)
        self.assertEqual(self.fitting_context.guess_workspace_name, None)
        self.assertEqual(self.fitting_context.function_name, "")
        self.assertTrue(self.fitting_context.function_name_auto_update)
        self.assertEqual(self.fitting_context.minimizer, "")
        self.assertEqual(self.fitting_context.evaluation_type, "")
        self.assertTrue(self.fitting_context.fit_to_raw)

    def test_that_current_dataset_index_will_raise_if_the_index_is_greater_than_or_equal_to_the_number_of_datasets(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(AssertionError):
            self.fitting_context.current_dataset_index = 2

    def test_that_current_dataset_index_does_not_throw_when_provided_None_and_there_are_no_datasets_loaded(self):
        self.fitting_context.current_dataset_index = None

    def test_that_number_of_datasets_returns_the_expected_number_of_datasets(self):
        self.fitting_context.dataset_names = self.dataset_names
        self.assertEqual(self.fitting_context.number_of_datasets, 2)

    def test_that_start_xs_will_raise_if_the_number_of_xs_is_greater_than_or_equal_to_the_number_of_datasets(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(AssertionError):
            self.fitting_context.start_xs = [1.0, 2.0, 3.0]

    def test_that_end_xs_will_raise_if_the_number_of_xs_is_greater_than_or_equal_to_the_number_of_datasets(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(AssertionError):
            self.fitting_context.end_xs = [1.0, 2.0, 3.0]

    def test_that_exclude_start_xs_will_raise_if_the_number_of_xs_is_greater_than_or_equal_to_the_number_of_datasets(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(AssertionError):
            self.fitting_context.exclude_start_xs = [1.0, 2.0, 3.0]

    def test_that_exclude_end_xs_will_raise_if_the_number_of_xs_is_greater_than_or_equal_to_the_number_of_datasets(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(AssertionError):
            self.fitting_context.exclude_end_xs = [1.0, 2.0, 3.0]

    def test_that_single_fit_functions_will_raise_if_the_num_of_funcs_is_greater_than_or_equal_to_the_num_of_datasets(self):
        self.fitting_context.dataset_names = ["Name1"]
        with self.assertRaises(AssertionError):
            self.fitting_context.single_fit_functions = self.single_fit_functions

    def test_that_fit_statuses_will_raise_if_the_number_of_datasets_is_smaller_than_the_provided_list_size(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(AssertionError):
            self.fitting_context.fit_statuses = ["Success", "Success", "Success"]

    def test_that_chi_squared_will_raise_if_the_number_of_datasets_is_smaller_than_the_provided_list_size(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(AssertionError):
            self.fitting_context.chi_squared = [1.0, 2.0, 3.0]

    def test_that_guess_workspace_name_will_not_raise_if_the_workspace_does_not_exist(self):
        self.fitting_context.guess_workspace_name = "FakeName"
        self.assertEqual(self.fitting_context.guess_workspace_name, None)

    def test_that_guess_workspace_name_will_not_raise_if_the_name_provided_is_none(self):
        self.fitting_context.guess_workspace_name = None
        self.assertEqual(self.fitting_context.guess_workspace_name, None)

    def test_that_clear_will_clear_the_undo_data_and_active_fits_list(self):
        self.fitting_context.active_fit_history = [mock.Mock(), mock.Mock()]
        self.fitting_context.dataset_indices_for_undo = [0, 1]
        self.fitting_context.single_fit_functions_for_undo = [mock.Mock(), mock.Mock()]
        self.fitting_context.fit_statuses_for_undo = ["Success", "Fail"]
        self.fitting_context.chi_squared_for_undo = [2.2, 3.3]

        self.fitting_context.clear()

        self.assertEqual(self.fitting_context.active_fit_history, [])
        self.assertEqual(self.fitting_context.all_latest_fits(), [])
        self.assertEqual(self.fitting_context.dataset_indices_for_undo, [])
        self.assertEqual(self.fitting_context.single_fit_functions_for_undo, [])
        self.assertEqual(self.fitting_context.fit_statuses_for_undo, [])
        self.assertEqual(self.fitting_context.chi_squared_for_undo, [])

    def test_that_all_latest_fits_will_return_the_two_most_recent_unique_fits(self):
        output_ws = WorkspaceFactory.create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        table_workspace = WorkspaceFactory.createTable()

        output_ws_wrap1 = StaticWorkspaceWrapper("Output1", output_ws)
        parameter_ws_wrap1 = StaticWorkspaceWrapper("Parameter1", table_workspace)
        covariance_ws_wrap1 = StaticWorkspaceWrapper("Covariance1", table_workspace)
        output_ws_wrap2 = StaticWorkspaceWrapper("Output2", output_ws)
        parameter_ws_wrap2 = StaticWorkspaceWrapper("Parameter2", table_workspace)
        covariance_ws_wrap2 = StaticWorkspaceWrapper("Covariance2", table_workspace)

        fit1 = FitInformation(["Input1"], "GausOsc", [output_ws_wrap1], parameter_ws_wrap1, covariance_ws_wrap1)
        fit2 = FitInformation(["Input2"], "GausOsc", [output_ws_wrap2], parameter_ws_wrap2, covariance_ws_wrap2)
        fit3 = FitInformation(["Input1"], "GausOsc", [output_ws_wrap1], parameter_ws_wrap1, covariance_ws_wrap1)

        self.fitting_context.active_fit_history = [fit1, fit2, fit3]

        self.assertEqual(self.fitting_context.active_fit_history[0], fit1)
        self.assertEqual(self.fitting_context.active_fit_history[1], fit2)
        self.assertEqual(self.fitting_context.active_fit_history[2], fit3)

        # fit3 == fit1 and fit3 is more recent, so 'all_latest_fits' will only return the most recent unique fits.
        self.assertEqual(self.fitting_context.all_latest_fits()[0], fit2)
        self.assertEqual(self.fitting_context.all_latest_fits()[1], fit3)

    def test_remove_workspace_by_name_will_remove_a_fit_containing_a_specific_parameter_workspace(self):
        output_ws = WorkspaceFactory.create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        table_workspace = WorkspaceFactory.createTable()

        output_ws_wrap1 = StaticWorkspaceWrapper("Output1", output_ws)
        parameter_ws_wrap1 = StaticWorkspaceWrapper("Parameter1", table_workspace)
        covariance_ws_wrap1 = StaticWorkspaceWrapper("Covariance1", table_workspace)
        output_ws_wrap2 = StaticWorkspaceWrapper("Output2", output_ws)
        parameter_ws_wrap2 = StaticWorkspaceWrapper("Parameter2", table_workspace)
        covariance_ws_wrap2 = StaticWorkspaceWrapper("Covariance2", table_workspace)

        fit1 = FitInformation(["Input1"], "GausOsc", [output_ws_wrap1], parameter_ws_wrap1, covariance_ws_wrap1)
        fit2 = FitInformation(["Input2"], "GausOsc", [output_ws_wrap2], parameter_ws_wrap2, covariance_ws_wrap2)

        self.fitting_context.active_fit_history = [fit1, fit2]

        self.fitting_context.remove_workspace_by_name("Parameter1")

        self.assertEqual(self.fitting_context.active_fit_history[0], fit2)
        self.assertEqual(self.fitting_context.all_latest_fits()[0], fit2)


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
