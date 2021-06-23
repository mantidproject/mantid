# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.Common.contexts.fitting_contexts.tf_asymmetry_fitting_context import TFAsymmetryFittingContext


class TFAsymmetryFittingContextTest(unittest.TestCase):

    def setUp(self):
        self.fitting_context = TFAsymmetryFittingContext()

    def test_that_allow_double_pulse_fitting_is_set_to_false_by_default(self):
        self.assertFalse(self.fitting_context.allow_double_pulse_fitting)

        self.fitting_context = TFAsymmetryFittingContext(allow_double_pulse_fitting=True)
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

        self.assertTrue(not self.fitting_context.tf_asymmetry_mode)
        self.assertEqual(self.fitting_context.tf_asymmetry_single_functions, [])
        self.assertEqual(self.fitting_context.tf_asymmetry_simultaneous_function, None)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
