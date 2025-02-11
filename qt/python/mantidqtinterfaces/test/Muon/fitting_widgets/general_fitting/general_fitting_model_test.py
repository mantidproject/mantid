# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import AnalysisDataService, CompositeFunction, FrameworkManager, FunctionFactory
from mantid.simpleapi import CreateSampleWorkspace

from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model import GeneralFittingModel
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context


class GeneralFittingModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        context = setup_context()
        self.model = GeneralFittingModel(context, context.fitting_context)
        self.dataset_names = ["Name1", "Name2"]
        self.fit_function = FunctionFactory.createFunction("FlatBackground")
        self.single_fit_functions = [self.fit_function.clone(), self.fit_function.clone()]
        self.simultaneous_fit_function = FunctionFactory.createInitializedMultiDomainFunction(
            str(self.fit_function), len(self.dataset_names)
        )

    def tearDown(self):
        self.model = None

    def test_that_the_model_has_been_instantiated_with_empty_fit_data(self):
        self.assertEqual(self.model.dataset_names, [])
        self.assertEqual(self.model.current_dataset_index, None)
        self.assertEqual(self.model.start_xs, [])
        self.assertEqual(self.model.end_xs, [])
        self.assertEqual(self.model.single_fit_functions, [])
        self.assertEqual(self.model.fitting_context.single_fit_functions_for_undo, [])
        self.assertEqual(self.model.fit_statuses, [])
        self.assertEqual(self.model.chi_squared, [])
        self.assertEqual(self.model.function_name, "")
        self.assertTrue(self.model.function_name_auto_update)
        self.assertEqual(self.model.minimizer, "")
        self.assertEqual(self.model.evaluation_type, "")
        self.assertTrue(self.model.fit_to_raw)

        self.assertEqual(self.model.simultaneous_fit_function, None)
        self.assertEqual(self.model.fitting_context.simultaneous_fit_functions_for_undo, [])
        self.assertTrue(not self.model.simultaneous_fitting_mode)
        self.assertEqual(self.model.simultaneous_fit_by, "")
        self.assertEqual(self.model.simultaneous_fit_by_specifier, "")
        self.assertEqual(self.model.global_parameters, [])

    def test_that_it_is_possible_to_set_the_simultaneous_fit_function_as_expected(self):
        self.model.dataset_names = self.dataset_names

        self.model.simultaneous_fit_function = self.simultaneous_fit_function

        self.assertEqual(str(self.model.simultaneous_fit_function), str(self.simultaneous_fit_function))

    def test_that_setting_the_simultaneous_fit_function_to_an_ifunction_will_raise_if_the_number_of_datasets_is_zero(self):
        self.assertEqual(self.model.number_of_datasets, 0)
        with self.assertRaises(AssertionError):
            self.model.simultaneous_fit_function = self.simultaneous_fit_function

    def test_that_setting_the_simultaneous_fit_function_to_a_none_will_not_raise_if_the_number_of_datasets_is_zero(self):
        self.assertEqual(self.model.number_of_datasets, 0)
        self.model.simultaneous_fit_function = None

    def test_that_clear_simultaneous_fit_function_will_clear_the_simultaneous_fit_function_as_expected(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function

        self.model.clear_simultaneous_fit_function()

        self.assertEqual(self.model.simultaneous_fit_function, None)

    def test_that_it_is_possible_to_cache_the_simultaneous_fit_function_as_expected(self):
        self.model.dataset_names = self.dataset_names

        self.model.simultaneous_fit_function_cache = self.simultaneous_fit_function

        self.assertEqual(str(self.model.simultaneous_fit_function_cache), str(self.simultaneous_fit_function))

    def test_that_cache_the_current_fit_functions_will_cache_the_simultaneous_and_single_fit_functions(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fitting_mode = True
        self.model.simultaneous_fit_function = self.simultaneous_fit_function

        self.model.save_current_fit_function_to_undo_data()

        self.assertEqual(str(self.model.fitting_context.simultaneous_fit_functions_for_undo[0]), str(self.model.simultaneous_fit_function))

    def test_that_clear_undo_data_will_clear_the_simultaneous_fit_functions(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fitting_mode = True
        self.model.simultaneous_fit_function = self.simultaneous_fit_function

        self.model.save_current_fit_function_to_undo_data()

        self.assertEqual(len(self.model.fitting_context.simultaneous_fit_functions_for_undo), 1)
        self.assertEqual(str(self.model.fitting_context.simultaneous_fit_functions_for_undo[0]), str(self.simultaneous_fit_function))

        self.model.clear_undo_data()

        self.assertEqual(self.model.fitting_context.simultaneous_fit_functions_for_undo, [])

    def test_that_the_simultaneous_fitting_mode_can_be_changed_as_expected(self):
        self.assertTrue(not self.model.simultaneous_fitting_mode)
        self.model.simultaneous_fitting_mode = True
        self.assertTrue(self.model.simultaneous_fitting_mode)

    def test_that_the_simultaneous_fit_by_property_can_be_changed_as_expected(self):
        self.model.simultaneous_fit_by = "Group/Pair"
        self.assertEqual(self.model.simultaneous_fit_by, "Group/Pair")

        self.model.simultaneous_fit_by = "Run"
        self.assertEqual(self.model.simultaneous_fit_by, "Run")

    def test_that_the_simultaneous_fit_by_specifier_property_can_be_changed_as_expected(self):
        self.model.simultaneous_fit_by_specifier = "long"
        self.assertEqual(self.model.simultaneous_fit_by_specifier, "long")

        self.model.simultaneous_fit_by_specifier = "fwd"
        self.assertEqual(self.model.simultaneous_fit_by_specifier, "fwd")

    def test_that_the_global_parameters_property_can_be_changed_as_expected(self):
        self.model.global_parameters = ["A0"]
        self.assertEqual(self.model.global_parameters, ["A0"])

        self.model.global_parameters = []
        self.assertEqual(self.model.global_parameters, [])

    def test_that_automatically_update_function_name_will_set_the_correct_function_name_for_single_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.simultaneous_fitting_mode = False

        self.assertEqual(self.model.function_name, "")
        self.model.automatically_update_function_name()

        self.assertEqual(self.model.function_name, " FlatBackground")

    def test_that_automatically_update_function_name_wont_set_the_name_if_automatic_update_is_off_in_single_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.simultaneous_fitting_mode = False
        self.model.function_name_auto_update = False

        self.assertEqual(self.model.function_name, "")
        self.model.automatically_update_function_name()

        self.assertEqual(self.model.function_name, "")

    def test_that_automatically_update_function_name_will_set_the_correct_function_name_for_simultaneous_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True

        self.assertEqual(self.model.function_name, "")
        self.model.automatically_update_function_name()

        self.assertEqual(self.model.function_name, " FlatBackground")

    def test_that_automatically_update_function_name_wont_set_the_name_if_automatic_update_is_off_in_simultaneous_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.function_name_auto_update = False

        self.assertEqual(self.model.function_name, "")
        self.model.automatically_update_function_name()

        self.assertEqual(self.model.function_name, "")

    def test_that_automatically_update_function_name_will_set_the_correct_function_name_for_a_composite_function(self):
        composite = CompositeFunction()
        composite.add(self.fit_function.clone())
        f2 = FunctionFactory.createFunction("ExpDecay")
        composite.add(f2)

        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = [composite.clone(), composite.clone()]

        self.assertEqual(self.model.function_name, "")
        self.model.automatically_update_function_name()

        self.assertEqual(self.model.function_name, " FlatBackground,ExpDecay")

    def test_that_use_cached_function_will_replace_the_simultaneous_and_single_functions_with_the_cached_functions(self):
        self.model.fitting_context.undo_previous_fit = mock.Mock()

        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.save_current_fit_function_to_undo_data()
        self.model.simultaneous_fit_function = None

        self.model.undo_previous_fit()

        self.assertEqual(str(self.model.simultaneous_fit_function), str(self.simultaneous_fit_function))

    def test_that_update_parameter_value_will_update_the_value_of_a_parameter_in_single_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = [self.fit_function, None]

        self.model.update_parameter_value("A0", 5.0)

        self.assertEqual(str(self.model.current_single_fit_function), "name=FlatBackground,A0=5")

    def test_that_update_parameter_value_will_update_the_value_of_a_parameter_in_simultaneous_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.current_dataset_index = 1

        self.model.update_parameter_value("A0", 5.0)

        self.assertEqual(
            str(self.model.simultaneous_fit_function),
            "composite=MultiDomainFunction,NumDeriv=true;name=FlatBackground,A0=0,$domains=i;name=FlatBackground,A0=5,$domains=i",
        )

    def test_that_update_attribute_value_will_update_the_value_of_a_parameter_in_single_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = [FunctionFactory.createFunction("Chebyshev"), None]

        self.model.update_attribute_value("StartX", 0.0)
        self.model.update_attribute_value("EndX", 15.0)

        self.assertEqual(str(self.model.current_single_fit_function), "name=Chebyshev,EndX=15,StartX=0,n=0,A0=0")

    def test_that_update_attribute_value_will_update_the_value_of_a_parameter_in_simultaneous_fit_mode(self):
        self.model.dataset_names = self.dataset_names

        self.fit_function = FunctionFactory.createFunction("Chebyshev")
        self.model.simultaneous_fit_function = FunctionFactory.createInitializedMultiDomainFunction(
            str(self.fit_function), len(self.dataset_names)
        )

        self.model.simultaneous_fitting_mode = True
        self.model.current_dataset_index = 1

        self.model.update_attribute_value("StartX", 0.0)
        self.model.update_attribute_value("EndX", 15.0)

        self.assertEqual(
            str(self.model.simultaneous_fit_function),
            "composite=MultiDomainFunction,NumDeriv=true;"
            "name=Chebyshev,EndX=1,StartX=-1,n=0,A0=0,$domains=i;"
            "name=Chebyshev,EndX=15,StartX=0,n=0,A0=0,$domains=i",
        )

    def test_that_setting_new_dataset_names_will_reset_the_fit_functions_but_attempt_to_use_the_previous_function(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.simultaneous_fit_function = self.simultaneous_fit_function

        self.model.dataset_names = ["Name3"]

        self.assertEqual(len(self.model.single_fit_functions), 1)
        self.assertEqual(str(self.model.single_fit_functions[0]), "name=FlatBackground,A0=0")
        self.assertEqual(str(self.model.simultaneous_fit_function), "name=FlatBackground,A0=0")

    @mock.patch(
        "mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model.GeneralFittingModel._get_selected_runs"
    )
    def test_that_get_simultaneous_fit_by_specifiers_to_display_from_context_attempts_to_get_the_runs(self, mock_get_runs):
        self.model.simultaneous_fit_by = "Run"

        self.model.get_simultaneous_fit_by_specifiers_to_display_from_context()
        mock_get_runs.assert_called_with()

        self.assertEqual(1, mock_get_runs.call_count)

    @mock.patch(
        "mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model.GeneralFittingModel."
        "_get_selected_groups_and_pairs"
    )
    def test_that_get_simultaneous_fit_by_specifiers_to_display_from_context_attempts_to_get_the_groups_pairs(self, mock_get_group_pairs):
        self.model.simultaneous_fit_by = "Group/Pair"

        self.model.get_simultaneous_fit_by_specifiers_to_display_from_context()
        mock_get_group_pairs.assert_called_with()

        self.assertEqual(1, mock_get_group_pairs.call_count)

    def test_that_get_simultaneous_fit_by_specifiers_to_display_from_context_returns_an_empty_list_if_fit_by_is_not_specified(self):
        self.assertEqual(self.model.get_simultaneous_fit_by_specifiers_to_display_from_context(), [])

    def test_that_get_workspace_names_to_display_from_context_will_attempt_to_get_runs_and_groups_for_single_fit_mode(self):
        workspace_names = ["Name"]
        workspace = CreateSampleWorkspace()
        AnalysisDataService.addOrReplace("Name", workspace)
        runs = "All"
        group_or_pair = ["long"]
        self.model.simultaneous_fitting_mode = False

        self.model._get_selected_groups_and_pairs = mock.MagicMock(return_value=(group_or_pair))

        self.model.context.get_workspace_names_for = mock.MagicMock(return_value=workspace_names)
        self.model.context.get_workspace_names_of_fit_data_with_run = mock.MagicMock()

        self.assertEqual(self.model.get_workspace_names_to_display_from_context(), workspace_names)

        self.model._get_selected_groups_and_pairs.assert_called_with()
        self.model.context.get_workspace_names_for.assert_called_with(runs, group_or_pair, True)

        self.assertEqual(1, self.model._get_selected_groups_and_pairs.call_count)
        self.assertEqual(1, self.model.context.get_workspace_names_for.call_count)

    def test_that_get_workspace_names_to_display_will_attempt_to_get_runs_and_groups_for_simultaneous_fit_mode(self):
        workspace_names = ["Name"]
        workspace = CreateSampleWorkspace()
        AnalysisDataService.addOrReplace("Name", workspace)
        runs = ["62260"]
        group_or_pair = ["long"]
        self.model.simultaneous_fitting_mode = True

        self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode = mock.MagicMock(return_value=(runs, group_or_pair))

        self.model.context.get_workspace_names_for = mock.MagicMock(return_value=workspace_names)
        self.model.context.get_workspace_names_of_fit_data_with_run = mock.MagicMock()

        self.assertEqual(self.model.get_workspace_names_to_display_from_context(), workspace_names)

        self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode.assert_called_with()
        self.model.context.get_workspace_names_for.assert_called_with(runs, group_or_pair, True)

        self.assertEqual(1, self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode.call_count)
        self.assertEqual(1, self.model.context.get_workspace_names_for.call_count)

    def test_that_get_selected_runs_groups_and_pairs_will_attempt_to_get_runs_and_groups_for_simultaneous_fit_mode(self):
        runs = ["62260"]
        group_or_pair = "long"
        self.model.simultaneous_fitting_mode = True

        self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode = mock.MagicMock(return_value=(runs, [group_or_pair]))

        output_runs, output_group_pairs = self.model.get_selected_runs_groups_and_pairs()
        self.assertEqual(output_runs, runs)
        self.assertEqual(output_group_pairs, [group_or_pair])

        self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode.assert_called_with()

        self.assertEqual(1, self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode.call_count)

    def test_that_get_selected_runs_groups_and_pairs_will_attempt_to_get_run_and_groups_for_single_fit_mode(self):
        run = "62260"
        group_or_pair = "long"
        self.model.simultaneous_fitting_mode = True
        self.model.simultaneous_fit_by = "Run"
        self.model.simultaneous_fit_by_specifier = run

        self.model._get_selected_groups_and_pairs = mock.MagicMock(return_value=[group_or_pair])

        output_runs, output_group_pairs = self.model.get_selected_runs_groups_and_pairs()
        self.assertEqual(output_runs, run)
        self.assertEqual(output_group_pairs, [group_or_pair])

        self.model._get_selected_groups_and_pairs.assert_called_with()
        self.assertEqual(1, self.model._get_selected_groups_and_pairs.call_count)

    def test_that_get_active_fit_function_will_return_the_simultaneous_fit_function_if_in_simultaneous_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True

        active_function = self.model.get_active_fit_function()

        self.assertEqual(active_function, self.model.simultaneous_fit_function)

    def test_that_get_active_fit_function_will_return_the_current_single_fit_function_if_in_single_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = False

        active_function = self.model.get_active_fit_function()

        self.assertEqual(active_function, self.model.current_single_fit_function)

    def test_that_get_active_workspace_names_will_return_all_the_dataset_names_if_in_simultaneous_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fitting_mode = True

        active_workspace_names = self.model.get_active_workspace_names()

        self.assertEqual(active_workspace_names, self.model.dataset_names)

    def test_that_get_active_workspace_names_will_return_the_current_dataset_name_if_in_single_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fitting_mode = False

        active_workspace_names = self.model.get_active_workspace_names()

        self.assertEqual(active_workspace_names, [self.model.current_dataset_name])

    def test_perform_fit_will_call_the_correct_function_for_a_single_fit(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.simultaneous_fitting_mode = False

        self.model._do_single_fit = mock.MagicMock(return_value=(self.model.current_single_fit_function, "success", 0.56))

        self.model.perform_fit()

        self.model._do_single_fit.assert_called_once_with(
            self.model._get_parameters_for_single_fit(self.model.current_dataset_name, self.model.current_single_fit_function)
        )

    def test_perform_fit_will_call_the_correct_function_for_a_simultaneous_fit(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        global_parameters = ["A0"]
        self.model.global_parameters = global_parameters

        self.model._do_simultaneous_fit = mock.MagicMock(return_value=(self.model.simultaneous_fit_function, "success", 0.56))

        self.model.perform_fit()

        self.model._do_simultaneous_fit.assert_called_once_with(
            self.model._get_parameters_for_simultaneous_fit(self.model.dataset_names, self.model.simultaneous_fit_function),
            global_parameters,
        )

    def test_that_get_fit_function_parameters_will_return_a_list_of_parameter_names_when_in_single_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions

        self.assertEqual(self.model.get_fit_function_parameters(), ["A0"])

        self.fit_function = FunctionFactory.createFunction("ExpDecay")
        self.model.single_fit_functions = [self.fit_function.clone(), self.fit_function.clone()]

        self.assertEqual(self.model.get_fit_function_parameters(), ["Height", "Lifetime"])

    def test_that_get_fit_function_parameters_will_return_a_list_of_parameter_names_when_in_simultaneous_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True

        self.assertEqual(self.model.get_fit_function_parameters(), ["f0.A0", "f1.A0"])

    def test_that_get_fit_function_parameters_returns_an_empty_list_if_the_simultaneous_function_is_none_in_simultaneous_fit_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fitting_mode = True

        self.assertEqual(self.model.get_fit_function_parameters(), [])

    def test_that_get_all_fit_functions_returns_the_simultaneous_fitting_function_in_a_list_when_in_simultaneous_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True

        self.assertEqual(self.model.get_all_fit_functions(), [self.model.simultaneous_fit_function])

    def test_that_the_existing_functions_are_reused_when_new_datasets_are_loaded(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.current_dataset_index = 0
        self.model.simultaneous_fit_function.setParameter("f0.A0", 1)
        self.model.simultaneous_fit_function.setParameter("f1.A0", 5)

        self.model.dataset_names = ["New Name1", "New Name2"]

        self.assertEqual(
            str(self.model.simultaneous_fit_function),
            "composite=MultiDomainFunction,NumDeriv=true;name=FlatBackground,A0=1,$domains=i;name=FlatBackground,A0=5,$domains=i",
        )

    def test_that_the_currently_selected_function_is_copied_for_when_a_larger_number_of_new_datasets_are_loaded(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.current_dataset_index = 0
        self.model.simultaneous_fit_function.setParameter("f0.A0", 1)
        self.model.simultaneous_fit_function.setParameter("f1.A0", 5)

        # The last two datasets are the same as the previously loaded datasets. This means their functions should be
        # reused for these last two domains. The first two domain functions are completely new, and so
        # are just given a copy of the previous currently selected function (i.e. A0=1).
        self.model.dataset_names = ["New Name1", "New Name2", "Name1", "Name2"]

        self.assertEqual(
            str(self.model.simultaneous_fit_function),
            "composite=MultiDomainFunction,NumDeriv=true;"
            "name=FlatBackground,A0=1,$domains=i;"
            "name=FlatBackground,A0=1,$domains=i;"
            "name=FlatBackground,A0=1,$domains=i;"
            "name=FlatBackground,A0=5,$domains=i",
        )

    def test_that_newly_loaded_datasets_will_reuse_the_existing_functions_when_there_are_fewer_new_datasets(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        self.model.current_dataset_index = 0
        self.model.simultaneous_fit_function.setParameter("f0.A0", 1)
        self.model.simultaneous_fit_function.setParameter("f1.A0", 5)

        # This dataset is the second dataset previously and so the A0=5 fit function should be reused. There is also
        # now only one domain, so a MultiDomainFunction is no longer used.
        self.model.dataset_names = ["Name2"]
        self.assertEqual(str(self.model.simultaneous_fit_function), "name=FlatBackground,A0=5")

    def test_that_reseting_the_simultaneous_fit_function_does_not_crash_when_there_is_a_single_domain(self):
        self.model.dataset_names = ["Name1"]
        self.model.simultaneous_fit_function = self.fit_function
        self.model.simultaneous_fitting_mode = False
        self.model.current_dataset_index = 0
        self.model.simultaneous_fit_function.setParameter("A0", 5)

        self.model.simultaneous_fitting_mode = True
        self.model.dataset_names = ["Name2"]

        self.assertEqual(str(self.model.simultaneous_fit_function), "name=FlatBackground,A0=5")

    """
    Tests for functions used by the Sequential fitting tab.
    """

    def test_that_get_runs_groups_and_pairs_for_fits_will_attempt_to_get_it_by_runs_when_in_simultaneous_fitting_mode(self):
        self.model._get_runs_groups_and_pairs_for_simultaneous_fit_by_runs = mock.Mock(return_value=(["62260"], ["fwd;bwd;long"]))
        self.model.simultaneous_fitting_mode = True
        self.model.simultaneous_fit_by = "Run"

        runs, groups_and_pairs = self.model.get_runs_groups_and_pairs_for_fits("All")
        self.assertEqual(runs, ["62260"])
        self.assertEqual(groups_and_pairs, ["fwd;bwd;long"])

        self.model._get_runs_groups_and_pairs_for_simultaneous_fit_by_runs.assert_called_once_with("All")

    def test_that_get_runs_groups_and_pairs_for_fits_will_attempt_to_get_it_by_groups_when_in_simultaneous_fitting_mode(self):
        self.model._get_runs_groups_and_pairs_for_simultaneous_fit_by_groups_and_pairs = mock.Mock(
            return_value=(["62260;62261;62262"], ["fwd", "bwd"])
        )
        self.model.simultaneous_fitting_mode = True
        self.model.simultaneous_fit_by = "Group/Pair"

        runs, groups_and_pairs = self.model.get_runs_groups_and_pairs_for_fits("All")
        self.assertEqual(runs, ["62260;62261;62262"])
        self.assertEqual(groups_and_pairs, ["fwd", "bwd"])

        self.model._get_runs_groups_and_pairs_for_simultaneous_fit_by_groups_and_pairs.assert_called_once_with("All")

    def test_that_get_all_fit_functions_for_will_return_the_simultaneous_fit_function_when_in_simultaneous_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True

        filtered_function_all = self.model.get_all_fit_functions_for("All")
        self.assertEqual(self.model.get_fit_function_parameter_values(filtered_function_all[0]), ([0.0, 0.0], [0.0, 0.0]))

        filtered_function_fwd = self.model.get_all_fit_functions_for("fwd")
        self.assertEqual(self.model.get_fit_function_parameter_values(filtered_function_fwd[0]), ([0.0, 0.0], [0.0, 0.0]))

    def test_that_get_fit_function_parameter_values_will_return_the_parameter_values_in_the_specified_function(self):
        self.assertEqual(self.model.get_fit_function_parameter_values(self.simultaneous_fit_function)[0], [0.0, 0.0])

    def test_that_update_ws_fit_function_parameters_will_update_the_parameters_when_in_simultaneous_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True

        self.model.update_ws_fit_function_parameters(self.model.dataset_names[:1], [1.0, 0.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.simultaneous_fit_function), ([1.0, 0.0], [0.0, 0.0]))

        self.model.update_ws_fit_function_parameters(self.model.dataset_names[1:], [1.0, 2.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.simultaneous_fit_function), ([1.0, 2.0], [0.0, 0.0]))

    def test_remove_non_existing_global_parameters_will_remove_non_existing_ties(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True

        # Add a non-existing parameter as a global parameter
        self.model.fitting_context.global_parameters = ["f0.A0"]

        self.model._remove_non_existing_global_parameters()

        self.assertEqual([], self.model.fitting_context.global_parameters)

    def test_remove_non_existing_global_parameters_will_not_remove_existing_ties(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True

        # Add an existing parameter as a global parameter
        self.model.fitting_context.global_parameters = ["A0"]

        self.model._remove_non_existing_global_parameters()

        self.assertEqual(["A0"], self.model.fitting_context.global_parameters)

    def test_add_global_ties_to_simultaneous_function_will_remove_non_existing_ties_before_adding_ties(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True

        # Add a non-existing parameter as a global parameter
        self.model.fitting_context.global_parameters = ["f0.A0"]

        # There should be no error on this line
        self.model._add_global_ties_to_simultaneous_function()

        self.assertEqual([], self.model.fitting_context.global_parameters)

    def test_add_global_ties_to_simultaneous_function_will_raise_assertion_if_the_simultaneous_function_is_none(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = None
        self.model.simultaneous_fitting_mode = True

        with self.assertRaisesRegex(AssertionError, "This method assumes the simultaneous fit function is not None."):
            self.model._add_global_ties_to_simultaneous_function()

    def test_get_new_domain_function_for_does_not_error_if_the_simultaneous_function_is_not_a_composite(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.fit_function

        self.model._get_new_domain_function_for(self.dataset_names[0])

    def test_get_new_domain_functions_using_existing_datasets_does_not_error_if_the_simultaneous_function_is_not_a_composite(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.fit_function

        self.model._get_new_domain_functions_using_existing_datasets(self.dataset_names)


if __name__ == "__main__":
    unittest.main()
