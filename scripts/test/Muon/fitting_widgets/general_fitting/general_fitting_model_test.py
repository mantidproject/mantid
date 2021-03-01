# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import CompositeFunction, FrameworkManager, FunctionFactory

from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model import GeneralFittingModel
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_base_pair import MuonBasePair
from Muon.GUI.Common.test_helpers.context_setup import setup_context


class GeneralFittingModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        self.model = GeneralFittingModel(setup_context())
        self.dataset_names = ["Name1", "Name2"]
        self.fit_function = FunctionFactory.createFunction("FlatBackground")
        self.single_fit_functions = [self.fit_function.clone(), self.fit_function.clone()]
        self.simultaneous_fit_function = FunctionFactory.createInitializedMultiDomainFunction(str(self.fit_function),
                                                                                              len(self.dataset_names))

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

        self.assertEqual(self.model.simultaneous_fit_function, None)
        self.assertEqual(self.model.simultaneous_fit_function_cache, None)
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
        with self.assertRaises(RuntimeError):
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

    def test_that_caching_the_simultaneous_fit_function_to_an_ifunction_will_raise_if_the_number_of_datasets_is_zero(self):
        self.assertEqual(self.model.number_of_datasets, 0)
        with self.assertRaises(RuntimeError):
            self.model.simultaneous_fit_function_cache = self.simultaneous_fit_function

    def test_that_caching_the_simultaneous_fit_function_to_a_none_will_not_raise_if_the_number_of_functions_is_zero(self):
        self.assertEqual(self.model.number_of_datasets, 0)
        self.model.simultaneous_fit_function_cache = None

    def test_that_cache_the_current_fit_functions_will_cache_the_simultaneous_and_single_fit_functions(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.simultaneous_fit_function = self.simultaneous_fit_function

        self.model.cache_the_current_fit_functions()

        self.assertEqual(str(self.model.single_fit_functions_cache[0]), str(self.model.single_fit_functions[0]))
        self.assertEqual(str(self.model.single_fit_functions_cache[1]), str(self.model.single_fit_functions[1]))
        self.assertEqual(str(self.model.simultaneous_fit_function_cache), str(self.model.simultaneous_fit_function))

    def test_that_clear_the_fit_function_cache_will_clear_the_simultaneous_and_single_fit_functions(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions_cache = self.single_fit_functions
        self.model.simultaneous_fit_function_cache = self.simultaneous_fit_function

        self.assertEqual(len(self.model.single_fit_functions_cache), 2)
        self.assertEqual(self.model.simultaneous_fit_function_cache, self.simultaneous_fit_function)

        self.model.clear_cached_fit_functions()

        self.assertEqual(self.model.single_fit_functions_cache, [None, None])
        self.assertEqual(self.model.simultaneous_fit_function_cache, None)

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
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = [self.fit_function, None]
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.cache_the_current_fit_functions()
        self.model.single_fit_functions = [None, None]
        self.model.simultaneous_fit_function = None

        self.model.use_cached_function()

        self.assertEqual(str(self.model.single_fit_functions[0]), "name=FlatBackground,A0=0")
        self.assertEqual(self.model.single_fit_functions[1], None)
        self.assertEqual(str(self.model.simultaneous_fit_function), str(self.simultaneous_fit_function))

    def test_that_setting_new_dataset_names_will_reset_the_fit_functions_but_attempt_to_use_the_previous_function(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.simultaneous_fit_function = self.simultaneous_fit_function

        self.model.dataset_names = ["Name3"]

        self.assertEqual(len(self.model.single_fit_functions), 1)
        self.assertEqual(str(self.model.single_fit_functions[0]), "name=FlatBackground,A0=0")
        self.assertEqual(str(self.model.simultaneous_fit_function), "name=FlatBackground,A0=0")

    @mock.patch('Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model.GeneralFittingModel.'
                '_get_selected_runs')
    def test_that_get_simultaneous_fit_by_specifiers_to_display_from_context_attempts_to_get_the_runs(self,
                                                                                                      mock_get_runs):
        self.model.simultaneous_fit_by = "Run"

        self.model.get_simultaneous_fit_by_specifiers_to_display_from_context()
        mock_get_runs.assert_called_with()

        self.assertEqual(1, mock_get_runs.call_count)

    @mock.patch('Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model.GeneralFittingModel.'
                '_get_selected_groups_and_pairs')
    def test_that_get_simultaneous_fit_by_specifiers_to_display_from_context_attempts_to_get_the_groups_pairs(self,
                                                                                                              mock_get_group_pairs):
        self.model.simultaneous_fit_by = "Group/Pair"

        self.model.get_simultaneous_fit_by_specifiers_to_display_from_context()
        mock_get_group_pairs.assert_called_with()

        self.assertEqual(1, mock_get_group_pairs.call_count)

    def test_that_get_simultaneous_fit_by_specifiers_to_display_from_context_returns_an_empty_list_if_fit_by_is_not_specified(self):
        self.assertEqual(self.model.get_simultaneous_fit_by_specifiers_to_display_from_context(), [])

    @mock.patch('Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model.GeneralFittingModel.'
                '_get_workspace_names_to_display_from_context')
    @mock.patch('Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model.GeneralFittingModel.'
                '_sort_workspace_names')
    def test_that_get_workspace_names_to_display_from_context_will_attempt_to_get_runs_and_groups_for_single_fit_mode(self,
                                                                                                                      mock_sort,
                                                                                                                      mock_get_workspaces):
        workspace_names = ["Name"]
        runs = ["62260"]
        group_or_pair = "long"
        self.model.simultaneous_fitting_mode = False

        self.model._get_selected_runs_groups_and_pairs_for_single_fit_mode = \
            mock.MagicMock(return_value=(runs, [group_or_pair]))

        mock_get_workspaces.return_value = workspace_names
        mock_sort.return_value = workspace_names

        self.assertEqual(self.model.get_workspace_names_to_display_from_context(), workspace_names)

        self.model._get_selected_runs_groups_and_pairs_for_single_fit_mode.assert_called_with()
        mock_get_workspaces.assert_called_with(runs, group_or_pair)
        mock_sort.assert_called_with(workspace_names)

        self.assertEqual(1, self.model._get_selected_runs_groups_and_pairs_for_single_fit_mode.call_count)
        self.assertEqual(1, mock_get_workspaces.call_count)
        self.assertEqual(1, mock_sort.call_count)

    @mock.patch('Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model.GeneralFittingModel.'
                '_get_workspace_names_to_display_from_context')
    @mock.patch('Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model.GeneralFittingModel.'
                '_sort_workspace_names')
    def test_that_get_workspace_names_to_display_will_attempt_to_get_runs_and_groups_for_simultaneous_fit_mode(self,
                                                                                                               mock_sort,
                                                                                                               mock_get_workspaces):
        workspace_names = ["Name"]
        runs = ["62260"]
        group_or_pair = "long"
        self.model.simultaneous_fitting_mode = True

        self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode = \
            mock.MagicMock(return_value=(runs, [group_or_pair]))

        mock_get_workspaces.return_value = workspace_names
        mock_sort.return_value = workspace_names

        self.assertEqual(self.model.get_workspace_names_to_display_from_context(), workspace_names)

        self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode.assert_called_with()
        mock_get_workspaces.assert_called_with(runs, group_or_pair)
        mock_sort.assert_called_with(workspace_names)

        self.assertEqual(1, self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode.call_count)
        self.assertEqual(1, mock_get_workspaces.call_count)
        self.assertEqual(1, mock_sort.call_count)

    def test_that_get_selected_runs_groups_and_pairs_will_attempt_to_get_runs_and_groups_for_simultaneous_fit_mode(self):
        runs = ["62260"]
        group_or_pair = "long"
        self.model.simultaneous_fitting_mode = True

        self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode = \
            mock.MagicMock(return_value=(runs, [group_or_pair]))

        output_runs, output_group_pairs = self.model.get_selected_runs_groups_and_pairs()
        self.assertEqual(output_runs, runs)
        self.assertEqual(output_group_pairs, [group_or_pair])

        self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode.assert_called_with()

        self.assertEqual(1, self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode.call_count)

    def test_that_get_selected_runs_groups_and_pairs_will_attempt_to_get_runs_and_groups_for_single_fit_mode(self):
        runs = ["62260"]
        group_or_pair = "long"
        self.model.simultaneous_fitting_mode = True

        self.model._get_selected_runs_groups_and_pairs_for_single_fit_mode = \
            mock.MagicMock(return_value=(runs, [group_or_pair]))

        output_runs, output_group_pairs = self.model.get_selected_runs_groups_and_pairs()
        self.assertEqual(output_runs, runs)
        self.assertEqual(output_group_pairs, [group_or_pair])

        self.model._get_selected_runs_groups_and_pairs_for_single_fit_mode.assert_called_with()

        self.assertEqual(1, self.model._get_selected_runs_groups_and_pairs_for_single_fit_mode.call_count)

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

        self.model._do_single_fit = mock.MagicMock(return_value=(self.model.current_single_fit_function, "success",
                                                                 0.56))

        self.model.perform_fit()

        self.model._do_single_fit.assert_called_once_with(self.model._get_parameters_for_single_fit())

    def test_perform_fit_will_call_the_correct_function_for_a_simultaneous_fit(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True
        global_parameters = ["A0"]
        self.model.global_parameters = global_parameters

        self.model._do_simultaneous_fit = mock.MagicMock(return_value=(self.model.simultaneous_fit_function, "success",
                                                                       0.56))

        self.model.perform_fit()

        self.model._do_simultaneous_fit.assert_called_once_with(self.model._get_parameters_for_simultaneous_fit(),
                                                                global_parameters)

    def test_that_get_fit_function_parameter_values_will_return_the_parameter_values_in_the_specified_function(self):
        self.assertEqual(self.model.get_fit_function_parameter_values(self.single_fit_functions[0]), [0.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.simultaneous_fit_function), [0.0, 0.0])

    def test_that_update_ws_fit_function_parameters_will_update_the_parameters_for_the_specified_dataset_in_single_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.single_fit_functions = self.single_fit_functions
        self.model.simultaneous_fitting_mode = False

        self.model.update_ws_fit_function_parameters(["Name1"], [1.0])

        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.single_fit_functions[0]), [1.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.single_fit_functions[1]), [0.0])

    def test_that_update_ws_fit_function_parameters_will_update_the_parameters_for_the_specified_dataset_in_simultaneous_mode(self):
        self.model.dataset_names = self.dataset_names
        self.model.simultaneous_fit_function = self.simultaneous_fit_function
        self.model.simultaneous_fitting_mode = True

        self.model.update_ws_fit_function_parameters(["Name1"], [1.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.simultaneous_fit_function), [1.0, 0.0])

        self.model.update_ws_fit_function_parameters(["Name2"], [2.0])
        self.assertEqual(self.model.get_fit_function_parameter_values(self.model.simultaneous_fit_function), [1.0, 2.0])

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


if __name__ == '__main__':
    unittest.main()
