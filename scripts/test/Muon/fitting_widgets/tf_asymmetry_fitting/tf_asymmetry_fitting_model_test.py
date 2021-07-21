# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import FrameworkManager, FunctionFactory
from mantid.simpleapi import CreateSampleWorkspace

from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_model import TFAsymmetryFittingModel
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_base_pair import MuonBasePair
from Muon.GUI.Common.test_helpers.context_setup import setup_context


def create_tf_asymmetry_function(normal_function):
    flat_back_normalisation = FunctionFactory.createFunction("FlatBackground")

    comp1 = FunctionFactory.createFunction("CompositeFunction")
    flat_back = FunctionFactory.createFunction("FlatBackground")
    comp1.add(flat_back)
    comp1.add(normal_function)

    prod_func = FunctionFactory.createFunction("ProductFunction")
    prod_func.add(flat_back_normalisation)
    prod_func.add(comp1)

    exp_decay_muon = FunctionFactory.createFunction("ExpDecayMuon")

    comp2 = FunctionFactory.createFunction("CompositeFunction")
    comp2.add(prod_func)
    comp2.add(exp_decay_muon)
    return comp2


class TFAsymmetryFittingModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        context = setup_context()
        self.model = TFAsymmetryFittingModel(context, context.fitting_context)
        self.dataset_names = ["EMU20884; Group; fwd; Asymmetry", "EMU20884; Group; top; Asymmetry"]
        self.tf_non_compliant_dataset_names = ["EMU20884; Group; fwd; Asymmetry", "EMU20884; Pair Asym; long; Asymmetry"]
        self.fit_function = FunctionFactory.createFunction("FlatBackground")
        self.single_fit_functions = [self.fit_function.clone(), self.fit_function.clone()]
        self.simultaneous_fit_function = FunctionFactory.createInitializedMultiDomainFunction(str(self.fit_function),
                                                                                              len(self.dataset_names))

        self.tf_single_function = create_tf_asymmetry_function(self.fit_function.clone())
        self.tf_single_fit_functions = [self.tf_single_function.clone(), self.tf_single_function.clone()]
        self.tf_simultaneous_fit_function = FunctionFactory.createInitializedMultiDomainFunction(str(self.tf_single_function),
                                                                                                 len(self.dataset_names))

        CreateSampleWorkspace(Function="One Peak",
                              XMin=0.0,
                              XMax=15.0,
                              BinWidth=0.1, OutputWorkspace=self.dataset_names[0])
        CreateSampleWorkspace(Function="One Peak",
                              XMin=0.0,
                              XMax=15.0,
                              BinWidth=0.1, OutputWorkspace=self.dataset_names[1])

    def tearDown(self):
        self.model = None

    def test_that_the_model_has_been_instantiated_with_empty_fit_data(self):
        self.assertEqual(self.model.dataset_names, [])
        self.assertEqual(self.model.current_dataset_index, None)
        self.assertEqual(self.model.start_xs, [])
        self.assertEqual(self.model.end_xs, [])
        self.assertEqual(self.model.single_fit_functions, [])
        self.assertEqual(self.model.fit_statuses, [])
        self.assertEqual(self.model.chi_squared, [])
        self.assertEqual(self.model.function_name, "")
        self.assertTrue(self.model.function_name_auto_update)
        self.assertEqual(self.model.minimizer, "")
        self.assertEqual(self.model.evaluation_type, "")
        self.assertTrue(self.model.fit_to_raw)

        self.assertEqual(self.model.simultaneous_fit_function, None)
        self.assertTrue(not self.model.simultaneous_fitting_mode)
        self.assertEqual(self.model.simultaneous_fit_by, "")
        self.assertEqual(self.model.simultaneous_fit_by_specifier, "")
        self.assertEqual(self.model.global_parameters, [])

        self.assertTrue(not self.model.tf_asymmetry_mode)
        self.assertEqual(self.model.tf_asymmetry_single_functions, [])
        self.assertEqual(self.model.tf_asymmetry_simultaneous_function, None)

    def test_that_setting_the_dataset_names_will_recalculate_the_tf_asymmetry_functions_to_be_empty(self):
        self.model.dataset_names = self.dataset_names

        self.assertEqual(self.model.dataset_names, self.dataset_names)
        self.assertEqual(self.model.tf_asymmetry_single_functions, [None] * self.model.number_of_datasets)
        self.assertEqual(self.model.tf_asymmetry_simultaneous_function, None)

    def test_that_setting_the_dataset_names_will_recalculate_the_tf_asymmetry_functions_to_be_empty_for_non_compliant_data(self):
        self.model.tf_asymmetry_mode = True
        self.model.dataset_names = self.tf_non_compliant_dataset_names

        self.assertEqual(self.model.dataset_names, self.tf_non_compliant_dataset_names)
        self.assertEqual(self.model.tf_asymmetry_single_functions, [None] * self.model.number_of_datasets)
        self.assertEqual(self.model.tf_asymmetry_simultaneous_function, None)

    def test_that_it_is_possible_to_set_tf_asymmetry_mode_as_being_on_as_expected(self):
        self.assertTrue(not self.model.tf_asymmetry_mode)

        self.model.tf_asymmetry_mode = True

        self.assertTrue(self.model.tf_asymmetry_mode)

    def test_that_current_tf_asymmetry_single_function_returns_the_selected_single_function(self):
        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_single_functions = [self.tf_single_function.clone(), None]

        self.model.current_dataset_index = 0
        self.assertEqual(str(self.model.current_tf_asymmetry_single_function), str(self.tf_single_function))

        self.model.current_dataset_index = 1
        self.assertEqual(self.model.current_tf_asymmetry_single_function, None)

    def test_that_update_tf_asymmetry_single_fit_function_will_update_the_current_tf_symmetry_and_normal_single_function(self):
        self.model.dataset_names = self.dataset_names
        fit_function = self.fit_function.clone()
        tf_fit_function = self.tf_single_function.clone()
        self.model.single_fit_functions = [fit_function, None]
        self.model.tf_asymmetry_single_functions = [tf_fit_function, None]
        self.model.current_dataset_index = 1

        self.model._get_normal_fit_function_from = mock.Mock(return_value=fit_function)

        self.model.update_tf_asymmetry_single_fit_function(self.model.current_dataset_index, tf_fit_function)

        self.model._get_normal_fit_function_from.assert_called_once_with(tf_fit_function)
        self.assertEqual(str(self.model.current_single_fit_function), str(fit_function))
        self.assertEqual(str(self.model.current_tf_asymmetry_single_function), str(tf_fit_function))

    def test_that_the_tf_asymmetry_simultaneous_function_can_be_set_as_expected(self):
        self.assertEqual(self.model.tf_asymmetry_simultaneous_function, None)

        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function

        self.assertEqual(str(self.model.tf_asymmetry_simultaneous_function), str(self.tf_simultaneous_fit_function))

    def test_that_update_tf_asymmetry_simultaneous_fit_function_will_update_the_current_functions_when_using_one_dataset(self):
        self.model.dataset_names = self.dataset_names[:1]
        self.model._get_normal_fit_function_from = mock.Mock(return_value=self.simultaneous_fit_function)

        self.model.update_tf_asymmetry_simultaneous_fit_function(self.tf_simultaneous_fit_function)

        self.model._get_normal_fit_function_from.assert_called_once_with(self.tf_simultaneous_fit_function)
        self.assertEqual(str(self.model.simultaneous_fit_function), str(self.simultaneous_fit_function))
        self.assertEqual(str(self.model.tf_asymmetry_simultaneous_function), str(self.tf_simultaneous_fit_function))

    def test_that_update_tf_asymmetry_simultaneous_fit_function_will_update_the_current_functions_when_using_multiple_dataset(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model._get_normal_fit_function_from = mock.Mock(return_value=self.fit_function)

        self.model.update_tf_asymmetry_simultaneous_fit_function(self.tf_simultaneous_fit_function)

        self.assertEqual(str(self.model.simultaneous_fit_function), str(self.simultaneous_fit_function))
        self.assertEqual(str(self.model.tf_asymmetry_simultaneous_function), str(self.tf_simultaneous_fit_function))

    def test_that_get_domain_tf_asymmetry_fit_function_returns_the_correct_function_when_there_is_only_one_dataset(self):
        self.model.dataset_names = self.dataset_names[:1]
        self.assertEqual(str(self.model.get_domain_tf_asymmetry_fit_function(self.tf_single_function, 0)),
                         str(self.tf_single_function))

    def test_that_get_domain_tf_asymmetry_fit_function_returns_the_correct_function_when_there_is_multiple_datasets(self):
        self.model.dataset_names = self.dataset_names
        self.assertEqual(str(self.model.get_domain_tf_asymmetry_fit_function(self.tf_simultaneous_fit_function, 0)),
                         str(self.tf_single_function))

    def test_that_reset_tf_asymmetry_functions_will_reset_the_tf_asymmetry_fit_functions(self):
        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_single_functions = [self.tf_single_function.clone(), None]
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function

        self.model.reset_tf_asymmetry_functions()

        self.assertEqual(self.model.tf_asymmetry_single_functions, [None, None])
        self.assertEqual(self.model.tf_asymmetry_simultaneous_function, None)

    def test_that_recalculate_tf_asymmetry_functions_will_reset_the_functions_if_tf_asymmetry_is_turned_off(self):
        self.model.tf_asymmetry_mode = False
        self.model.reset_tf_asymmetry_functions = mock.Mock()

        self.assertTrue(self.model.recalculate_tf_asymmetry_functions())
        self.model.reset_tf_asymmetry_functions.assert_called_once_with()

    def test_that_recalculate_tf_asymmetry_functions_will_attempt_to_recalculate_when_tf_asymmetry_is_turned_on(self):
        self.model.tf_asymmetry_mode = True
        self.model._recalculate_tf_asymmetry_functions = mock.Mock()

        self.assertTrue(self.model.recalculate_tf_asymmetry_functions())
        self.model._recalculate_tf_asymmetry_functions.assert_called_once_with()

    def test_that_recalculate_tf_asymmetry_functions_will_reset_the_functions_if_the_recalculation_fails(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_mode = True
        self.model.reset_tf_asymmetry_functions = mock.Mock()

        self.assertTrue(not self.model.recalculate_tf_asymmetry_functions())
        self.model.reset_tf_asymmetry_functions.assert_called_once_with()

    def test_that_setting_the_current_normalisation_will_work_in_single_fit_mode(self):
        new_normalisation = 5.0

        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = False
        self.model.tf_asymmetry_single_functions = self.tf_single_fit_functions

        self.assertEqual(self.model.current_normalisation(), 0.0)
        self.model.set_current_normalisation(new_normalisation)

        self.assertEqual(self.model.current_normalisation(), new_normalisation)

    def test_that_setting_the_current_normalisation_will_work_in_simultaneous_fit_mode(self):
        new_normalisation = 5.0

        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function

        self.assertEqual(self.model.current_normalisation(), 0.0)
        self.model.set_current_normalisation(new_normalisation)

        self.assertEqual(self.model.current_normalisation(), new_normalisation)

    def test_that_getting_the_current_normalisation_error_will_work_in_single_fit_mode(self):
        normalisation_error = 0.0

        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = False
        self.model.tf_asymmetry_single_functions = self.tf_single_fit_functions

        self.assertEqual(self.model.current_normalisation_error(), normalisation_error)

    def test_that_getting_the_current_normalisation_error_will_work_in_simultaneous_fit_mode(self):
        normalisation_error = 0.0

        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function

        self.assertEqual(self.model.current_normalisation_error(), normalisation_error)

    def test_that_toggle_fix_current_normalisation_will_fix_the_current_normalisation_in_single_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = False
        self.model.tf_asymmetry_single_functions = self.tf_single_fit_functions

        self.assertTrue(not self.model.is_current_normalisation_fixed())
        self.model.toggle_fix_current_normalisation(True)

        self.assertTrue(self.model.is_current_normalisation_fixed())

    def test_that_toggle_fix_current_normalisation_will_fix_the_current_normalisation_in_simultaneous_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function

        self.assertTrue(not self.model.is_current_normalisation_fixed())
        self.model.toggle_fix_current_normalisation(True)

        self.assertTrue(self.model.is_current_normalisation_fixed())

    def test_that_update_parameter_value_will_set_the_value_of_the_correct_parameter_when_in_single_fit_mode(self):
        new_value = 5.0

        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = False
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_single_functions = self.tf_single_fit_functions

        self.model.update_parameter_value("A0", new_value)

        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.single_fit_functions[0])[0], [5.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.single_fit_functions[1])[0], [0.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.tf_asymmetry_single_functions[0])[0],
                         [0.0, 0.0, 5.0, 0.2, 0.2])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.tf_asymmetry_single_functions[1])[0],
                         [0.0, 0.0, 0.0, 0.2, 0.2])

    def test_that_update_parameter_value_will_set_the_value_of_the_correct_parameter_when_in_simultaneous_fit_mode(self):
        new_value = 5.0

        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = True
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function

        self.model.update_parameter_value("A0", new_value)

        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.simultaneous_fit_function)[0],
                         [new_value, 0.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.tf_asymmetry_simultaneous_function)[0],
                         [0.0, 0.0, new_value, 0.2, 0.2, 0.0, 0.0, 0.0, 0.2, 0.2])

    def test_that_automatically_update_function_name_will_set_the_correct_name_for_tf_and_single_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = False
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_single_functions = self.tf_single_fit_functions

        self.model.automatically_update_function_name()

        self.assertEqual(self.model.function_name, " FlatBackground,TFAsymmetry")

    def test_that_automatically_update_function_name_will_set_the_correct_name_for_tf_and_simultaneous_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = True
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function

        self.model.automatically_update_function_name()

        self.assertEqual(self.model.function_name, " FlatBackground,TFAsymmetry")

    def test_that_check_datasets_are_tf_asymmetry_compliant_returns_false_if_none_of_the_dataset_names_contains_Group(self):
        self.model.dataset_names = self.tf_non_compliant_dataset_names

        tf_compliant, non_compliant_names = self.model.check_datasets_are_tf_asymmetry_compliant()

        self.assertTrue(not tf_compliant)
        self.assertEqual(non_compliant_names, "'long'")

    def test_that_check_datasets_are_tf_asymmetry_compliant_returns_true_if_all_of_the_dataset_names_contains_Group(self):
        self.model.dataset_names = self.dataset_names

        tf_compliant, non_compliant_names = self.model.check_datasets_are_tf_asymmetry_compliant()

        self.assertTrue(tf_compliant)
        self.assertEqual(non_compliant_names, "''")

    def test_that_get_fit_function_parameter_values_returns_the_expected_parameter_values(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function

        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.simultaneous_fit_function)[0],
                         [0.0, 0.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.tf_asymmetry_simultaneous_function)[0],
                         [0.0, 0.0, 0.0, 0.2, 0.2, 0.0, 0.0, 0.0, 0.2, 0.2])

    def test_perform_fit_will_call_the_correct_function_for_a_tf_single_fit(self):
        fit_variables = {"DummyVar": 5.0}
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = False

        self.model._get_parameters_for_tf_asymmetry_single_fit = mock.Mock(return_value=fit_variables)
        self.model._do_tf_asymmetry_single_fit = mock.MagicMock(return_value=(self.tf_single_function, "success", 0.56))

        self.model.perform_fit()

        self.model._get_parameters_for_tf_asymmetry_single_fit.assert_called_once_with(
            self.model.current_dataset_name, self.model.current_tf_asymmetry_single_function)
        self.model._do_tf_asymmetry_single_fit.assert_called_once_with(fit_variables)

    def test_perform_fit_will_call_the_correct_function_for_a_tf_simultaneous_fit(self):
        fit_variables = {"DummyVar": 5.0}
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = True

        self.model._get_parameters_for_tf_asymmetry_simultaneous_fit = mock.Mock(return_value=fit_variables)
        self.model._get_global_parameters_for_tf_asymmetry_fit = mock.Mock(return_value=[])
        self.model._do_tf_asymmetry_simultaneous_fit = mock.MagicMock(return_value=(self.tf_simultaneous_fit_function,
                                                                                    "success", 0.56))

        self.model.perform_fit()

        self.model._get_parameters_for_tf_asymmetry_simultaneous_fit.assert_called_once_with(
            self.model.dataset_names, self.model.tf_asymmetry_simultaneous_function)
        self.model._get_global_parameters_for_tf_asymmetry_fit.assert_called_once_with()
        self.model._do_tf_asymmetry_simultaneous_fit.assert_called_once_with(fit_variables, [])

    def test_that_set_normalisation_for_dataset_will_set_the_normalisation_as_expected_in_single_fit_mode(self):
        normalisation = 1.234
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_single_functions = self.tf_single_fit_functions
        self.model.tf_asymmetry_mode = True

        self.model._set_normalisation_for_dataset(self.model.dataset_names[1], normalisation)

        self.assertEqual(self.model.get_fit_function_parameter_values(self.tf_single_fit_functions[0])[0],
                         [0.0, 0.0, 0.0, 0.2, 0.2])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.tf_single_fit_functions[1])[0],
                         [normalisation, 0.0, 0.0, 0.2, 0.2])

    def test_that_set_normalisation_for_dataset_will_set_the_normalisation_as_expected_in_simultaneous_fit_mode(self):
        normalisation = 1.234
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True

        self.model._set_normalisation_for_dataset(self.model.dataset_names[1], normalisation)

        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.tf_asymmetry_simultaneous_function)[0],
                         [0.0, 0.0, 0.0, 0.2, 0.2, normalisation, 0.0, 0.0, 0.2, 0.2])

    def test_that_get_all_fit_functions_will_return_the_tf_single_functions_when_in_single_fit_tf_asymmetry_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_single_functions = self.tf_single_fit_functions
        self.model.simultaneous_fitting_mode = False
        self.model.tf_asymmetry_mode = True

        self.assertEqual(self.model.get_all_fit_functions(), self.model.tf_asymmetry_single_functions)

    def test_that_get_all_fit_functions_will_return_the_tf_simultaneous_function_when_in_simultaneous_fit_tf_asymmetry_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True

        self.assertEqual(self.model.get_all_fit_functions(), [self.model.tf_asymmetry_simultaneous_function])

    def test_that_get_fit_function_parameters_will_return_the_normal_parameters_when_not_in_tf_asymmetry_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.tf_asymmetry_mode = False

        self.model.simultaneous_fitting_mode = False
        self.assertEqual(self.model.get_fit_function_parameters(), ["A0"])

        self.model.simultaneous_fitting_mode = True
        self.assertEqual(self.model.get_fit_function_parameters(), ["f0.A0", "f1.A0"])

    def test_that_get_fit_function_parameters_will_return_the_parameters_including_normalisation_for_single_tf_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_single_functions = self.tf_single_fit_functions
        self.model.simultaneous_fitting_mode = False
        self.model.tf_asymmetry_mode = True

        self.assertEqual(self.model.get_fit_function_parameters(), ["N0", "A0"])

    def test_that_get_fit_function_parameters_will_return_the_parameters_including_normalisation_for_simultaneous_tf_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True

        self.assertEqual(self.model.get_fit_function_parameters(), ["f0.N0", "f0.A0", "f1.N0", "f1.A0"])

    def test_that_get_all_fit_function_parameter_values_for_will_return_the_normal_parameter_values_when_not_in_tf_asymmetry_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_mode = False

        self.model.simultaneous_fitting_mode = False
        self.assertEqual(self.model.get_all_fit_function_parameter_values_for(self.single_fit_functions[0]),
                         [0.0])

        self.model.simultaneous_fitting_mode = True
        self.assertEqual(self.model.get_all_fit_function_parameter_values_for(self.simultaneous_fit_function),
                         [0.0, 0.0])

    def test_that_get_all_fit_function_parameter_values_will_return_the_parameters_values_including_normalisation_for_single_mode(self):
        normalisation = 1.234
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_single_functions = self.tf_single_fit_functions
        self.model.simultaneous_fitting_mode = False
        self.model.tf_asymmetry_mode = True

        self.model._set_normalisation_for_dataset(self.model.dataset_names[1], normalisation)

        self.assertEqual(self.model.get_all_fit_function_parameter_values_for(
            self.model.tf_asymmetry_single_functions[1]), [normalisation, 0.0])

    def test_that_get_all_fit_function_parameter_values_will_return_the_param_values_including_normalisation_for_simultaenous_mode(self):
        normalisation = 1.234
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True

        self.model._set_normalisation_for_dataset(self.model.dataset_names[1], normalisation)

        self.assertEqual(self.model.get_all_fit_function_parameter_values_for(
            self.model.tf_asymmetry_simultaneous_function), [0.0, 0.0, normalisation, 0.0])

    def test_that_get_normalisation_from_tf_fit_function_will_return_the_expected_normalisation_for_single_fit_mode(self):
        normalisation = 1.234
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_single_functions = self.tf_single_fit_functions
        self.model.simultaneous_fitting_mode = False
        self.model.tf_asymmetry_mode = True

        self.model._set_normalisation_for_dataset(self.model.dataset_names[1], normalisation)

        self.assertEqual(self.model._get_normalisation_from_tf_fit_function(
            self.model.tf_asymmetry_single_functions[0]), 0.0)
        self.assertEqual(self.model._get_normalisation_from_tf_fit_function(
            self.model.tf_asymmetry_single_functions[1]), normalisation)

    def test_that_get_normalisation_from_tf_fit_function_will_return_the_expected_normalisation_for_simultaneous_fit_mode(self):
        normalisation = 1.234
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True

        self.model._set_normalisation_for_dataset(self.model.dataset_names[1], normalisation)

        self.assertEqual(self.model._get_normalisation_from_tf_fit_function(
            self.model.tf_asymmetry_simultaneous_function, 0), 0.0)
        self.assertEqual(self.model._get_normalisation_from_tf_fit_function(
            self.model.tf_asymmetry_simultaneous_function, 1), normalisation)

    """
    Tests for functions used by the Sequential fitting tab.
    """

    def test_that_parse_parameter_values_will_parse_the_parameters_into_normal_params_and_normalisations_for_single_mode(self):
        all_parameter_values = [1.234, 5.0, 6.0]
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fitting_mode = False
        self.model.tf_asymmetry_mode = True

        parameter_values, normalisations = self.model._parse_parameter_values(all_parameter_values)
        self.assertEqual(parameter_values, [5.0, 6.0])
        self.assertEqual(normalisations, [1.234])

    def test_that_parse_parameter_values_will_parse_the_parameters_into_normal_params_and_normalisations_for_simultaneous_mode(self):
        all_parameter_values = [1.234, 5.0, 6.0, 2.234, 7.0, 8.0]
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True

        parameter_values, normalisations = self.model._parse_parameter_values(all_parameter_values)
        self.assertEqual(parameter_values, [5.0, 6.0, 7.0, 8.0])
        self.assertEqual(normalisations, [1.234, 2.234])

    def test_that_get_runs_groups_and_pairs_for_fits_will_attempt_to_get_the_runs_groups_and_pairs_when_in_single_fit_mode(self):
        self.model._get_runs_groups_and_pairs_for_single_fit = mock.Mock(return_value=(["62260"], ["fwd", "bwd"]))

        runs, groups_and_pairs = self.model.get_runs_groups_and_pairs_for_fits()
        self.assertEqual(runs, ["62260"])
        self.assertEqual(groups_and_pairs, ["fwd", "bwd"])

        self.model._get_runs_groups_and_pairs_for_single_fit.assert_called_once_with()

    def test_that_get_runs_groups_and_pairs_for_fits_will_attempt_to_get_it_by_runs_when_in_simultaneous_fitting_mode(self):
        self.model._get_runs_groups_and_pairs_for_simultaneous_fit_by_runs = mock.Mock(return_value=(["62260"], ["fwd;bwd;long"]))
        self.model.simultaneous_fitting_mode = True
        self.model.simultaneous_fit_by = "Run"

        runs, groups_and_pairs = self.model.get_runs_groups_and_pairs_for_fits()
        self.assertEqual(runs, ["62260"])
        self.assertEqual(groups_and_pairs, ["fwd;bwd;long"])

        self.model._get_runs_groups_and_pairs_for_simultaneous_fit_by_runs.assert_called_once_with()

    def test_that_get_runs_groups_and_pairs_for_fits_will_attempt_to_get_it_by_groups_when_in_simultaneous_fitting_mode(self):
        self.model._get_runs_groups_and_pairs_for_simultaneous_fit_by_groups_and_pairs = \
            mock.Mock(return_value=(["62260;62261;62262"], ["fwd", "bwd"]))
        self.model.simultaneous_fitting_mode = True
        self.model.simultaneous_fit_by = "Group/Pair"

        runs, groups_and_pairs = self.model.get_runs_groups_and_pairs_for_fits()
        self.assertEqual(runs, ["62260;62261;62262"])
        self.assertEqual(groups_and_pairs, ["fwd", "bwd"])

        self.model._get_runs_groups_and_pairs_for_simultaneous_fit_by_groups_and_pairs.assert_called_once_with()

    def test_that_get_fit_function_parameter_values_will_return_the_parameter_values_in_the_specified_function(self):
        self.assertEqual(self.model.get_fit_function_parameter_values(self.single_fit_functions[0])[0], [0.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.simultaneous_fit_function)[0], [0.0, 0.0])

    def test_that_update_ws_fit_function_parameters_will_update_the_parameters_for_the_specified_dataset_in_single_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.simultaneous_fitting_mode = False

        self.model.update_ws_fit_function_parameters(self.model.dataset_names[:1], [1.0])

        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.single_fit_functions[0])[0], [1.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.single_fit_functions[1])[0], [0.0])

    def test_that_update_ws_fit_function_parameters_will_update_the_parameters_when_in_simultaneous_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True

        self.model.update_ws_fit_function_parameters(self.model.dataset_names[:1], [1.0, 0.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.simultaneous_fit_function),
                         ([1.0, 0.0], [0.0, 0.0]))

        self.model.update_ws_fit_function_parameters(self.model.dataset_names[1:], [1.0, 2.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.simultaneous_fit_function),
                         ([1.0, 2.0], [0.0, 0.0]))

    def test_that_update_ws_fit_function_parameters_will_update_the_parameters_when_in_TF_asymmetry_simultaneous_mode(self):
        normalisation1, normalisation2 = 1.345, 2.345
        param1, param2 = 3.345, 4.345

        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True

        self.model.update_ws_fit_function_parameters(self.dataset_names, [normalisation1, param1, normalisation2, param2])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.tf_asymmetry_simultaneous_function)[0],
                         [normalisation1, 0.0, param1, 0.2, 0.2, normalisation2, 0.0, param2, 0.2, 0.2])

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

        self.assertEqual(["MUSR1; Pair Asym; long; Rebin; MA",
                          "MUSR1; PhaseQuad; phase_Re_; Rebin; MA",
                          "MUSR1; PhaseQuad; phase2_Im_; Rebin; MA",
                          "MUSR2; Pair Asym; long; Rebin; MA",
                          "MUSR2; PhaseQuad; phase_Re_; Rebin; MA",
                          "MUSR2; PhaseQuad; phase2_Im_; Rebin; MA",
                          "MUSR3; Pair Asym; long; Rebin; MA",
                          "MUSR3; PhaseQuad; phase_Re_; Rebin; MA",
                          "MUSR3; PhaseQuad; phase2_Im_; Rebin; MA"], result)

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

        self.assertEqual(["MUSR1; Pair Asym; long; MA",
                          "MUSR1; PhaseQuad; phase_Re_; MA",
                          "MUSR1; PhaseQuad; phase2_Im_; MA",
                          "MUSR2; Pair Asym; long; MA",
                          "MUSR2; PhaseQuad; phase_Re_; MA",
                          "MUSR2; PhaseQuad; phase2_Im_; MA",
                          "MUSR3; Pair Asym; long; MA",
                          "MUSR3; PhaseQuad; phase_Re_; MA",
                          "MUSR3; PhaseQuad; phase2_Im_; MA"], result)

    def test_that_perform_sequential_fit_will_call_the_correct_functions_when_not_in_tf_asymmetry_mode(self):
        workspaces = [self.dataset_names[:1], self.dataset_names[1:]]
        flattened_workspaces = self.dataset_names
        parameter_values = [[0.0], [1.0]]
        functions = ["FakeFunc"]
        fit_statuses = ["Success"]
        chi_squared = [1.2]

        self.model._get_sequential_fitting_func_for_normal_fitting_mode = mock.Mock(return_value=None)
        self.model._flatten_workspace_names = mock.Mock(return_value=flattened_workspaces)
        self.model._evaluate_sequential_fit = mock.Mock(return_value=(functions, fit_statuses, chi_squared))
        self.model._update_fit_functions_after_sequential_fit = mock.Mock()
        self.model._update_fit_statuses_and_chi_squared_after_sequential_fit = mock.Mock()

        self.model.simultaneous_fitting_mode = False
        self.model.tf_asymmetry_mode = False

        self.model.perform_sequential_fit(workspaces, parameter_values)

        self.model._get_sequential_fitting_func_for_normal_fitting_mode.assert_called_once_with()
        self.model._flatten_workspace_names.assert_called_once_with(workspaces)
        self.model._evaluate_sequential_fit.assert_called_once_with(None, flattened_workspaces, parameter_values, False)
        self.model._update_fit_functions_after_sequential_fit.assert_called_once_with(flattened_workspaces, functions)
        self.model._update_fit_statuses_and_chi_squared_after_sequential_fit.assert_called_once_with(
            flattened_workspaces, fit_statuses, chi_squared)

    def test_that_perform_sequential_fit_will_call_the_correct_functions_when_in_tf_asymmetry_mode(self):
        workspaces = [self.dataset_names[:1], self.dataset_names[1:]]
        parameter_values = [[0.0], [1.0]]
        use_initial_values = True
        functions = ["FakeFunc"]
        fit_statuses = ["Success"]
        chi_squared = [1.2]

        self.model._get_sequential_fitting_func_for_tf_asymmetry_fitting_mode = mock.Mock(return_value=None)
        self.model._flatten_workspace_names = mock.Mock()
        self.model._evaluate_sequential_fit = mock.Mock(return_value=(functions, fit_statuses, chi_squared))
        self.model._update_fit_functions_after_sequential_fit = mock.Mock()
        self.model._update_fit_statuses_and_chi_squared_after_sequential_fit = mock.Mock()

        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True

        self.model.perform_sequential_fit(workspaces, parameter_values, use_initial_values)

        self.model._get_sequential_fitting_func_for_tf_asymmetry_fitting_mode.assert_called_once_with()
        self.assertTrue(not self.model._flatten_workspace_names.called)
        self.model._evaluate_sequential_fit.assert_called_once_with(None, workspaces, parameter_values,
                                                                    use_initial_values)
        self.model._update_fit_functions_after_sequential_fit.assert_called_once_with(workspaces, functions)
        self.model._update_fit_statuses_and_chi_squared_after_sequential_fit.assert_called_once_with(
            workspaces, fit_statuses, chi_squared)

    def test_that_are_same_workspaces_as_the_datasets_returns_true_if_all_the_dataset_names_match(self):
        self.model.dataset_names = self.dataset_names
        self.assertTrue(self.model._are_same_workspaces_as_the_datasets(self.dataset_names))

    def test_that_are_same_workspaces_as_the_datasets_returns_false_if_one_of_the_dataset_names_does_not_match(self):
        dataset_names = self.dataset_names + ["EMU20884; Group; bottom; Asymmetry"]
        self.model.dataset_names = self.dataset_names
        self.assertTrue(not self.model._are_same_workspaces_as_the_datasets(dataset_names))


if __name__ == '__main__':
    unittest.main()
