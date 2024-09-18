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

from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_model import TFAsymmetryFittingModel
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context


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

        cls.dataset_names = ["EMU20884; Group; fwd; Asymmetry", "EMU20884; Group; top; Asymmetry"]
        cls.tf_non_compliant_dataset_names = ["EMU20884; Group; fwd; Asymmetry", "EMU20884; Pair Asym; long; Asymmetry"]
        cls.fit_function = FunctionFactory.createFunction("FlatBackground")
        cls.single_fit_functions = [cls.fit_function.clone(), cls.fit_function.clone()]
        cls.simultaneous_fit_function = FunctionFactory.createInitializedMultiDomainFunction(str(cls.fit_function), len(cls.dataset_names))

        cls.tf_single_function = create_tf_asymmetry_function(cls.fit_function.clone())

        CreateSampleWorkspace(Function="One Peak", XMin=0.0, XMax=15.0, BinWidth=0.1, OutputWorkspace=cls.dataset_names[0])
        CreateSampleWorkspace(Function="One Peak", XMin=0.0, XMax=15.0, BinWidth=0.1, OutputWorkspace=cls.dataset_names[1])

    def setUp(self):
        context = setup_context()
        self.model = TFAsymmetryFittingModel(context, context.fitting_context)

        self.tf_single_fit_functions = [self.tf_single_function.clone(), self.tf_single_function.clone()]
        self.tf_simultaneous_fit_function = FunctionFactory.createInitializedMultiDomainFunction(
            str(self.tf_single_function), len(self.dataset_names)
        )

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
        self.assertEqual(str(self.model.get_domain_tf_asymmetry_fit_function(self.tf_single_function, 0)), str(self.tf_single_function))

    def test_that_get_domain_tf_asymmetry_fit_function_returns_the_correct_function_when_there_is_multiple_datasets(self):
        self.model.dataset_names = self.dataset_names
        self.assertEqual(
            str(self.model.get_domain_tf_asymmetry_fit_function(self.tf_simultaneous_fit_function, 0)), str(self.tf_single_function)
        )

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
        self.assertEqual(
            self.model.get_fit_function_parameter_values(self.model.tf_asymmetry_single_functions[0])[0], [0.0, 0.0, 5.0, 0.2, 0.2]
        )
        self.assertEqual(
            self.model.get_fit_function_parameter_values(self.model.tf_asymmetry_single_functions[1])[0], [0.0, 0.0, 0.0, 0.2, 0.2]
        )

    def test_that_update_parameter_value_will_set_the_value_of_the_correct_parameter_when_in_simultaneous_fit_mode(self):
        new_value = 5.0

        self.model.dataset_names = self.dataset_names
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = True
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function

        self.model.update_parameter_value("A0", new_value)

        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.simultaneous_fit_function)[0], [new_value, 0.0])
        self.assertEqual(
            self.model.get_fit_function_parameter_values(self.model.tf_asymmetry_simultaneous_function)[0],
            [0.0, 0.0, new_value, 0.2, 0.2, 0.0, 0.0, 0.0, 0.2, 0.2],
        )

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

        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.simultaneous_fit_function)[0], [0.0, 0.0])
        self.assertEqual(
            self.model.get_fit_function_parameter_values(self.model.tf_asymmetry_simultaneous_function)[0],
            [0.0, 0.0, 0.0, 0.2, 0.2, 0.0, 0.0, 0.0, 0.2, 0.2],
        )

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
            self.model.current_dataset_name, self.model.current_tf_asymmetry_single_function
        )
        self.model._do_tf_asymmetry_single_fit.assert_called_once_with(fit_variables)

    def test_perform_fit_will_call_the_correct_function_for_a_tf_simultaneous_fit(self):
        fit_variables = {"DummyVar": 5.0}
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fitting_mode = True

        self.model._get_parameters_for_tf_asymmetry_simultaneous_fit = mock.Mock(return_value=fit_variables)
        self.model._get_global_parameters_for_tf_asymmetry_fit = mock.Mock(return_value=[])
        self.model._do_tf_asymmetry_simultaneous_fit = mock.MagicMock(return_value=(self.tf_simultaneous_fit_function, "success", 0.56))

        self.model.perform_fit()

        self.model._get_parameters_for_tf_asymmetry_simultaneous_fit.assert_called_once_with(
            self.model.dataset_names, self.model.tf_asymmetry_simultaneous_function
        )
        self.model._get_global_parameters_for_tf_asymmetry_fit.assert_called_once_with()
        self.model._do_tf_asymmetry_simultaneous_fit.assert_called_once_with(fit_variables, [])

    def test_that_set_normalisation_for_dataset_will_set_the_normalisation_as_expected_in_single_fit_mode(self):
        normalisation = 1.234
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_single_functions = self.tf_single_fit_functions
        self.model.tf_asymmetry_mode = True

        self.model._set_normalisation_for_dataset(self.model.dataset_names[1], normalisation)

        self.assertEqual(self.model.get_fit_function_parameter_values(self.tf_single_fit_functions[0])[0], [0.0, 0.0, 0.0, 0.2, 0.2])
        self.assertEqual(
            self.model.get_fit_function_parameter_values(self.tf_single_fit_functions[1])[0], [normalisation, 0.0, 0.0, 0.2, 0.2]
        )

    def test_that_set_normalisation_for_dataset_will_set_the_normalisation_as_expected_in_simultaneous_fit_mode(self):
        normalisation = 1.234
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True

        self.model._set_normalisation_for_dataset(self.model.dataset_names[1], normalisation)

        self.assertEqual(
            self.model.get_fit_function_parameter_values(self.model.tf_asymmetry_simultaneous_function)[0],
            [0.0, 0.0, 0.0, 0.2, 0.2, normalisation, 0.0, 0.0, 0.2, 0.2],
        )

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
        self.assertEqual(self.model.get_all_fit_function_parameter_values_for(self.single_fit_functions[0]), [0.0])

        self.model.simultaneous_fitting_mode = True
        self.assertEqual(self.model.get_all_fit_function_parameter_values_for(self.simultaneous_fit_function), [0.0, 0.0])

    def test_that_get_all_fit_function_parameter_values_will_return_the_parameters_values_including_normalisation_for_single_mode(self):
        normalisation = 1.234
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_single_functions = self.tf_single_fit_functions
        self.model.simultaneous_fitting_mode = False
        self.model.tf_asymmetry_mode = True

        self.model._set_normalisation_for_dataset(self.model.dataset_names[1], normalisation)

        self.assertEqual(
            self.model.get_all_fit_function_parameter_values_for(self.model.tf_asymmetry_single_functions[1]), [normalisation, 0.0]
        )

    def test_that_get_all_fit_function_parameter_values_will_return_the_param_values_including_normalisation_for_simultaenous_mode(self):
        normalisation = 1.234
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True

        self.model._set_normalisation_for_dataset(self.model.dataset_names[1], normalisation)

        self.assertEqual(
            self.model.get_all_fit_function_parameter_values_for(self.model.tf_asymmetry_simultaneous_function),
            [0.0, 0.0, normalisation, 0.0],
        )

    def test_that_get_normalisation_from_tf_fit_function_will_return_the_expected_normalisation_for_single_fit_mode(self):
        normalisation = 1.234
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_single_functions = self.tf_single_fit_functions
        self.model.simultaneous_fitting_mode = False
        self.model.tf_asymmetry_mode = True

        self.model._set_normalisation_for_dataset(self.model.dataset_names[1], normalisation)

        self.assertEqual(self.model._get_normalisation_from_tf_fit_function(self.model.tf_asymmetry_single_functions[0]), 0.0)
        self.assertEqual(self.model._get_normalisation_from_tf_fit_function(self.model.tf_asymmetry_single_functions[1]), normalisation)

    def test_that_get_normalisation_from_tf_fit_function_will_return_the_expected_normalisation_for_simultaneous_fit_mode(self):
        normalisation = 1.234
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True

        self.model._set_normalisation_for_dataset(self.model.dataset_names[1], normalisation)

        self.assertEqual(self.model._get_normalisation_from_tf_fit_function(self.model.tf_asymmetry_simultaneous_function, 0), 0.0)
        self.assertEqual(
            self.model._get_normalisation_from_tf_fit_function(self.model.tf_asymmetry_simultaneous_function, 1), normalisation
        )

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

    def test_that_update_ws_fit_function_parameters_will_update_the_parameters_when_in_TF_asymmetry_simultaneous_mode(self):
        normalisation1, normalisation2 = 1.345, 2.345
        param1, param2 = 3.345, 4.345

        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True

        self.model.update_ws_fit_function_parameters(self.dataset_names, [normalisation1, param1, normalisation2, param2])
        self.assertEqual(
            self.model.get_fit_function_parameter_values(self.model.tf_asymmetry_simultaneous_function)[0],
            [normalisation1, 0.0, param1, 0.2, 0.2, normalisation2, 0.0, param2, 0.2, 0.2],
        )

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
            flattened_workspaces, fit_statuses, chi_squared
        )

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
        self.model._evaluate_sequential_fit.assert_called_once_with(None, workspaces, parameter_values, use_initial_values)
        self.model._update_fit_functions_after_sequential_fit.assert_called_once_with(workspaces, functions)
        self.model._update_fit_statuses_and_chi_squared_after_sequential_fit.assert_called_once_with(workspaces, fit_statuses, chi_squared)

    def test_that_are_same_workspaces_as_the_datasets_returns_true_if_all_the_dataset_names_match(self):
        self.model.dataset_names = self.dataset_names
        self.assertTrue(self.model._are_same_workspaces_as_the_datasets(self.dataset_names))

    def test_that_are_same_workspaces_as_the_datasets_returns_false_if_one_of_the_dataset_names_does_not_match(self):
        dataset_names = self.dataset_names + ["EMU20884; Group; bottom; Asymmetry"]
        self.model.dataset_names = self.dataset_names
        self.assertTrue(not self.model._are_same_workspaces_as_the_datasets(dataset_names))

    def test_that_validate_sequential_fit_returns_an_empty_message_when_the_data_provided_is_valid_in_tf_fitting(self):
        self.model.dataset_names = ["EMU20884; Group; bottom; Asymmetry", "EMU20884; Group; top; Asymmetry"]
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_mode = True

        message = self.model.validate_sequential_fit([self.model.dataset_names])

        self.assertEqual(message, "")

    def test_that_validate_sequential_fit_returns_an_error_message_for_pair_data_in_tf_asymmetry_fitting_mode(self):
        self.model.dataset_names = ["EMU20884; Pair Asym; bottom; Asymmetry", "EMU20884; Pair Asym; top; Asymmetry"]
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_mode = True

        message = self.model.validate_sequential_fit([self.model.dataset_names])

        self.assertEqual(
            message,
            "Only Groups can be fitted in TF Asymmetry mode. "
            "Please unselect the following Pairs/Diffs in the grouping tab: 'bottom', 'top'",
        )

    def test_that_get_all_fit_functions_for_returns_all_the_tf_single_functions_for_the_display_type_all(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_single_functions = [self.tf_single_function.clone(), self.tf_single_function.clone()]
        self.model.tf_asymmetry_mode = True

        filtered_functions = self.model.get_all_fit_functions_for("All")

        self.assertEqual(len(filtered_functions), len(self.model.single_fit_functions))
        self.assertEqual(self.model.get_fit_function_parameter_values(filtered_functions[0])[0], [0.0, 0.0, 0.0, 0.2, 0.2])
        self.assertEqual(self.model.get_fit_function_parameter_values(filtered_functions[1])[0], [0.0, 0.0, 0.0, 0.2, 0.2])

    def test_that_get_all_fit_functions_for_does_not_do_filtering_even_for_a_fwd_display_type(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.tf_asymmetry_single_functions = [self.tf_single_function.clone(), self.tf_single_function.clone()]
        self.model.tf_asymmetry_mode = True

        filtered_functions = self.model.get_all_fit_functions_for("fwd")

        self.assertEqual(len(filtered_functions), 2)
        self.assertEqual(self.model.get_fit_function_parameter_values(filtered_functions[0])[0], [0.0, 0.0, 0.0, 0.2, 0.2])
        self.assertEqual(self.model.get_fit_function_parameter_values(filtered_functions[1])[0], [0.0, 0.0, 0.0, 0.2, 0.2])

    def test_that_get_all_fit_functions_for_returns_the_tf_simultaneous_function_when_in_simultaneous_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.tf_asymmetry_simultaneous_function = self.tf_simultaneous_fit_function

        filtered_functions_all = self.model.get_all_fit_functions_for("All")
        self.assertEqual(str(filtered_functions_all[0]), str(self.tf_simultaneous_fit_function))

        filtered_functions_fwd = self.model.get_all_fit_functions_for("fwd")
        self.assertEqual(str(filtered_functions_fwd[0]), str(self.tf_simultaneous_fit_function))

    def test_get_normal_fit_function_from_does_not_error_if_the_function_has_an_unexpected_structure(self):
        function_string = (
            "composite=MultiDomainFunction,NumDeriv=true;(composite=CompositeFunction,NumDeriv=false,"
            "$domains=i;(composite=ProductFunction,NumDeriv=false;name=FlatBackground,A0=0.859525;"
            "(name=FlatBackground,A0=1,ties=(A0=1);name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0));"
            "name=ExpDecayMuon,A=0,Lambda=-2.19698,ties=(A=0,Lambda=-2.19698));(composite=CompositeFunction,"
            "NumDeriv=false,$domains=i;(composite=ProductFunction,NumDeriv=false;name=FlatBackground,"
            "A0=1.22426;(name=FlatBackground,A0=1,ties=(A0=1);name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0));"
            "name=ExpDecayMuon,A=0,Lambda=-2.19698,ties=(A=0,Lambda=-2.19698))"
        )
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fitting_mode = True
        self.model.tf_asymmetry_mode = True

        func = FunctionFactory.createInitialized(function_string)

        self.assertEqual(None, self.model._get_normal_fit_function_from(func))


if __name__ == "__main__":
    unittest.main()
