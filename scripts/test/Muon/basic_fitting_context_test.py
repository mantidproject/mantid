# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import FunctionFactory, FrameworkManager
from Muon.GUI.Common.contexts.fitting_contexts.basic_fitting_context import BasicFittingContext


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
        self.assertEqual(self.fitting_context.single_fit_functions, [])
        self.assertEqual(self.fitting_context.single_fit_functions_cache, [])
        self.assertEqual(self.fitting_context.fit_statuses, [])
        self.assertEqual(self.fitting_context.chi_squared, [])
        self.assertEqual(self.fitting_context.plot_guess, False)
        self.assertEqual(self.fitting_context.guess_workspace_name, None)
        self.assertEqual(self.fitting_context.function_name, "")
        self.assertTrue(self.fitting_context.function_name_auto_update)
        self.assertEqual(self.fitting_context.minimizer, "")
        self.assertEqual(self.fitting_context.evaluation_type, "")
        self.assertTrue(self.fitting_context.fit_to_raw)

    def test_that_current_dataset_index_will_raise_if_the_index_is_greater_than_or_equal_to_the_number_of_datasets(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(RuntimeError):
            self.fitting_context.current_dataset_index = 2

    def test_that_current_dataset_index_does_not_throw_when_provided_None_and_there_are_no_datasets_loaded(self):
        self.fitting_context.current_dataset_index = None

    def test_that_number_of_datasets_returns_the_expected_number_of_datasets(self):
        self.fitting_context.dataset_names = self.dataset_names
        self.assertEqual(self.fitting_context.number_of_datasets, 2)

    def test_that_start_xs_will_raise_if_the_number_of_xs_is_greater_than_or_equal_to_the_number_of_datasets(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(RuntimeError):
            self.fitting_context.start_xs = [1.0, 2.0, 3.0]

    def test_that_end_xs_will_raise_if_the_number_of_xs_is_greater_than_or_equal_to_the_number_of_datasets(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(RuntimeError):
            self.fitting_context.end_xs = [1.0, 2.0, 3.0]

    def test_that_single_fit_functions_will_raise_if_the_num_of_funcs_is_greater_than_or_equal_to_the_num_of_datasets(self):
        self.fitting_context.dataset_names = ["Name1"]
        with self.assertRaises(RuntimeError):
            self.fitting_context.single_fit_functions = self.single_fit_functions

    def test_that_single_fit_functions_cache_will_raise_if_the_num_of_funcs_is_greater_than_or_equal_to_the_num_of_datasets(self):
        self.fitting_context.dataset_names = ["Name1"]
        with self.assertRaises(RuntimeError):
            self.fitting_context.single_fit_functions_cache = self.single_fit_functions

    def test_that_fit_statuses_will_raise_if_the_number_of_datasets_is_smaller_than_the_provided_list_size(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(RuntimeError):
            self.fitting_context.fit_statuses = ["Success", "Success", "Success"]

    def test_that_fit_statuses_cache_will_raise_if_the_number_of_datasets_is_smaller_than_the_provided_list_size(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(RuntimeError):
            self.fitting_context.fit_statuses_cache = ["Success", "Success", "Success"]

    def test_that_chi_squared_will_raise_if_the_number_of_datasets_is_smaller_than_the_provided_list_size(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(RuntimeError):
            self.fitting_context.chi_squared = [1.0, 2.0, 3.0]

    def test_that_chi_squared_cache_will_raise_if_the_number_of_datasets_is_smaller_than_the_provided_list_size(self):
        self.fitting_context.dataset_names = self.dataset_names
        with self.assertRaises(RuntimeError):
            self.fitting_context.chi_squared_cache = [1.0, 2.0, 3.0]

    def test_that_guess_workspace_name_will_not_raise_if_the_workspace_does_not_exist(self):
        self.fitting_context.guess_workspace_name = "FakeName"
        self.assertEqual(self.fitting_context.guess_workspace_name, None)

    def test_that_guess_workspace_name_will_not_raise_if_the_name_provided_is_none(self):
        self.fitting_context.guess_workspace_name = None
        self.assertEqual(self.fitting_context.guess_workspace_name, None)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
