# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import AnalysisDataService, FrameworkManager, FunctionFactory, WorkspaceFactory

from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.ADS_calls import add_ws_to_ads, check_if_workspace_exist, retrieve_ws
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_model import ModelFittingModel
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context


def create_results_table():
    table = WorkspaceFactory.createTable()

    table.addColumn(type="str", name="workspace_name")
    table.addColumn(type="float", name="A0")
    table.addColumn(type="float", name="A1")
    table.addColumn(type="float", name="Sigma")
    table.addColumn(type="float", name="Lambda")
    table.addColumn(type="float", name="f1.Sigma")
    table.addColumn(type="float", name="f1.Lambda")
    table.addRow(["MUSR62260; Group; bottom; Asymmetry; MA", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6])
    table.addRow(["MUSR62260; Group; top; Asymmetry; MA", 0.3, 0.4, 0.5, 0.6, 0.7, 0.8])
    table.addRow(["MUSR62260; Group; fwd; Asymmetry; MA", 0.5, 0.6, 0.7, 0.8, 0.9, 1.0])
    return table


class ModelFittingModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        context = setup_context()
        self.model = ModelFittingModel(context, context.model_fitting_context)
        self.result_table_names = ["Result1", "Result2"]
        self.dataset_names = ["workspace_name_A0", "workspace_name_A1", "A0_A1", "Sigma_Lambda", "f1.Sigma_f1.Lambda"]
        self.fit_function1 = FunctionFactory.createFunction("FlatBackground")
        self.fit_function2 = FunctionFactory.createFunction("LinearBackground")
        self.single_fit_functions = [self.fit_function1.clone(), self.fit_function1.clone(), self.fit_function2.clone()]

    def tearDown(self):
        self.model = None
        AnalysisDataService.clear()

    def test_that_the_model_has_been_instantiated_with_empty_fit_data(self):
        self.assertEqual(self.model.result_table_names, [])
        self.assertEqual(self.model.current_result_table_index, None)

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

    def test_that_result_table_names_will_set_the_names_of_the_result_tables_as_expected(self):
        self.model.result_table_names = self.result_table_names
        self.assertEqual(self.model.result_table_names, self.result_table_names)

    def test_that_current_result_table_index_will_change_which_results_table_is_selected(self):
        self.model.result_table_names = ["Result1", "Result2", "Result3", "Result4", "Result5", "Result6"]

        for table_index in range(len(self.model.result_table_names)):
            self.model.current_result_table_index = table_index
            self.assertEqual(self.model.current_result_table_name, self.model.result_table_names[self.model.current_result_table_index])

    def test_that_current_result_table_name_will_return_none_when_there_are_no_results_tables_loaded(self):
        self.assertEqual(self.model.current_result_table_name, None)

    def test_that_current_result_table_index_will_raise_if_the_index_is_greater_than_or_equal_to_the_number_of_tables(self):
        self.model.result_table_names = self.result_table_names
        with self.assertRaises(AssertionError):
            self.model.current_result_table_index = 2

    def test_that_current_result_table_index_will_set_the_current_dataset_index_as_expected(self):
        self.model.result_table_names = self.result_table_names

        self.model.current_result_table_index = 1

        self.assertEqual(self.model.current_result_table_index, 1)

    def test_that_the_current_result_table_index_will_be_reset_to_none_if_there_are_no_results_tables_anymore(self):
        self.model.result_table_names = self.result_table_names
        self.model.current_result_table_index = 1
        self.assertEqual(self.model.current_result_table_index, 1)

        self.model.result_table_names = []

        self.assertEqual(self.model.current_result_table_index, None)

    def test_that_the_current_result_table_index_will_be_reset_to_zero_it_has_gone_out_of_range_but_theres_still_data_left(self):
        self.model.result_table_names = self.result_table_names
        self.model.current_result_table_index = 1
        self.assertEqual(self.model.current_result_table_index, 1)

        self.model.result_table_names = ["Name3"]

        self.assertEqual(self.model.current_result_table_index, 0)

    def test_that_parameter_combination_workspace_name_returns_the_expected_name_for_the_provided_parameters(self):
        self.model.result_table_names = self.result_table_names
        self.assertEqual(self.model.parameter_combination_workspace_name("A0", "A1"), "Result1; A1 vs A0")

    def test_that_parameter_combination_workspace_name_returns_none_when_there_are_no_results_tables(self):
        self.assertEqual(self.model.parameter_combination_workspace_name("A0", "A1"), None)

    def test_that_parameter_combination_group_name_returns_the_expected_name_for_the_provided_parameters(self):
        self.model.result_table_names = self.result_table_names
        self.assertEqual(self.model.parameter_combination_group_name(), "Result1; Parameter Combinations")

    def test_that_parameter_combination_group_name_returns_none_when_there_are_no_results_tables(self):
        self.assertEqual(self.model.parameter_combination_group_name(), None)

    def test_that_get_workspace_names_to_display_from_context_only_returns_the_names_that_exist_in_the_ADS(self):
        self.model.context.results_context.result_table_names = self.result_table_names

        table_workspace = WorkspaceFactory.createTable()
        add_ws_to_ads("Result1", table_workspace)

        self.assertEqual(self.model.get_workspace_names_to_display_from_context(), ["Result1"])

    def test_that_create_x_and_y_parameter_combinations_will_create_the_expected_parameter_data(self):
        self.model.result_table_names = self.result_table_names
        self.model.current_result_table_index = 0

        table = create_results_table()
        add_ws_to_ads("Result1", table)

        self.model.create_x_and_y_parameter_combinations()
        x_parameters = self.model.x_parameters()
        y_parameters = self.model.y_parameters()

        self.assertEqual(list(x_parameters), ["workspace_name", "A0", "A1", "Sigma", "Lambda", "f1.Sigma", "f1.Lambda"])
        self.assertEqual(list(y_parameters), ["workspace_name", "A0", "A1", "Sigma", "Lambda", "f1.Sigma", "f1.Lambda"])

        self.assertAlmostEqual(self.model.fitting_context.x_parameters["A0"][0][0], 0.1, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.x_parameters["A0"][0][1], 0.3, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.x_parameters["A0"][0][2], 0.5, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["A1"][0][0], 0.2, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["A1"][0][1], 0.4, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["A1"][0][2], 0.6, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["Sigma"][0][0], 0.3, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["Sigma"][0][1], 0.5, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["Sigma"][0][2], 0.7, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["Lambda"][0][0], 0.4, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["Lambda"][0][1], 0.6, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["Lambda"][0][2], 0.8, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["f1.Sigma"][0][0], 0.5, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["f1.Sigma"][0][1], 0.7, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["f1.Sigma"][0][2], 0.9, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["f1.Lambda"][0][0], 0.6, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["f1.Lambda"][0][1], 0.8, delta=0.000001)
        self.assertAlmostEqual(self.model.fitting_context.y_parameters["f1.Lambda"][0][2], 1.0, delta=0.000001)

        self.assertEqual(
            self.model.fitting_context.y_parameters["workspace_name"][0],
            ["MUSR62260; Group; bottom; Asymmetry; MA", "MUSR62260; Group; top; Asymmetry; MA", "MUSR62260; Group; fwd; Asymmetry; MA"],
        )

    def test_that_create_x_and_y_parameter_combinations_will_not_create_the_parameter_workspaces(self):
        self.model.result_table_names = self.result_table_names
        self.model.current_result_table_index = 0

        table = create_results_table()
        add_ws_to_ads("Result1", table)

        self.model.create_x_and_y_parameter_combinations()

        self.assertTrue(not check_if_workspace_exist("Result1; Parameter Combinations"))

        self.assertTrue(not check_if_workspace_exist("Result1; workspace_name vs A0"))
        self.assertTrue(not check_if_workspace_exist("Result1; workspace_name vs A1"))
        self.assertTrue(not check_if_workspace_exist("Result1; A0 vs workspace_name"))
        self.assertTrue(not check_if_workspace_exist("Result1; A0 vs A1"))
        self.assertTrue(not check_if_workspace_exist("Result1; A1 vs workspace_name"))
        self.assertTrue(not check_if_workspace_exist("Result1; A1 vs A0"))

        self.assertTrue(not check_if_workspace_exist("Result1; workspace_name vs workspace_name"))
        self.assertTrue(not check_if_workspace_exist("Result1; A0 vs A0"))
        self.assertTrue(not check_if_workspace_exist("Result1; A1 vs A1"))

    def test_that_create_x_and_y_parameter_combination_workspace_will_create_the_expected_parameter_workspaces(self):
        self.model.result_table_names = self.result_table_names
        self.model.current_result_table_index = 0

        table = create_results_table()
        add_ws_to_ads("Result1", table)

        self.model.create_x_and_y_parameter_combinations()
        self.model.create_x_and_y_parameter_combination_workspace("workspace_name", "A0")
        self.model.create_x_and_y_parameter_combination_workspace("A1", "A0")
        self.model.create_x_and_y_parameter_combination_workspace("workspace_name", "A1")
        self.model.create_x_and_y_parameter_combination_workspace("Sigma", "Lambda")
        self.model.create_x_and_y_parameter_combination_workspace("f1.Sigma", "f1.Lambda")

        self.assertTrue(check_if_workspace_exist("Result1; Parameter Combinations"))

        self.assertTrue(check_if_workspace_exist("Result1; A0 vs workspace_name"))
        self.assertTrue(check_if_workspace_exist("Result1; A1 vs workspace_name"))
        self.assertTrue(check_if_workspace_exist("Result1; A0 vs A1"))
        self.assertTrue(check_if_workspace_exist("Result1; Lambda vs Sigma"))
        self.assertTrue(check_if_workspace_exist("Result1; f1.Lambda vs f1.Sigma"))

        self.assertTrue(not check_if_workspace_exist("Result1; workspace_name vs A0"))
        self.assertTrue(not check_if_workspace_exist("Result1; workspace_name vs A1"))
        self.assertTrue(not check_if_workspace_exist("Result1; A1 vs A0"))

        self.assertTrue(not check_if_workspace_exist("Result1; workspace_name vs workspace_name"))
        self.assertTrue(not check_if_workspace_exist("Result1; A0 vs A0"))
        self.assertTrue(not check_if_workspace_exist("Result1; A1 vs A1"))

        unit_test_ws = retrieve_ws("Result1; Lambda vs Sigma")
        self.assertTrue(str(unit_test_ws.getAxis(0).getUnit().symbol()) == "\\mu s^{-1}")
        self.assertTrue(str(unit_test_ws.YUnit()) == "Lambda ($\\mu$ $s^{-1}$)")

        unit_test_ws = retrieve_ws("Result1; f1.Lambda vs f1.Sigma")
        self.assertTrue(str(unit_test_ws.getAxis(0).getUnit().symbol()) == "\\mu s^{-1}")
        self.assertTrue(str(unit_test_ws.YUnit()) == "f1.Lambda ($\\mu$ $s^{-1}$)")


if __name__ == "__main__":
    unittest.main()
