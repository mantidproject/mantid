# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import FrameworkManager, FunctionFactory

from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import BasicFittingModel, DEFAULT_START_X
from Muon.GUI.Common.test_helpers.context_setup import setup_context


class BasicFittingModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        self.model = BasicFittingModel(setup_context())
        self.dataset_names = ["Name1", "Name2"]
        self.fit_function = FunctionFactory.createFunction("FlatBackground")
        self.single_fit_functions = [self.fit_function.clone(), self.fit_function.clone()]

    def tearDown(self):
        self.model = None

    def test_that_the_model_has_been_instantiated_with_empty_fit_data(self):
        self.assertEqual(self.model.dataset_names, [])
        self.assertEqual(self.model.current_dataset_index, None)
        self.assertEqual(self.model.start_xs, [])
        self.assertEqual(self.model.end_xs, [])
        self.assertEqual(self.model.single_fit_functions, [])
        self.assertEqual(self.model.single_fit_functions_cache, [])
        self.assertEqual(self.model.fit_statuses, [])
        self.assertEqual(self.model.chi_squared, [])
        self.assertEqual(self.model.function_name, "")
        self.assertTrue(self.model.function_name_auto_update)
        self.assertEqual(self.model.minimizer, "")
        self.assertEqual(self.model.evaluation_type, "")
        self.assertTrue(self.model.fit_to_raw)

    def test_that_dataset_names_will_set_the_names_of_the_datasets_as_expected(self):
        self.model.dataset_names = self.dataset_names
        self.assertEqual(self.model.dataset_names, self.dataset_names)

    def test_that_current_dataset_index_will_raise_if_the_index_is_greater_than_or_equal_to_the_number_of_datasets(self):
        self.model.dataset_names = self.dataset_names
        with self.assertRaises(RuntimeError):
            self.model.current_dataset_index = 2

    def test_that_current_dataset_index_will_set_the_current_dataset_index_as_expected(self):
        self.model.dataset_names = self.dataset_names

        self.model.current_dataset_index = 1

        self.assertEqual(self.model.current_dataset_index, 1)

    def test_that_it_is_possible_to_set_the_value_of_the_current_dataset_name_as_expected(self):
        self.model.dataset_names = self.dataset_names
        self.model.current_dataset_index = 1

        self.model.current_dataset_name = "NewName"

        self.assertEqual(self.model.dataset_names, self.dataset_names)

    def test_that_number_of_datasets_returns_the_correct_number_of_datasets_in_the_model(self):
        self.model.dataset_names = self.dataset_names
        self.assertEqual(self.model.number_of_datasets, 2)

    def test_that_it_is_possible_to_set_the_start_xs_as_expected(self):
        start_xs = [0.0, 1.0]

        self.model.dataset_names = self.dataset_names
        self.model.start_xs = start_xs

        self.assertEqual(self.model.start_xs, start_xs)

    def test_that_setting_the_start_xs_will_raise_if_the_number_of_xs_is_not_equal_to_the_number_of_datasets(self):
        self.model.dataset_names = self.dataset_names
        with self.assertRaises(RuntimeError):
            self.model.start_xs = [0.0]

    def test_that_it_is_possible_to_set_the_current_start_x_as_expected(self):
        self.model.dataset_names = self.dataset_names
        self.model.start_xs = [0.0, 1.0]
        self.model.current_dataset_index = 1

        self.model.current_start_x = 5.0

        self.assertEqual(self.model.start_xs, [0.0, 5.0])

    def test_that_the_current_start_x_will_not_be_changed_if_the_value_provided_is_larger_than_the_current_end_x(self):
        self.model.dataset_names = self.dataset_names
        self.model.start_xs = [0.0, 1.0]
        self.model.end_xs = [5.0, 6.0]
        self.model.current_dataset_index = 1

        self.model.current_start_x = 8.0

        self.assertEqual(self.model.start_xs, [0.0, 1.0])

    def test_that_it_is_possible_to_set_the_end_xs_as_expected(self):
        end_xs = [10.0, 11.0]

        self.model.dataset_names = self.dataset_names
        self.model.end_xs = end_xs

        self.assertEqual(self.model.end_xs, end_xs)

    def test_that_setting_the_end_xs_will_raise_if_the_number_of_xs_is_not_equal_to_the_number_of_datasets(self):
        self.model.dataset_names = self.dataset_names
        with self.assertRaises(RuntimeError):
            self.model.end_xs = [10.0]

    def test_that_it_is_possible_to_set_the_current_end_x_as_expected(self):
        self.model.dataset_names = self.dataset_names
        self.model.end_xs = [10.0, 11.0]
        self.model.current_dataset_index = 1

        self.model.current_end_x = 15.0

        self.assertEqual(self.model.end_xs, [10.0, 15.0])

    def test_that_the_current_end_x_will_not_be_changed_if_the_value_provided_is_smaller_than_the_current_start_x(self):
        self.model.dataset_names = self.dataset_names
        self.model.start_xs = [5.0, 6.0]
        self.model.end_xs = [10.0, 11.0]
        self.model.current_dataset_index = 1

        self.model.current_end_x = 4.0

        self.assertEqual(self.model.end_xs, [10.0, 11.0])

    def test_that_setting_the_dataset_names_will_reset_the_start_and_end_xs(self):
        self.model.dataset_names = self.dataset_names
        self.model.start_xs = [0.0, 0.0]
        self.model.end_xs = [10.0, 10.0]

        self.model.dataset_names = []
        self.assertEqual(self.model.start_xs, [])
        self.assertEqual(self.model.end_xs, [])

    def test_that_it_is_possible_to_set_the_single_fit_function_as_expected(self):
        self.model.dataset_names = self.dataset_names

        self.model.single_fit_functions = self.single_fit_functions

        self.assertEqual(str(self.model.single_fit_functions), str(self.single_fit_functions))

    def test_that_setting_the_single_fit_function_will_raise_if_the_number_of_functions_is_not_equal_to_the_number_of_datasets(self):
        self.model.dataset_names = self.dataset_names
        with self.assertRaises(RuntimeError):
            self.model.single_fit_functions = [self.fit_function]

    def test_that_clear_single_fit_functions_will_clear_the_single_fit_functions_as_expected(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions

        self.model.clear_single_fit_functions()

        self.assertEqual(self.model.single_fit_functions, [None, None])

    def test_that_current_single_fit_function_will_return_the_default_function_if_there_is_no_selected_data(self):
        self.assertEqual(self.model.current_single_fit_function, None)

    def test_that_it_is_possible_to_set_the_current_single_fit_function_as_expected(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.current_dataset_index = 1

        self.model.current_single_fit_function = None

        self.assertEqual(str(self.model.single_fit_functions[0]), "name=FlatBackground,A0=0")
        self.assertEqual(self.model.single_fit_functions[1], None)

    def test_that_get_single_fit_function_for_will_return_the_correct_fit_function_corresponding_to_a_dataset(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = [self.fit_function, None]

        fit_function1 = self.model.get_single_fit_function_for("Name1")
        fit_function2 = self.model.get_single_fit_function_for("Name2")

        self.assertEqual(str(fit_function1), "name=FlatBackground,A0=0")
        self.assertEqual(fit_function2, None)

    def test_that_cache_the_single_fit_functions_will_cache_the_fit_functions_in_the_model(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = [self.fit_function, None]

        self.model.cache_the_current_fit_functions()

        self.assertEqual(len(self.model.single_fit_functions), 2)
        self.assertEqual(str(self.model.single_fit_functions_cache[0]), "name=FlatBackground,A0=0")
        self.assertEqual(self.model.single_fit_functions_cache[1], None)

    def test_that_clear_cached_fit_functions_will_clear_the_cache_of_fit_functions(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = [self.fit_function, None]

        self.model.cache_the_current_fit_functions()
        self.model.clear_cached_fit_functions()

        self.assertEqual(len(self.model.single_fit_functions), 2)
        self.assertEqual(self.model.single_fit_functions_cache[0], None)
        self.assertEqual(self.model.single_fit_functions_cache[1], None)

    def test_that_setting_the_fit_statuses_will_raise_if_the_number_of_fit_statuses_is_not_equal_to_the_number_of_datasets(self):
        self.model.dataset_names = self.dataset_names
        with self.assertRaises(RuntimeError):
            self.model.fit_statuses = ["success"]

    def test_it_is_possible_to_set_the_currently_selected_fit_status(self):
        self.model.dataset_names = self.dataset_names
        self.model.fit_statuses = ["success", "success"]
        self.model.current_dataset_index = 1

        self.model.current_fit_status = "failure"

        self.assertEqual(self.model.fit_statuses, ["success", "failure"])

    def test_that_setting_the_chi_squared_will_raise_if_the_number_of_chi_squared_is_not_equal_to_the_number_of_datasets(self):
        self.model.dataset_names = self.dataset_names
        with self.assertRaises(RuntimeError):
            self.model.chi_squared = [0.0]

    def test_it_is_possible_to_set_the_currently_selected_chi_squared(self):
        self.model.dataset_names = self.dataset_names
        self.model.chi_squared = [5.0, 6.0]
        self.model.current_dataset_index = 1

        self.model.current_chi_squared = 0.0

        self.assertEqual(self.model.chi_squared, [5.0, 0.0])

    def test_that_setting_the_function_name_will_add_a_space_at_the_start_of_the_name(self):
        self.model.function_name = "FuncName"
        self.assertEqual(self.model.function_name, " FuncName")

    def test_that_an_empty_string_is_stored_for_the_function_name_if_an_empty_string_is_provided(self):
        self.model.function_name = "FuncName"
        self.model.function_name = ""
        self.assertEqual(self.model.function_name, "")

    def test_that_automatically_update_function_name_will_choose_the_correct_function_name(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.current_dataset_index = 0

        self.model.automatically_update_function_name()

        self.assertEqual(self.model.function_name, " FlatBackground")

    def test_that_automatically_update_function_name_will_not_update_the_function_name_if_automatic_update_is_turned_off(self):
        self.model.function_name = "FuncName"
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.current_dataset_index = 0

        self.model.function_name_auto_update = False
        self.model.automatically_update_function_name()

        self.assertEqual(self.model.function_name, " FuncName")

    def test_that_simultaneous_fitting_mode_returns_false(self):
        self.assertTrue(not self.model.simultaneous_fitting_mode)

    def test_that_use_cached_function_will_replace_the_single_functions_with_the_cached_functions(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = [self.fit_function, None]
        self.model.cache_the_current_fit_functions()
        self.model.single_fit_functions = [None, None]

        self.model.use_cached_function()

        self.assertEqual(str(self.model.single_fit_functions[0]), "name=FlatBackground,A0=0")
        self.assertEqual(self.model.single_fit_functions[1], None)

    def test_that_the_minimizer_property_can_be_set_as_expected(self):
        minimizer = "A Minimizer"

        self.model.minimizer = minimizer

        self.assertEqual(self.model.minimizer, minimizer)

    def test_that_the_evaluation_type_property_can_be_set_as_expected(self):
        evaluation_type = "An evaluation type"

        self.model.evaluation_type = evaluation_type

        self.assertEqual(self.model.evaluation_type, evaluation_type)

    def test_that_the_fit_to_raw_property_can_be_set_as_expected(self):
        fit_to_raw = True

        self.model.fit_to_raw = fit_to_raw

        self.assertTrue(self.model.fit_to_raw)

    def test_that_the_current_dataset_index_will_be_reset_to_none_if_there_are_no_datasets_anymore(self):
        self.model.dataset_names = self.dataset_names
        self.model.current_dataset_index = 1
        self.assertEqual(self.model.current_dataset_index, 1)

        self.model.dataset_names = []

        self.assertEqual(self.model.current_dataset_index, None)

    def test_that_the_current_dataset_index_will_be_reset_to_zero_it_has_gone_out_of_range_but_there_are_still_datasets_left(self):
        self.model.dataset_names = self.dataset_names
        self.model.current_dataset_index = 1
        self.assertEqual(self.model.current_dataset_index, 1)

        self.model.dataset_names = ["Name3"]

        self.assertEqual(self.model.current_dataset_index, 0)

    def test_that_setting_new_dataset_names_will_reset_the_fit_statuses_and_chi_squared(self):
        self.model.dataset_names = self.dataset_names
        self.model.fit_statuses = ["success", "failure"]
        self.model.chi_squared = [2.0, 2.0]

        self.model.dataset_names = ["Name3"]

        self.assertEqual(self.model.fit_statuses, [None])
        self.assertEqual(self.model.chi_squared, [0.0])

    def test_that_setting_new_dataset_names_will_reset_the_fit_functions_but_attempt_to_use_the_previous_function(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions

        self.model.dataset_names = ["Name3"]

        self.assertEqual(len(self.model.single_fit_functions), 1)
        self.assertEqual(str(self.model.single_fit_functions[0]), "name=FlatBackground,A0=0")

    def test_that_retrieve_first_good_data_from_run_will_return_the_default_start_x_if_it_cannot_find_the_workspace(self):
        self.assertEqual(self.model.retrieve_first_good_data_from_run("WorkspaceName"), DEFAULT_START_X)

    def test_that_get_active_fit_function_returns_the_currently_selected_fit_function(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = [None, self.fit_function]
        self.model.current_dataset_index = 1

        self.assertEqual(str(self.model.get_active_fit_function()), "name=FlatBackground,A0=0")

    def test_that_get_active_workspace_names_returns_the_currently_selected_dataset_in_a_list(self):
        self.model.dataset_names = self.dataset_names
        self.model.current_dataset_index = 1

        self.assertEqual(self.model.get_active_workspace_names(), ["Name2"])

    def test_that_get_active_fit_results_returns_an_empty_list_if_there_are_no_datasets(self):
        self.assertEqual(self.model.get_active_fit_results(), [])

    def test_update_plot_guess_will_evaluate_the_function(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.start_xs = [0.0, 1.0]
        self.model.end_xs = [10.0, 11.0]
        self.model.current_dataset_index = 0

        self.model.context = mock.Mock()
        with mock.patch('Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model.EvaluateFunction') as mock_evaluate:
            self.model._get_guess_parameters = mock.Mock(return_value=['func', 'ws'])
            self.model.update_plot_guess(True)
            mock_evaluate.assert_called_with(InputWorkspace=self.model.current_dataset_name,
                                             Function=self.model.current_single_fit_function,
                                             StartX=self.model.current_start_x,
                                             EndX=self.model.current_end_x,
                                             OutputWorkspace="__frequency_domain_analysis_fitting_guessName1")

    @mock.patch('Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model.EvaluateFunction')
    def test_update_plot_guess_notifies_subscribers_with_the_guess_workspace_name_if_plot_guess_is_true(self, mock_evaluate):
        guess_workspace_name = "__frequency_domain_analysis_fitting_guessName1"
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.start_xs = [0.0, 1.0]
        self.model.end_xs = [10.0, 11.0]
        self.model.current_dataset_index = 0

        self.model.context = mock.Mock()
        self.model.update_plot_guess(True)
        mock_evaluate.assert_called_with(InputWorkspace=self.model.current_dataset_name,
                                         Function=self.model.current_single_fit_function,
                                         StartX=self.model.current_start_x,
                                         EndX=self.model.current_end_x,
                                         OutputWorkspace=guess_workspace_name)
        self.assertEqual(1, self.model.context.fitting_context.notify_plot_guess_changed.call_count)
        self.model.context.fitting_context.notify_plot_guess_changed.assert_called_with(True, guess_workspace_name)

    def test_update_plot_guess_notifies_subscribers_with_the_guess_workspace_name_as_none_if_plot_guess_is_false(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.start_xs = [0.0, 1.0]
        self.model.end_xs = [10.0, 11.0]
        self.model.current_dataset_index = 0

        self.model.context = mock.Mock()
        self.model.update_plot_guess(False)

        self.assertEqual(1, self.model.context.fitting_context.notify_plot_guess_changed.call_count)
        self.model.context.fitting_context.notify_plot_guess_changed.assert_called_with(False, None)

    def test_perform_fit_will_call_the_correct_function_for_a_single_fit(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions

        self.model._do_single_fit = mock.MagicMock(return_value=(self.model.current_single_fit_function, "success",
                                                                 0.56))

        self.model.perform_fit()

        self.model._do_single_fit.assert_called_once_with(self.model._get_parameters_for_single_fit())


if __name__ == '__main__':
    unittest.main()
