# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import FrameworkManager, FunctionFactory

from Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_model import ModelFittingModel
from Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_presenter import ModelFittingPresenter
from Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_view import ModelFittingView
from Muon.GUI.Common.test_helpers.fitting_mock_setup import (add_mock_methods_to_model_fitting_model,
                                                             add_mock_methods_to_model_fitting_presenter,
                                                             add_mock_methods_to_model_fitting_view)
from Muon.GUI.Common.thread_model import ThreadModel


class ModelFittingPresenterTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        self.dataset_names = ["Results1; A0 vs A1", "Results1; A1 vs A0"]
        self.current_dataset_index = 0
        self.start_x = 0.0
        self.end_x = 15.0
        self.fit_status = "success"
        self.chi_squared = 1.5
        self.function_name = "FlatBackground"
        self.minimizer = "Levenberg-Marquardt"
        self.evaluation_type = "eval type"
        self.fit_to_raw = True
        self.plot_guess = True
        self.fit_function = FunctionFactory.createFunction("FlatBackground")
        self.single_fit_functions = [self.fit_function.clone(), self.fit_function.clone()]

        self.param_combination_name = "Results1; A0 vs A1"
        self.param_group_name = "Results1; Parameter Combinations"
        self.result_table_names = ["Results1", "Results2"]
        self.x_parameters = ["A0", "A1"]
        self.y_parameters = ["A0", "A1"]

        self._setup_mock_view()
        self._setup_mock_model()
        self._setup_presenter()

    def tearDown(self):
        self.presenter = None
        self.model = None
        self.view = None

    def test_that_the_presenter_init_calls_the_expected_methods(self):
        self.mock_view_minimizer.assert_called_once_with()
        self.mock_view_evaluation_type.assert_called_once_with()
        self.mock_view_fit_to_raw.assert_called_once_with()
        self.mock_model_minimizer.assert_called_once_with(self.minimizer)
        self.mock_model_evaluation_type.assert_called_once_with(self.evaluation_type)
        self.mock_model_fit_to_raw.assert_called_once_with(self.fit_to_raw)

        self.assertEqual(self.view.set_slot_for_fit_generator_clicked.call_count, 1)
        self.assertEqual(self.view.set_slot_for_fit_button_clicked.call_count, 1)
        self.assertEqual(self.view.set_slot_for_undo_fit_clicked.call_count, 1)
        self.assertEqual(self.view.set_slot_for_plot_guess_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_fit_name_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_function_structure_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_function_parameter_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_start_x_updated.call_count, 1)
        self.assertEqual(self.view.set_slot_for_end_x_updated.call_count, 1)
        self.assertEqual(self.view.set_slot_for_minimizer_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_evaluation_type_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_use_raw_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_results_table_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_selected_x_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_selected_y_changed.call_count, 1)

    def test_that_handle_new_results_table_created_will_append_a_results_table_name_if_it_does_not_exist_already(self):
        new_table = "Results3"
        self.presenter.handle_new_results_table_created(new_table)

        self.mock_model_result_table_names.assert_has_calls([mock.call(), mock.call(),
                                                             mock.call(["Results1", "Results2", new_table])])
        self.view.add_results_table_name.assert_called_once_with(new_table)

    def test_that_handle_new_results_table_will_call_handle_results_table_changed_if_the_provided_table_is_selected(self):
        self.presenter.handle_results_table_changed = mock.Mock()

        new_table = "Results1"
        self.presenter.handle_new_results_table_created(new_table)

        self.mock_model_result_table_names.assert_called_once_with()
        self.mock_model_current_result_table_name.assert_called_once_with()
        self.presenter.handle_results_table_changed.assert_called_once_with()

    def test_that_handle_new_results_table_will_not_call_handle_results_table_changed_if_the_provided_table_is_not_selected(self):
        self.presenter.handle_results_table_changed = mock.Mock()

        new_table = "Results2"
        self.presenter.handle_new_results_table_created(new_table)

        self.mock_model_result_table_names.assert_called_once_with()
        self.mock_model_current_result_table_name.assert_called_once_with()
        self.assertEqual(self.presenter.handle_results_table_changed.call_count, 0)

    def test_that_handle_results_table_changed_will_attempt_to_create_the_parameter_combination_tables(self):
        self.presenter._create_parameter_combination_workspaces = mock.Mock()
        self.presenter.handle_parameter_combinations_finished = mock.Mock()

        self.presenter.handle_results_table_changed()

        self.mock_view_current_result_table_index.assert_called_once_with()
        self.mock_model_current_result_table_index.assert_called_once_with(0)
        self.presenter._create_parameter_combination_workspaces.assert_called_once_with(
            self.presenter.handle_parameter_combinations_finished)

    def test_that_handle_selected_x_changed_will_find_a_different_y_parameter_if_it_matches_x(self):
        self.presenter.update_selected_parameter_combination_workspace = mock.Mock()
        self.view.y_parameter = mock.Mock(return_value="A0")

        self.presenter.handle_selected_x_changed()

        self.view.x_parameter.assert_called_once_with()
        self.view.y_parameter.assert_called_once_with()
        self.model.get_first_y_parameter_not.assert_called_once_with("A0")
        self.view.set_selected_y_parameter.assert_called_once_with("A1")

        self.presenter.update_selected_parameter_combination_workspace.assert_called_once_with()

    def test_that_handle_selected_x_changed_will_not_find_a_different_y_parameter_if_it_is_different_to_x(self):
        self.presenter.update_selected_parameter_combination_workspace = mock.Mock()

        self.presenter.handle_selected_x_changed()

        self.view.x_parameter.assert_called_once_with()
        self.view.y_parameter.assert_called_once_with()
        self.assertEqual(self.model.get_first_y_parameter_not.call_count, 0)
        self.assertEqual(self.view.set_selected_y_parameter.call_count, 0)

        self.presenter.update_selected_parameter_combination_workspace.assert_called_once_with()

    def test_that_handle_selected_y_changed_will_find_a_different_x_parameter_if_it_matches_y(self):
        self.presenter.update_selected_parameter_combination_workspace = mock.Mock()
        self.view.x_parameter = mock.Mock(return_value="A1")

        self.presenter.handle_selected_y_changed()

        self.view.y_parameter.assert_called_once_with()
        self.view.x_parameter.assert_called_once_with()
        self.model.get_first_x_parameter_not.assert_called_once_with("A1")
        self.view.set_selected_x_parameter.assert_called_once_with("A0")

        self.presenter.update_selected_parameter_combination_workspace.assert_called_once_with()

    def test_that_handle_selected_y_changed_will_not_find_a_different_x_parameter_if_it_is_different_to_y(self):
        self.presenter.update_selected_parameter_combination_workspace = mock.Mock()

        self.presenter.handle_selected_y_changed()

        self.view.y_parameter.assert_called_once_with()
        self.view.x_parameter.assert_called_once_with()
        self.assertEqual(self.model.get_first_x_parameter_not.call_count, 0)
        self.assertEqual(self.view.set_selected_x_parameter.call_count, 0)

        self.presenter.update_selected_parameter_combination_workspace.assert_called_once_with()

    def test_that_handle_parameter_combinations_created_successfully_will_update_the_view(self):
        self.presenter.handle_selected_x_and_y_changed = mock.Mock()

        self.presenter.handle_parameter_combinations_created_successfully()

        self.mock_model_dataset_names.assert_has_calls([mock.call(), mock.call()])
        self.view.set_datasets_in_function_browser.assert_called_once_with(self.dataset_names)
        self.view.update_dataset_name_combo_box.assert_called_once_with(self.dataset_names, emit_signal=False)
        self.view.update_y_parameters.assert_called_once_with(self.y_parameters)
        self.view.update_x_parameters.assert_called_once_with(self.x_parameters, emit_signal=True)

    def test_that_handle_parameter_combinations_error_will_show_a_warning_in_the_view(self):
        error = "Error message"
        self.presenter.handle_parameter_combinations_error(error)

        self.presenter.disable_editing_notifier.notify_subscribers.assert_called_once_with()
        self.view.warning_popup.assert_called_once_with(error)

    def test_that_handle_function_structure_changed_will_update_the_fit_functions_and_notify_they_are_updated(self):
        self.presenter.update_fit_functions_in_model_from_view = mock.Mock()
        self.presenter.automatically_update_function_name = mock.Mock()
        self.presenter.reset_fit_status_and_chi_squared_information = mock.Mock()

        self.presenter.handle_function_structure_changed()

        self.presenter.update_fit_functions_in_model_from_view.assert_called_once_with()
        self.presenter.automatically_update_function_name.assert_called_once_with()
        self.model.get_active_fit_function.assert_called_once_with()
        self.presenter.reset_fit_status_and_chi_squared_information.assert_called_once_with()
        self.model.update_plot_guess.assert_called_once_with()
        self.presenter.fit_function_changed_notifier.notify_subscribers.assert_called_once_with()

    def test_that_update_dataset_names_in_view_and_model_will_update_the_datasets_in_the_model_and_view(self):
        self.presenter.update_dataset_names_in_view_and_model()

        self.model.get_workspace_names_to_display_from_context.assert_called_once_with()
        self.mock_model_result_table_names.assert_has_calls([mock.call(self.result_table_names), mock.call()])
        self.view.update_result_table_names.assert_called_once_with(self.result_table_names)

    def test_that_update_selected_parameter_combination_workspace_calls_the_expected_methods(self):
        self.presenter.update_selected_parameter_combination_workspace()

        self.mock_model_dataset_names.assert_called_once_with()
        self.mock_model_current_dataset_index.assert_called_once_with(0)
        self.mock_view_current_dataset_name.assert_called_once_with(self.param_combination_name)

    def test_that_clear_current_fit_function_for_undo_will_only_clear_the_current_function(self):
        self.presenter.clear_current_fit_function_for_undo()

        self.model.clear_undo_data_for_current_dataset_index.assert_called_once_with()
        self.model.number_of_undos.assert_called_once_with()
        self.view.set_number_of_undos.assert_called_once_with(1)

    def _setup_mock_view(self):
        self.view = mock.Mock(spec=ModelFittingView)
        self.view = add_mock_methods_to_model_fitting_view(self.view)

        # Mock the properties of the view
        self.mock_view_minimizer = mock.PropertyMock(return_value=self.minimizer)
        type(self.view).minimizer = self.mock_view_minimizer
        self.mock_view_evaluation_type = mock.PropertyMock(return_value=self.evaluation_type)
        type(self.view).evaluation_type = self.mock_view_evaluation_type
        self.mock_view_fit_to_raw = mock.PropertyMock(return_value=self.fit_to_raw)
        type(self.view).fit_to_raw = self.mock_view_fit_to_raw
        self.mock_view_fit_object = mock.PropertyMock(return_value=self.fit_function)
        type(self.view).fit_object = self.mock_view_fit_object
        self.mock_view_start_x = mock.PropertyMock(return_value=self.start_x)
        type(self.view).start_x = self.mock_view_start_x
        self.mock_view_end_x = mock.PropertyMock(return_value=self.end_x)
        type(self.view).end_x = self.mock_view_end_x
        self.mock_view_plot_guess = mock.PropertyMock(return_value=self.plot_guess)
        type(self.view).plot_guess = self.mock_view_plot_guess
        self.mock_view_function_name = mock.PropertyMock(return_value=self.function_name)
        type(self.view).function_name = self.mock_view_function_name
        self.mock_view_current_result_table_index = mock.PropertyMock(return_value=0)
        type(self.view).current_result_table_index = self.mock_view_current_result_table_index
        self.mock_view_current_dataset_name = mock.PropertyMock()
        type(self.view).current_dataset_name = self.mock_view_current_dataset_name

    def _setup_mock_model(self):
        self.model = mock.Mock(spec=ModelFittingModel)
        self.model = add_mock_methods_to_model_fitting_model(self.model, self.dataset_names, self.current_dataset_index,
                                                             self.fit_function, self.start_x, self.end_x,
                                                             self.fit_status, self.chi_squared,
                                                             self.param_combination_name, self.param_group_name,
                                                             self.result_table_names, self.x_parameters,
                                                             self.y_parameters)

        # Mock the properties of the model
        self.mock_model_current_dataset_index = mock.PropertyMock(return_value=self.current_dataset_index)
        type(self.model).current_dataset_index = self.mock_model_current_dataset_index
        self.mock_model_dataset_names = mock.PropertyMock(return_value=self.dataset_names)
        type(self.model).dataset_names = self.mock_model_dataset_names
        self.mock_model_current_dataset_name = mock.PropertyMock(return_value=
                                                                 self.dataset_names[self.current_dataset_index])
        type(self.model).current_dataset_name = self.mock_model_current_dataset_name
        self.mock_model_number_of_datasets = mock.PropertyMock(return_value=len(self.dataset_names))
        type(self.model).number_of_datasets = self.mock_model_number_of_datasets
        self.mock_model_start_xs = mock.PropertyMock(return_value=[self.start_x] * len(self.dataset_names))
        type(self.model).start_xs = self.mock_model_start_xs
        self.mock_model_current_start_x = mock.PropertyMock(return_value=self.start_x)
        type(self.model).current_start_x = self.mock_model_current_start_x
        self.mock_model_end_xs = mock.PropertyMock(return_value=[self.end_x] * len(self.dataset_names))
        type(self.model).end_xs = self.mock_model_end_xs
        self.mock_model_current_end_x = mock.PropertyMock(return_value=self.end_x)
        type(self.model).current_end_x = self.mock_model_current_end_x
        self.mock_model_plot_guess = mock.PropertyMock(return_value=self.plot_guess)
        type(self.model).plot_guess = self.mock_model_plot_guess
        self.mock_model_minimizer = mock.PropertyMock(return_value=self.minimizer)
        type(self.model).minimizer = self.mock_model_minimizer
        self.mock_model_evaluation_type = mock.PropertyMock(return_value=self.evaluation_type)
        type(self.model).evaluation_type = self.mock_model_evaluation_type
        self.mock_model_fit_to_raw = mock.PropertyMock(return_value=self.fit_to_raw)
        type(self.model).fit_to_raw = self.mock_model_fit_to_raw
        self.mock_model_single_fit_functions = mock.PropertyMock(return_value=self.single_fit_functions)
        type(self.model).single_fit_functions = self.mock_model_single_fit_functions
        self.mock_model_current_single_fit_function = mock.PropertyMock(return_value=self.fit_function)
        type(self.model).current_single_fit_function = self.mock_model_current_single_fit_function
        self.mock_model_single_fit_functions_cache = mock.PropertyMock(return_value=self.fit_function)
        type(self.model).single_fit_functions_cache = self.mock_model_single_fit_functions_cache
        self.mock_model_fit_statuses = mock.PropertyMock(return_value=[self.fit_status] * len(self.dataset_names))
        type(self.model).fit_statuses = self.mock_model_fit_statuses
        self.mock_model_current_fit_status = mock.PropertyMock(return_value=self.fit_status)
        type(self.model).current_fit_status = self.mock_model_current_fit_status
        self.mock_model_chi_squared = mock.PropertyMock(return_value=[self.chi_squared] * len(self.dataset_names))
        type(self.model).chi_squared = self.mock_model_chi_squared
        self.mock_model_current_chi_squared = mock.PropertyMock(return_value=self.chi_squared)
        type(self.model).current_chi_squared = self.mock_model_current_chi_squared
        self.mock_model_function_name = mock.PropertyMock(return_value=self.function_name)
        type(self.model).function_name = self.mock_model_function_name
        self.mock_model_function_name_auto_update = mock.PropertyMock(return_value=True)
        type(self.model).function_name_auto_update = self.mock_model_function_name_auto_update
        self.mock_model_simultaneous_fitting_mode = mock.PropertyMock(return_value=False)
        type(self.model).simultaneous_fitting_mode = self.mock_model_simultaneous_fitting_mode
        self.mock_model_global_parameters = mock.PropertyMock(return_value=[])
        type(self.model).global_parameters = self.mock_model_global_parameters
        self.mock_model_do_rebin = mock.PropertyMock(return_value=False)
        type(self.model).do_rebin = self.mock_model_do_rebin

        self.mock_model_current_result_table_index = mock.PropertyMock(return_value=0)
        type(self.model).current_result_table_index = self.mock_model_current_result_table_index
        self.mock_model_result_table_names = mock.PropertyMock(return_value=self.result_table_names)
        type(self.model).result_table_names = self.mock_model_result_table_names
        self.mock_model_current_result_table_name = mock.PropertyMock(return_value=self.result_table_names[0])
        type(self.model).current_result_table_name = self.mock_model_current_result_table_name

    def _setup_presenter(self):
        self.presenter = ModelFittingPresenter(self.view, self.model)
        self.presenter = add_mock_methods_to_model_fitting_presenter(self.presenter)

        # Mock the fit result property
        self.presenter.fitting_calculation_model = mock.Mock(spec=ThreadModel)
        self.mock_presenter_get_fit_results = mock.PropertyMock(return_value=(self.fit_function, self.fit_status,
                                                                              self.chi_squared))
        type(self.presenter.fitting_calculation_model).result = self.mock_presenter_get_fit_results


if __name__ == '__main__':
    unittest.main()
