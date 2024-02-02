# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import AnalysisDataService, FrameworkManager, FunctionFactory
from mantid.simpleapi import CreateEmptyTableWorkspace, CreateSampleWorkspace

from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.basic_fitting_context import (
    X_FROM_FIT_RANGE,
    X_FROM_DATA_RANGE,
    X_FROM_CUSTOM,
)
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import BasicFittingModel, DEFAULT_START_X
from mantidqtinterfaces.Muon.GUI.Common.muon_pair import MuonPair
from mantidqtinterfaces.Muon.GUI.Common.muon_base_pair import MuonBasePair
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context
from mantidqtinterfaces.Muon.GUI.Common.utilities.workspace_data_utils import X_OFFSET
from mantidqtinterfaces.Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper


class BasicFittingModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        context = setup_context()
        self.model = BasicFittingModel(context, context.fitting_context)
        self.dataset_names = ["EMU20884; Group; fwd; Asymmetry", "EMU20884; Group; top; Asymmetry"]
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
        self.assertEqual(self.model.fitting_context.dataset_indices_for_undo, [])
        self.assertEqual(self.model.fitting_context.single_fit_functions_for_undo, [])
        self.assertEqual(self.model.fitting_context.fit_statuses_for_undo, [])
        self.assertEqual(self.model.fitting_context.chi_squared_for_undo, [])
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

    def test_that_setting_the_dataset_names_will_attempt_to_reuse_the_start_and_end_x_used_for_the_previous_datasets(self):
        self.model.dataset_names = self.dataset_names
        self.model.start_xs = [2.0, 3.0]
        self.model.end_xs = [4.0, 5.0]

        self.model.dataset_names = ["NewName", "EMU20884; Group; fwd; Asymmetry"]

        self.assertEqual(self.model.start_xs, [2.0, 3.0])
        self.assertEqual(self.model.end_xs, [4.0, 5.0])

    def test_that_the_currently_selected_start_and_end_xs_are_used_for_when_a_larger_number_of_new_datasets_are_loaded(self):
        self.model.x_limits_of_workspace = mock.Mock(return_value=(0.0, 10.0))

        self.model.dataset_names = self.dataset_names
        self.model.current_dataset_index = 1
        self.model.start_xs = [2.0, 3.0]
        self.model.end_xs = [4.0, 5.0]

        self.model.dataset_names = ["EMU20884; Group; fwd; Asymmetry", "EMU20884; Group; top; Asymmetry", "Name3"]

        self.assertEqual(self.model.start_xs, [2.0, 3.0, 3.0])
        self.assertEqual(self.model.end_xs, [4.0, 5.0, 5.0])

    def test_that_newly_loaded_datasets_will_reuse_the_existing_xs_when_there_are_fewer_new_datasets(self):
        self.model.dataset_names = self.dataset_names
        self.model.start_xs = [2.0, 3.0]
        self.model.end_xs = [4.0, 5.0]

        self.model.dataset_names = ["EMU20884; Group; top; Asymmetry"]

        self.assertEqual(self.model.start_xs, [3.0])
        self.assertEqual(self.model.end_xs, [5.0])

    def test_that_newly_loaded_datasets_will_reset_the_existing_exclude_xs_when_there_are_fewer_new_datasets(self):
        self.model.dataset_names = self.dataset_names
        self.model.start_xs = [2.0, 3.0]
        self.model.end_xs = [4.0, 5.0]
        self.model.fitting_context.exclude_start_xs = [2.2, 4.2]
        self.model.fitting_context.exclude_end_xs = [2.5, 4.5]

        self.model.dataset_names = ["EMU20884; Group; top; Asymmetry"]

        self.assertEqual(self.model.fitting_context.exclude_start_xs, [5.0])
        self.assertEqual(self.model.fitting_context.exclude_end_xs, [5.0])

    def test_that_newly_loaded_datasets_will_reuse_the_existing_exclude_xs_when_there_are_more_new_datasets(self):
        self.model.dataset_names = self.dataset_names
        self.model.start_xs = [2.0, 2.0]
        self.model.end_xs = [4.0, 4.0]
        self.model.fitting_context.exclude_start_xs = [2.2, 2.2]
        self.model.fitting_context.exclude_end_xs = [2.5, 2.5]

        self.model.dataset_names = ["EMU20884; Group; top; Asymmetry", "Name3", "Name4"]

        self.assertEqual(self.model.fitting_context.exclude_start_xs, [2.2, 2.2, 4.0])
        self.assertEqual(self.model.fitting_context.exclude_end_xs, [2.5, 2.5, 4.0])

    def test_that_newly_loaded_datasets_will_reuse_the_existing_exclude_xs_when_there_are_equal_num_datasets(self):
        self.model.dataset_names = self.dataset_names
        self.model.start_xs = [2.0, 2.0]
        self.model.end_xs = [4.0, 4.0]
        self.model.fitting_context.exclude_start_xs = [2.2, 2.2]
        self.model.fitting_context.exclude_end_xs = [2.5, 2.5]

        self.model.dataset_names = ["EMU20884; Group; top; Asymmetry", "Name3"]

        self.assertEqual(self.model.fitting_context.exclude_start_xs, [2.2, 2.2])
        self.assertEqual(self.model.fitting_context.exclude_end_xs, [2.5, 2.5])

    def test_that_current_dataset_index_will_raise_if_the_index_is_greater_than_or_equal_to_the_number_of_datasets(self):
        self.model.dataset_names = self.dataset_names
        with self.assertRaises(AssertionError):
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
        with self.assertRaises(AssertionError):
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
        with self.assertRaises(AssertionError):
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

    def test_that_the_current_exclude_start_x_and_end_x_will_return_the_selected_exclude_end_x(self):
        self.model.dataset_names = self.dataset_names
        self.model.start_xs = [0.0, 1.0]
        self.model.end_xs = [5.0, 6.0]
        self.model.fitting_context.exclude_start_xs = [0.1, 1.1]
        self.model.fitting_context.exclude_end_xs = [4.5, 5.5]
        self.model.current_dataset_index = 1

        self.assertEqual(self.model.current_exclude_start_x, 1.1)
        self.assertEqual(self.model.current_exclude_end_x, 5.5)

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
        with self.assertRaises(AssertionError):
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

        fit_function1 = self.model.get_single_fit_function_for("EMU20884; Group; fwd; Asymmetry")
        fit_function2 = self.model.get_single_fit_function_for("EMU20884; Group; top; Asymmetry")

        self.assertEqual(str(fit_function1), "name=FlatBackground,A0=0")
        self.assertEqual(fit_function2, None)

    def test_that_cache_the_single_fit_functions_will_cache_the_fit_functions_in_the_model(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = [self.fit_function, None]
        self.model.fit_statuses = ["Success", "Fail"]
        self.model.chi_squared = [2.0, 1.0]

        self.model.save_current_fit_function_to_undo_data()
        self.model.current_dataset_index = 1
        self.model.save_current_fit_function_to_undo_data()

        self.assertEqual(len(self.model.single_fit_functions), 2)
        self.assertEqual(self.model.fitting_context.dataset_indices_for_undo, [0, 1])
        self.assertEqual(str(self.model.fitting_context.single_fit_functions_for_undo[0]), "name=FlatBackground,A0=0")
        self.assertEqual(self.model.fitting_context.fit_statuses_for_undo, ["Success", "Fail"])
        self.assertEqual(self.model.fitting_context.chi_squared_for_undo, [2.0, 1.0])

    def test_that_clear_cached_fit_functions_will_clear_the_cache_of_fit_functions(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = [self.fit_function, None]
        self.model.fit_statuses = ["success", None]
        self.model.chi_squared = [1.0, None]

        self.model.save_current_fit_function_to_undo_data()
        self.model.clear_undo_data()

        self.assertEqual(len(self.model.fitting_context.single_fit_functions), 2)
        self.assertEqual(self.model.fitting_context.single_fit_functions_for_undo, [])
        self.assertEqual(self.model.fitting_context.fit_statuses_for_undo, [])
        self.assertEqual(self.model.fitting_context.chi_squared_for_undo, [])

    def test_that_setting_the_fit_statuses_will_raise_if_the_number_of_fit_statuses_is_not_equal_to_the_number_of_datasets(self):
        self.model.dataset_names = self.dataset_names
        with self.assertRaises(AssertionError):
            self.model.fit_statuses = ["success"]

    def test_it_is_possible_to_set_the_currently_selected_fit_status(self):
        self.model.dataset_names = self.dataset_names
        self.model.fit_statuses = ["success", "success"]
        self.model.current_dataset_index = 1

        self.model.current_fit_status = "failure"

        self.assertEqual(self.model.fit_statuses, ["success", "failure"])

    def test_that_setting_the_chi_squared_will_raise_if_the_number_of_chi_squared_is_not_equal_to_the_number_of_datasets(self):
        self.model.dataset_names = self.dataset_names
        with self.assertRaises(AssertionError):
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

    def test_that_undo_previous_fit_will_replace_the_single_functions_with_the_cached_functions(self):
        self.model.fitting_context.undo_previous_fit = mock.Mock()

        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = [self.fit_function, None]
        self.model.fit_statuses = ["success", "success"]
        self.model.chi_squared = [1.0, 2.0]

        self.model.current_dataset_index = 0
        self.model.save_current_fit_function_to_undo_data()
        self.assertEqual(self.model.number_of_undos(), 1)

        self.model.current_dataset_index = 1
        self.model.save_current_fit_function_to_undo_data()
        self.assertEqual(self.model.number_of_undos(), 2)

        self.model.single_fit_functions = [None, None]
        self.model.fit_statuses = [None, None]
        self.model.chi_squared = [None, None]

        self.model.undo_previous_fit()
        self.model.undo_previous_fit()

        self.assertEqual(str(self.model.single_fit_functions[0]), "name=FlatBackground,A0=0")
        self.assertEqual(self.model.single_fit_functions[1], None)
        self.assertEqual(self.model.fit_statuses, ["success", "success"])
        self.assertEqual(self.model.chi_squared, [1.0, 2.0])

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

    def test_that_retrieve_first_good_data_from_will_return_the_default_start_x_if_it_cannot_find_the_workspace(self):
        self.assertEqual(self.model.retrieve_first_good_data_from("WorkspaceName"), DEFAULT_START_X)

    def test_that_get_active_fit_function_returns_the_currently_selected_fit_function(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = [None, self.fit_function]
        self.model.current_dataset_index = 1

        self.assertEqual(str(self.model.get_active_fit_function()), "name=FlatBackground,A0=0")

    def test_that_get_active_workspace_names_returns_the_currently_selected_dataset_in_a_list(self):
        self.model.dataset_names = self.dataset_names
        self.model.current_dataset_index = 1

        self.assertEqual(self.model.get_active_workspace_names(), ["EMU20884; Group; top; Asymmetry"])

    def test_that_get_active_fit_results_returns_an_empty_list_if_there_are_no_datasets(self):
        self.assertEqual(self.model.get_active_fit_results(), [])

    def test_that_current_normalised_covariance_matrix_returns_none_when_there_are_no_fits(self):
        self.assertEqual(self.model.current_normalised_covariance_matrix(), None)

    def test_that_current_normalised_covariance_matrix_will_return_a_statix_workspace_wrapper_when_a_fit_exists(self):
        ws = CreateEmptyTableWorkspace()
        wrapper = StaticWorkspaceWrapper("CovarianceMatrix", ws)
        self.model._get_normalised_covariance_matrix_for = mock.Mock(return_value=wrapper)

        covariance_wrapper = self.model.current_normalised_covariance_matrix()
        self.assertEqual(covariance_wrapper, wrapper)

    def test_that_has_normalised_covariance_matrix_returns_false_when_there_is_not_a_covariance_matrix(self):
        self.assertTrue(not self.model.has_normalised_covariance_matrix())

    def test_that_has_normalised_covariance_matrix_returns_true_when_there_is_not_a_covariance_matrix(self):
        ws = CreateEmptyTableWorkspace()
        wrapper = StaticWorkspaceWrapper("CovarianceMatrix", ws)
        self.model._get_normalised_covariance_matrix_for = mock.Mock(return_value=wrapper)

        self.assertTrue(self.model.has_normalised_covariance_matrix())

    def test_update_plot_guess_will_evaluate_the_function(self):
        guess_workspace_name = "__frequency_domain_analysis_fitting_guessName1"
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.start_xs = [0.0, 1.0]
        self.model.end_xs = [10.0, 11.0]
        self.model.current_dataset_index = 0
        self.model.plot_guess = True
        self.model.plot_guess_type = X_FROM_FIT_RANGE

        self.model.context = mock.Mock()
        self.model._double_pulse_enabled = mock.Mock(return_value=False)
        self.model._get_plot_guess_name = mock.Mock(return_value=guess_workspace_name)
        with mock.patch(
            "mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model.evaluate_function", autospec=True
        ) as mock_evaluate:
            self.model._get_guess_parameters = mock.Mock(return_value=["func", "ws"])
            self.model.update_plot_guess()
            mock_evaluate.assert_called_with(
                input_ws_name=mock.ANY, fun=self.model.current_single_fit_function, out_ws_name=guess_workspace_name
            )

    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model.evaluate_function", autospec=True)
    def test_update_plot_guess_notifies_subscribers_with_the_guess_workspace_name_if_plot_guess_is_true(self, mock_evaluate):
        guess_workspace_name = "__frequency_domain_analysis_fitting_guessName1"
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.start_xs = [0.0, 1.0]
        self.model.end_xs = [10.0, 11.0]
        self.model.current_dataset_index = 0
        self.model.plot_guess = True
        self.model.plot_guess_type = X_FROM_FIT_RANGE

        self.model.context = mock.Mock()
        self.model._double_pulse_enabled = mock.Mock(return_value=False)
        self.model._get_plot_guess_name = mock.Mock(return_value=guess_workspace_name)
        self.mock_context_guess_workspace_name = mock.PropertyMock(return_value=guess_workspace_name)
        type(self.model.fitting_context).guess_workspace_name = self.mock_context_guess_workspace_name
        self.model.update_plot_guess()
        mock_evaluate.assert_called_with(
            input_ws_name=mock.ANY, fun=self.model.current_single_fit_function, out_ws_name=guess_workspace_name
        )

        self.assertEqual(1, self.mock_context_guess_workspace_name.call_count)
        self.mock_context_guess_workspace_name.assert_called_with(guess_workspace_name)

    def test_update_plot_guess_notifies_subscribers_with_the_guess_workspace_name_as_none_if_plot_guess_is_false(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.start_xs = [0.0, 1.0]
        self.model.end_xs = [10.0, 11.0]
        self.model.current_dataset_index = 0
        self.model.plot_guess = False
        self.model.plot_guess_type = X_FROM_FIT_RANGE

        self.model.context = mock.Mock()
        self.mock_context_guess_workspace_name = mock.PropertyMock(return_value=None)
        type(self.model.fitting_context).guess_workspace_name = self.mock_context_guess_workspace_name
        self.model.update_plot_guess()

        self.assertEqual(1, self.mock_context_guess_workspace_name.call_count)
        self.mock_context_guess_workspace_name.assert_called_with("")

    def test_update_plot_guess_will_evaluate_the_function_when_in_double_fit_mode(self):
        guess_workspace_name = "__frequency_domain_analysis_fitting_guessName1"
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.start_xs = [0.0, 1.0]
        self.model.end_xs = [10.0, 11.0]
        self.model.current_dataset_index = 0
        self.model.plot_guess = True
        self.model.plot_guess_type = X_FROM_FIT_RANGE

        self.model.context = mock.Mock()
        self.model._double_pulse_enabled = mock.Mock(return_value=True)
        self.model._get_guess_parameters = mock.Mock(return_value=["func", "ws"])
        self.model._get_plot_guess_name = mock.Mock(return_value=guess_workspace_name)
        self.model._evaluate_double_pulse_function = mock.Mock()

        self.model.update_plot_guess()

        self.model._evaluate_double_pulse_function.assert_called_once_with(self.model.current_single_fit_function, guess_workspace_name)

    def test_update_plot_guess_will_use_tmp_workspace_with_data_range_if_plot_range_type_is_selected(self):
        guess_workspace_name = "__frequency_domain_analysis_fitting_guessName1"
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.start_xs = [0.0, 1.0]
        self.model.end_xs = [10.0, 11.0]
        self.model.current_dataset_index = 0
        self.model.plot_guess = True
        self.model.plot_guess_type = X_FROM_FIT_RANGE

        self.model.context = mock.Mock()
        self.model._double_pulse_enabled = mock.Mock(return_value=False)
        self.model._get_plot_guess_name = mock.Mock(return_value=guess_workspace_name)
        self.model._get_guess_parameters = mock.Mock(return_value=["func", "ws"])
        self.model.update_plot_guess()

        guess_workspace = AnalysisDataService.retrieve(guess_workspace_name)
        data_workspace = AnalysisDataService.retrieve(self.dataset_names[0])
        self.assertEqual(guess_workspace.dataX(0).min(), 0.0)
        self.assertEqual(guess_workspace.dataX(0).max(), 10.0)
        self.assertEqual(guess_workspace.dataX(0).size, data_workspace.blocksize())

    def test_update_plot_guess_will_use_tmp_workspace_with_data_range_if_data_points_type_is_selected(self):
        guess_workspace_name = "__frequency_domain_analysis_fitting_guessName1"
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.start_xs = [0.0, 1.0]
        self.model.end_xs = [10.0, 11.0]
        self.model.current_dataset_index = 0
        self.model.plot_guess = True
        self.model.plot_guess_type = X_FROM_DATA_RANGE
        self.model.plot_guess_points = 1000

        self.model.context = mock.Mock()
        self.model._double_pulse_enabled = mock.Mock(return_value=False)
        self.model._get_plot_guess_name = mock.Mock(return_value=guess_workspace_name)
        self.model._get_guess_parameters = mock.Mock(return_value=["func", "ws"])
        self.model.update_plot_guess()

        guess_workspace = AnalysisDataService.retrieve(guess_workspace_name)
        data_workspace = AnalysisDataService.retrieve(self.dataset_names[0])
        self.assertEqual(guess_workspace.dataX(0).min(), data_workspace.dataX(0).min())
        self.assertEqual(guess_workspace.dataX(0).max(), data_workspace.dataX(0).max())
        self.assertEqual(guess_workspace.dataX(0).size, 1000)

    def test_update_plot_guess_will_use_tmp_workspace_with_data_range_if_custom_type_is_selected(self):
        guess_workspace_name = "__frequency_domain_analysis_fitting_guessName1"
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.start_xs = [0.0, 1.0]
        self.model.end_xs = [10.0, 11.0]
        self.model.current_dataset_index = 0
        self.model.plot_guess = True
        self.model.plot_guess_type = X_FROM_CUSTOM
        self.model.plot_guess_points = 1000
        self.model.plot_guess_start_x = 2.0
        self.model.plot_guess_end_x = 12.0

        self.model.context = mock.Mock()
        self.model._double_pulse_enabled = mock.Mock(return_value=False)
        self.model._get_plot_guess_name = mock.Mock(return_value=guess_workspace_name)
        self.model._get_guess_parameters = mock.Mock(return_value=["func", "ws"])
        self.model.update_plot_guess()

        guess_workspace = AnalysisDataService.retrieve(guess_workspace_name)
        self.assertEqual(guess_workspace.dataX(0).min(), 2.0)
        self.assertEqual(guess_workspace.dataX(0).max(), 12.0)
        self.assertEqual(guess_workspace.dataX(0).size, 1000)

    def test_perform_fit_will_call_the_correct_function_for_a_single_fit(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions

        self.model._do_single_fit = mock.MagicMock(return_value=(self.model.current_single_fit_function, "success", 0.56))

        self.model.perform_fit()

        self.model._do_single_fit.assert_called_once_with(
            self.model._get_parameters_for_single_fit(self.model.current_dataset_name, self.model.current_single_fit_function)
        )

    def test_that_get_fit_function_parameters_will_return_a_list_of_parameter_names_for_the_current_single_functions(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions

        self.assertEqual(self.model.get_fit_function_parameters(), ["A0"])

        self.fit_function = FunctionFactory.createFunction("ExpDecay")
        self.model.single_fit_functions = [self.fit_function.clone(), self.fit_function.clone()]

        self.assertEqual(self.model.get_fit_function_parameters(), ["Height", "Lifetime"])

    def test_that_get_fit_function_parameters_returns_an_empty_list_if_the_single_fit_functions_are_empty(self):
        self.model.dataset_names = self.dataset_names
        self.assertEqual(self.model.get_fit_function_parameters(), [])

    def test_that_get_all_fit_functions_returns_the_single_fit_functions(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions

        self.assertEqual(self.model.get_all_fit_functions(), self.model.single_fit_functions)

    def test_that_the_existing_functions_are_reused_when_new_datasets_are_loaded(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.current_dataset_index = 0
        self.model.single_fit_functions[0].setParameter("A0", 1)
        self.model.single_fit_functions[1].setParameter("A0", 5)

        self.model.dataset_names = ["New Name1", "New Name2"]

        self.assertEqual(str(self.model.single_fit_functions[0]), "name=FlatBackground,A0=1")
        self.assertEqual(str(self.model.single_fit_functions[1]), "name=FlatBackground,A0=5")

    def test_that_the_currently_selected_function_is_copied_for_when_a_larger_number_of_new_datasets_are_loaded(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.current_dataset_index = 0
        self.model.single_fit_functions[0].setParameter("A0", 1)
        self.model.single_fit_functions[1].setParameter("A0", 5)

        # The last two datasets are the same as the previously loaded datasets. This means their functions should be
        # reused for these last two single fit functions. The first two single fit functions are completely new, and so
        # are just given a copy of the previous currently selected function (i.e. A0=1).
        self.model.dataset_names = ["New Name1", "New Name2", "EMU20884; Group; fwd; Asymmetry", "EMU20884; Group; top; Asymmetry"]

        self.assertEqual(str(self.model.single_fit_functions[0]), "name=FlatBackground,A0=1")
        self.assertEqual(str(self.model.single_fit_functions[1]), "name=FlatBackground,A0=1")
        self.assertEqual(str(self.model.single_fit_functions[2]), "name=FlatBackground,A0=1")
        self.assertEqual(str(self.model.single_fit_functions[3]), "name=FlatBackground,A0=5")

    def test_that_newly_loaded_datasets_will_reuse_the_existing_functions_when_there_are_fewer_new_datasets(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.current_dataset_index = 0
        self.model.single_fit_functions[0].setParameter("A0", 1)
        self.model.single_fit_functions[1].setParameter("A0", 5)

        # This dataset is the second dataset previously and so the A0=5 fit function should be reused.
        self.model.dataset_names = ["EMU20884; Group; top; Asymmetry"]
        self.assertEqual(str(self.model.single_fit_functions[0]), "name=FlatBackground,A0=5")

    def test_that_x_limits_of_current_dataset_will_return_the_x_limits_of_the_workspace(self):
        self.model.dataset_names = self.dataset_names
        workspace = CreateSampleWorkspace()
        AnalysisDataService.addOrReplace("EMU20884; Group; fwd; Asymmetry", workspace)

        x_lower, x_upper = self.model.x_limits_of_workspace(self.model.current_dataset_name)

        self.assertEqual(x_lower, 0.0 - X_OFFSET)
        self.assertEqual(x_upper, 20000.0 + X_OFFSET)

    def test_that_x_limits_of_current_dataset_will_return_the_default_x_values_if_there_are_no_workspaces_loaded(self):
        x_lower, x_upper = self.model.x_limits_of_workspace(self.model.current_dataset_name)

        self.assertEqual(x_lower, 0.0)
        self.assertEqual(x_upper, 15.0)

    def test_that_x_limits_of_current_dataset_will_return_the_default_x_values_if_the_workspace_does_not_exist(self):
        x_lower, x_upper = self.model.x_limits_of_workspace("FakeName")

        self.assertEqual(x_lower, 0.0)
        self.assertEqual(x_upper, 15.0)

    """
    Tests for functions used by the Sequential fitting tab.
    """

    def test_that_get_runs_groups_and_pairs_for_fits_will_attempt_to_get_the_runs_groups_and_pairs_when_in_single_fit_mode(self):
        self.mock_context_instrument = mock.PropertyMock(return_value="EMU")
        type(self.model.context.data_context).instrument = self.mock_context_instrument

        self.model.dataset_names = self.dataset_names

        workspace_names, runs, groups_and_pairs = self.model.get_runs_groups_and_pairs_for_fits("All")
        self.assertEqual(workspace_names, self.dataset_names)
        self.assertEqual(runs, ["20884", "20884"])
        self.assertEqual(groups_and_pairs, ["fwd", "top"])

    def test_that_get_runs_groups_and_pairs_for_fits_will_attempt_to_get_the_runs_groups_and_pairs_for_a_fwd_type(self):
        self.mock_context_instrument = mock.PropertyMock(return_value="EMU")
        type(self.model.context.data_context).instrument = self.mock_context_instrument

        self.model.dataset_names = self.dataset_names

        workspace_names, runs, groups_and_pairs = self.model.get_runs_groups_and_pairs_for_fits("fwd")
        self.assertEqual(list(workspace_names), ["EMU20884; Group; fwd; Asymmetry"])
        self.assertEqual(list(runs), ["20884"])
        self.assertEqual(list(groups_and_pairs), ["fwd"])

    def test_that_get_runs_groups_and_pairs_for_fits_will_return_a_tuple_of_empty_lists_when_all_data_is_filtered_out(self):
        self.mock_context_instrument = mock.PropertyMock(return_value="EMU")
        type(self.model.context.data_context).instrument = self.mock_context_instrument

        self.model.dataset_names = self.dataset_names

        workspace_names, runs, groups_and_pairs = self.model.get_runs_groups_and_pairs_for_fits("Non-matching string")
        self.assertEqual(list(workspace_names), [])
        self.assertEqual(list(runs), [])
        self.assertEqual(list(groups_and_pairs), [])

    def test_that_get_all_fit_functions_for_will_get_all_the_fit_functions_when_the_display_type_is_all(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions

        filtered_functions = self.model.get_all_fit_functions_for("All")

        self.assertEqual(len(filtered_functions), len(self.model.single_fit_functions))
        self.assertEqual(self.model.get_fit_function_parameter_values(filtered_functions[0])[0], [0.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(filtered_functions[1])[0], [0.0])

    def test_that_get_all_fit_functions_for_will_get_only_the_fwd_fit_function_when_the_display_type_is_fwd(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions

        filtered_functions = self.model.get_all_fit_functions_for("fwd")

        self.assertEqual(len(filtered_functions), 1)
        self.assertEqual(self.model.get_fit_function_parameter_values(filtered_functions[0])[0], [0.0])

    def test_that_get_fit_function_parameter_values_will_return_the_parameter_values_in_the_specified_function(self):
        self.assertEqual(self.model.get_fit_function_parameter_values(self.single_fit_functions[0])[0], [0.0])

    def test_that_update_ws_fit_function_parameters_will_update_the_parameters_for_the_specified_dataset_in_single_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions

        self.model.update_ws_fit_function_parameters(self.model.dataset_names[:1], [1.0])

        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.single_fit_functions[0])[0], [1.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.single_fit_functions[1])[0], [0.0])

    def test_get_fit_workspace_names_from_groups_and_runs_when_fit_to_raw_is_false(self):
        self.model.fit_to_raw = False
        self.model.context.data_context.instrument = "MUSR"

        self.model.context.group_pair_context.add_pair(MuonPair("long", "f", "b", 1.0))
        self.model.context.group_pair_context.add_pair(MuonPair("long2", "f", "b", 2.0))
        self.model.context.group_pair_context.add_pair(MuonBasePair("phase_Re_"))
        self.model.context.group_pair_context.add_pair(MuonBasePair("phase_Im_"))
        self.model.context.group_pair_context.add_pair(MuonBasePair("phase2_Re_"))
        self.model.context.group_pair_context.add_pair(MuonBasePair("phase2_Im_"))
        self.model.context.group_pair_context.add_pair_to_selected_pairs("long")
        self.model.context.group_pair_context.add_pair_to_selected_pairs("phase_Re_")
        self.model.context.group_pair_context.add_pair_to_selected_pairs("phase2_Im_")

        selection = ["long", "long2", "phase_Re_", "phase_Im_", "phase2_Re_", "phase2_Im_"]
        result = self.model.get_fit_workspace_names_from_groups_and_runs([1, 2, 3], selection)

        self.assertEqual(
            [
                "MUSR1; Pair Asym; long; Rebin; MA",
                "MUSR1; PhaseQuad; phase_Re_; Rebin; MA",
                "MUSR1; PhaseQuad; phase2_Im_; Rebin; MA",
                "MUSR2; Pair Asym; long; Rebin; MA",
                "MUSR2; PhaseQuad; phase_Re_; Rebin; MA",
                "MUSR2; PhaseQuad; phase2_Im_; Rebin; MA",
                "MUSR3; Pair Asym; long; Rebin; MA",
                "MUSR3; PhaseQuad; phase_Re_; Rebin; MA",
                "MUSR3; PhaseQuad; phase2_Im_; Rebin; MA",
            ],
            result,
        )

    def test_get_fit_workspace_names_from_groups_and_runs_when_fit_to_raw_is_true(self):
        self.model.fit_to_raw = True
        self.model.context.data_context.instrument = "MUSR"

        self.model.context.group_pair_context.add_pair(MuonPair("long", "f", "b", 1.0))
        self.model.context.group_pair_context.add_pair(MuonPair("long2", "f", "b", 2.0))
        self.model.context.group_pair_context.add_pair(MuonBasePair("phase_Re_"))
        self.model.context.group_pair_context.add_pair(MuonBasePair("phase_Im_"))
        self.model.context.group_pair_context.add_pair(MuonBasePair("phase2_Re_"))
        self.model.context.group_pair_context.add_pair(MuonBasePair("phase2_Im_"))
        self.model.context.group_pair_context.add_pair_to_selected_pairs("long")
        self.model.context.group_pair_context.add_pair_to_selected_pairs("phase_Re_")
        self.model.context.group_pair_context.add_pair_to_selected_pairs("phase2_Im_")

        selection = ["long", "long2", "phase_Re_", "phase_Im_", "phase2_Re_", "phase2_Im_"]
        result = self.model.get_fit_workspace_names_from_groups_and_runs([1, 2, 3], selection)

        self.assertEqual(
            [
                "MUSR1; Pair Asym; long; MA",
                "MUSR1; PhaseQuad; phase_Re_; MA",
                "MUSR1; PhaseQuad; phase2_Im_; MA",
                "MUSR2; Pair Asym; long; MA",
                "MUSR2; PhaseQuad; phase_Re_; MA",
                "MUSR2; PhaseQuad; phase2_Im_; MA",
                "MUSR3; Pair Asym; long; MA",
                "MUSR3; PhaseQuad; phase_Re_; MA",
                "MUSR3; PhaseQuad; phase2_Im_; MA",
            ],
            result,
        )

    def test_that_validate_sequential_fit_returns_an_empty_message_when_the_data_provided_is_valid_in_normal_fitting(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions

        message = self.model.validate_sequential_fit([self.model.dataset_names])

        self.assertEqual(message, "")

    def test_that_validate_sequential_fit_returns_an_error_message_when_no_fit_function_is_selected(self):
        self.model.dataset_names = self.dataset_names

        message = self.model.validate_sequential_fit([self.model.dataset_names])

        self.assertEqual(message, "No data or fit function selected for fitting.")

    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model.make_group")
    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model.add_list_to_group")
    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model.check_if_workspace_exist")
    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model.retrieve_ws")
    def test_add_workspaces_to_group(self, retrieve_ws, check_exists, add_to_group, make_group):
        check_exists.return_value = True
        retrieve_ws.return_value = "group ws"

        self.model._add_workspaces_to_group(["ws"], "group")
        retrieve_ws.assert_called_once_with("group")
        add_to_group.assert_called_once_with(["ws"], "group ws")
        make_group.assert_not_called()

    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model.make_group")
    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model.add_list_to_group")
    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model.check_if_workspace_exist")
    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model.retrieve_ws")
    def test_add_workspaces_to_new_group(self, retrieve_ws, check_exists, add_to_group, make_group):
        check_exists.return_value = False
        retrieve_ws.return_value = "group ws"

        self.model._add_workspaces_to_group(["ws"], "group")
        retrieve_ws.assert_not_called()
        add_to_group.assert_not_called()
        make_group.assert_called_once_with(["ws"], "group")

    def test_set_current_start_and_end_x_as_expected(self):
        self.model.dataset_names = self.dataset_names
        self.model.current_dataset_index = 0
        self.model.start_xs = [0.0, 0.0]
        self.model.end_xs = [15.0, 15.0]

        self.assertEqual(self.model.current_start_x, 0.0)
        self.assertEqual(self.model.current_end_x, 15.0)
        new_start_x = 5
        new_end_x = 10

        self.model.set_current_start_and_end_x(new_start_x, new_end_x)
        self.assertEqual(self.model.current_start_x, new_start_x)
        self.assertEqual(self.model.current_end_x, new_end_x)

    def test_set_current_start_and_end_x_with_start_bigger_end(self):
        self.model.dataset_names = self.dataset_names
        self.model.current_dataset_index = 0
        self.model.start_xs = [0.0, 0.0]
        self.model.end_xs = [15.0, 15.0]

        self.assertEqual(self.model.current_start_x, 0.0)
        self.assertEqual(self.model.current_end_x, 15.0)
        new_start_x = 30
        new_end_x = 50

        self.model.set_current_start_and_end_x(new_start_x, new_end_x)
        self.assertEqual(self.model.current_start_x, new_start_x)
        self.assertEqual(self.model.current_end_x, new_end_x)

    def test_set_current_start_and_end_x_fail(self):
        self.model.dataset_names = self.dataset_names
        self.model.current_dataset_index = 0
        self.model.start_xs = [0.0, 0.0]
        self.model.end_xs = [15.0, 15.0]

        self.assertEqual(self.model.current_start_x, 0.0)
        self.assertEqual(self.model.current_end_x, 15.0)
        new_start_x = 50
        new_end_x = 10

        self.model.set_current_start_and_end_x(new_start_x, new_end_x)
        self.assertEqual(self.model.current_start_x, 0.0)
        self.assertEqual(self.model.current_end_x, 15.0)


if __name__ == "__main__":
    unittest.main()
