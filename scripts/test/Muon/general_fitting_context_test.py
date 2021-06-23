# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import FunctionFactory, FrameworkManager
from Muon.GUI.Common.contexts.fitting_contexts.general_fitting_context import GeneralFittingContext


class GeneralFittingContextTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        self.fitting_context = GeneralFittingContext()

        self.dataset_names = ["Name1", "Name2"]
        self.fit_function = FunctionFactory.createFunction("FlatBackground")
        self.single_fit_functions = [self.fit_function.clone(), self.fit_function.clone()]
        self.simultaneous_fit_function = FunctionFactory.createInitializedMultiDomainFunction(str(self.fit_function),
                                                                                              len(self.dataset_names))

    def test_that_allow_double_pulse_fitting_is_set_to_false_by_default(self):
        self.assertFalse(self.fitting_context.allow_double_pulse_fitting)

        self.fitting_context = GeneralFittingContext(allow_double_pulse_fitting=True)
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

        self.assertEqual(self.fitting_context.simultaneous_fit_function, None)
        self.assertEqual(self.fitting_context.simultaneous_fit_function_cache, None)
        self.assertTrue(not self.fitting_context.simultaneous_fitting_mode)
        self.assertEqual(self.fitting_context.simultaneous_fit_by, "")
        self.assertEqual(self.fitting_context.simultaneous_fit_by_specifier, "")
        self.assertEqual(self.fitting_context.global_parameters, [])

    def test_that_simultaneous_fit_function_will_raise_if_there_are_no_datasets_loaded(self):
        with self.assertRaises(RuntimeError):
            self.fitting_context.simultaneous_fit_function = self.simultaneous_fit_function

    def test_that_simultaneous_fit_function_will_not_raise_if_there_are_no_datasets_and_none_is_provided(self):
        self.fitting_context.simultaneous_fit_function = None

    def test_that_simultaneous_fit_function_cache_will_raise_if_there_are_no_datasets_loaded(self):
        with self.assertRaises(RuntimeError):
            self.fitting_context.simultaneous_fit_function_cache = self.simultaneous_fit_function

    def test_that_simultaneous_fit_function_cache_will_not_raise_if_there_are_no_datasets_and_none_is_provided(self):
        self.fitting_context.simultaneous_fit_function_cache = None


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
