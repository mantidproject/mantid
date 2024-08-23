# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import FunctionFactory, FrameworkManager, WorkspaceFactory
from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.model_fitting_context import ModelFittingContext
from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.fitting_context import FitInformation
from mantidqtinterfaces.Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper


class ModelFittingContextTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        self.fitting_context = ModelFittingContext()

        self.dataset_names = ["Name1", "Name2"]
        self.fit_function = FunctionFactory.createFunction("FlatBackground")
        self.single_fit_functions = [self.fit_function.clone(), self.fit_function.clone()]

    def test_that_allow_double_pulse_fitting_is_set_to_false_by_default(self):
        self.assertFalse(self.fitting_context.allow_double_pulse_fitting)

        self.fitting_context = ModelFittingContext(allow_double_pulse_fitting=True)
        self.assertTrue(self.fitting_context.allow_double_pulse_fitting)

    def test_that_the_context_has_been_instantiated_with_empty_fit_data(self):
        self.assertEqual(self.fitting_context.dataset_names, [])
        self.assertEqual(self.fitting_context.current_dataset_index, None)
        self.assertEqual(self.fitting_context.start_xs, [])
        self.assertEqual(self.fitting_context.end_xs, [])
        self.assertEqual(self.fitting_context.single_fit_functions, [])
        self.assertEqual(self.fitting_context.dataset_indices_for_undo, [])
        self.assertEqual(self.fitting_context.single_fit_functions_for_undo, [])
        self.assertEqual(self.fitting_context.fit_statuses_for_undo, [])
        self.assertEqual(self.fitting_context.chi_squared_for_undo, [])
        self.assertEqual(self.fitting_context.fit_statuses, [])
        self.assertEqual(self.fitting_context.chi_squared, [])
        self.assertEqual(self.fitting_context.plot_guess, False)
        self.assertEqual(self.fitting_context.guess_workspace_name, None)
        self.assertEqual(self.fitting_context.function_name, "")
        self.assertTrue(self.fitting_context.function_name_auto_update)
        self.assertEqual(self.fitting_context.minimizer, "")
        self.assertEqual(self.fitting_context.evaluation_type, "")
        self.assertTrue(self.fitting_context.fit_to_raw)

        self.assertEqual(self.fitting_context.result_table_names, [])
        self.assertEqual(self.fitting_context.current_result_table_index, None)
        self.assertEqual(self.fitting_context.x_parameters, {})
        self.assertEqual(self.fitting_context.y_parameters, {})
        self.assertEqual(self.fitting_context.y_parameter_errors, {})

    def test_that_current_result_table_index_will_raise_if_the_index_provided_is_too_large(self):
        with self.assertRaises(AssertionError):
            self.fitting_context.current_result_table_index = 4

    def test_that_current_result_table_index_will_not_raise_a_runtime_error_if_theres_no_datasets_and_none_is_provided(self):
        self.fitting_context.current_result_table_index = None

    def test_that_number_of_result_tables_will_return_the_expected_number_of_results_tables(self):
        self.assertEqual(self.fitting_context.number_of_result_tables(), 0)

        self.fitting_context.result_table_names = ["Results1", "Results2"]
        self.assertEqual(self.fitting_context.number_of_result_tables(), 2)

    def test_that_clear_will_clear_the_undo_data_and_active_fits_list(self):
        self.fitting_context.active_fit_history = [mock.Mock(), mock.Mock()]
        self.fitting_context.dataset_indices_for_undo = [0, 1]
        self.fitting_context.single_fit_functions_for_undo = [mock.Mock(), mock.Mock()]
        self.fitting_context.fit_statuses_for_undo = ["Success", "Fail"]
        self.fitting_context.chi_squared_for_undo = [2.2, 3.3]

        self.assertEqual(len(self.fitting_context.active_fit_history), 2)
        self.assertEqual(len(self.fitting_context.all_latest_fits()), 2)
        self.assertEqual(len(self.fitting_context.dataset_indices_for_undo), 2)
        self.assertEqual(len(self.fitting_context.single_fit_functions_for_undo), 2)
        self.assertEqual(len(self.fitting_context.fit_statuses_for_undo), 2)
        self.assertEqual(len(self.fitting_context.chi_squared_for_undo), 2)

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
