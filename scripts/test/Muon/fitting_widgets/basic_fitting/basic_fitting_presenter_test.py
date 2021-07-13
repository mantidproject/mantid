# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import FrameworkManager, FunctionFactory
from mantidqt.widgets.fitscriptgenerator import FittingMode

from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import BasicFittingModel
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_presenter import BasicFittingPresenter
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_view import BasicFittingView
from Muon.GUI.Common.test_helpers.fitting_mock_setup import (add_mock_methods_to_basic_fitting_model,
                                                             add_mock_methods_to_basic_fitting_presenter,
                                                             add_mock_methods_to_basic_fitting_view)
from Muon.GUI.Common.thread_model import ThreadModel


class BasicFittingPresenterTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        self.dataset_names = ["Name1", "Name2"]
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

        self._setup_mock_view()
        self._setup_mock_model()
        self._setup_presenter()

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

    def tearDown(self):
        self.presenter = None
        self.model = None
        self.view = None

    def test_that_handle_ads_clear_or_remove_workspace_event_will_attempt_to_reset_all_the_data_and_enable_gui(self):
        self.presenter.handle_ads_clear_or_remove_workspace_event()

        self.presenter.update_and_reset_all_data.assert_called_with()
        self.presenter.enable_editing_notifier.notify_subscribers.assert_called_once_with()

    def test_that_handle_ads_clear_or_remove_workspace_event_will_disable_the_tab_if_no_data_is_loaded(self):
        self.mock_model_number_of_datasets = mock.PropertyMock(return_value=0)
        type(self.model).number_of_datasets = self.mock_model_number_of_datasets

        self.presenter.handle_ads_clear_or_remove_workspace_event()

        self.presenter.update_and_reset_all_data.assert_called_with()
        self.view.disable_view.assert_called_once_with()

    def test_that_handle_gui_changes_made_will_reset_the_start_and_end_x_in_the_model_and_view(self):
        self.presenter.handle_gui_changes_made({"FirstGoodDataFromFile": True})

        self.model.reset_start_xs_and_end_xs.assert_called_once_with()
        self.mock_model_current_start_x.assert_called_once_with()
        self.mock_view_start_x.assert_called_once_with(self.start_x)
        self.mock_model_current_end_x.assert_called_once_with()
        self.mock_view_end_x.assert_called_once_with(self.end_x)

    def test_that_handle_new_data_loaded_will_attempt_to_reset_all_the_data_and_enable_the_gui(self):
        self.presenter.clear_undo_data = mock.Mock()

        self.presenter.handle_new_data_loaded()

        self.presenter.update_and_reset_all_data.assert_called_with()
        self.mock_view_plot_guess.assert_called_once_with(False)
        self.mock_model_plot_guess.assert_called_once_with(False)
        self.presenter.clear_undo_data.assert_called_with()
        self.presenter.enable_editing_notifier.notify_subscribers.assert_called_once_with()

    def test_that_handle_new_data_loaded_will_disable_the_tab_if_no_data_is_loaded(self):
        self.presenter.clear_undo_data = mock.Mock()
        self.mock_model_number_of_datasets = mock.PropertyMock(return_value=0)
        type(self.model).number_of_datasets = self.mock_model_number_of_datasets

        self.presenter.handle_new_data_loaded()

        self.presenter.update_and_reset_all_data.assert_called_with()
        self.mock_view_plot_guess.assert_called_once_with(False)
        self.mock_model_plot_guess.assert_called_once_with(False)
        self.presenter.clear_undo_data.assert_called_with()
        self.view.disable_view.assert_called_once_with()

    def test_that_handle_plot_guess_changed_will_update_plot_guess_using_the_model(self):
        self.presenter.handle_plot_guess_changed()
        self.model.update_plot_guess.assert_called_once_with()

    def test_that_handle_undo_fit_clicked_will_attempt_to_reset_the_fit_data_and_notify_that_the_data_has_changed(self):
        self.presenter.update_fit_function_in_view_from_model = mock.Mock()
        self.presenter.update_fit_statuses_and_chi_squared_in_view_from_model = mock.Mock()

        self.presenter.handle_undo_fit_clicked()

        self.model.undo_previous_fit.assert_called_once_with()
        self.presenter.update_fit_function_in_view_from_model.assert_called_once_with()
        self.presenter.update_fit_statuses_and_chi_squared_in_view_from_model.assert_called_once_with()
        self.model.update_plot_guess.assert_called_once_with()
        self.presenter.selected_fit_results_changed.notify_subscribers.assert_called_once_with([])

    def test_that_handle_fit_clicked_will_show_a_warning_if_no_data_is_loaded(self):
        self.mock_model_number_of_datasets = mock.PropertyMock(return_value=0)
        type(self.model).number_of_datasets = self.mock_model_number_of_datasets
        self.presenter._perform_fit = mock.Mock()

        self.presenter.handle_fit_clicked()

        self.view.warning_popup.assert_called_once_with("No data selected for fitting.")
        self.assertTrue(not self.presenter._perform_fit.called)

    def test_that_handle_fit_clicked_will_not_perform_a_fit_if_there_is_no_fit_function(self):
        self.mock_view_fit_object = mock.PropertyMock(return_value=None)
        type(self.view).fit_object = self.mock_view_fit_object
        self.presenter._perform_fit = mock.Mock()

        self.presenter.handle_fit_clicked()

        self.assertTrue(not self.presenter._perform_fit.called)

    def test_that_handle_fit_clicked_will_perform_a_fit_and_cache_the_functions_if_the_data_is_valid(self):
        self.presenter._perform_fit = mock.Mock()

        self.presenter.handle_fit_clicked()

        self.model.save_current_fit_function_to_undo_data.assert_called_once_with()
        self.presenter._perform_fit.assert_called_once_with()

    def test_that_handle_started_will_disable_the_tab(self):
        self.presenter.handle_started()
        self.presenter.disable_editing_notifier.notify_subscribers.assert_called_once_with()

    def test_that_handle_finished_will_enable_the_tab_and_handle_the_fitting_results_if_the_thread_is_successful(self):
        self.presenter.handle_finished()

        self.presenter.enable_editing_notifier.notify_subscribers.assert_called_once_with()
        self.mock_presenter_get_fit_results.assert_called_once_with()
        self.presenter.handle_fitting_finished.assert_called_once_with(self.fit_function, self.fit_status,
                                                                       self.chi_squared)
        self.view.set_number_of_undos.assert_called_once_with(1)
        self.mock_view_plot_guess.assert_called_once_with(False)
        self.mock_model_plot_guess.assert_called_once_with(False)

    def test_that_handle_finished_will_enable_the_tab_but_not_call_handle_the_fitting_results_if_the_thread_is_unsuccessful(self):
        self.presenter.thread_success = False
        self.presenter.handle_finished()

        self.presenter.enable_editing_notifier.notify_subscribers.assert_called_once_with()
        self.assertTrue(not self.mock_presenter_get_fit_results.called)
        self.assertTrue(not self.presenter.handle_fitting_finished.called)

    def test_that_handle_finished_will_enable_the_tab_but_not_call_handle_the_fitting_results_if_one_of_the_results_is_invalid(self):
        self.mock_presenter_get_fit_results = mock.PropertyMock(return_value=(None, self.fit_status, self.chi_squared))
        type(self.presenter.fitting_calculation_model).result = self.mock_presenter_get_fit_results

        self.presenter.handle_finished()

        self.presenter.enable_editing_notifier.notify_subscribers.assert_called_once_with()
        self.mock_presenter_get_fit_results.assert_called_once_with()
        self.assertTrue(not self.presenter.handle_fitting_finished.called)

    def test_that_handle_error_will_show_an_error_popup(self):
        error = "Error message"

        self.presenter.handle_error(error)

        self.presenter.enable_editing_notifier.notify_subscribers.assert_called_once_with()
        self.view.warning_popup.assert_called_once_with(error)

    def test_that_handle_fit_generator_clicked_will_attempt_to_open_the_fit_script_generator(self):
        fit_options = {"Minimizer": self.model.minimizer, "Evaluation Type": self.model.evaluation_type}
        self.presenter._open_fit_script_generator_interface = mock.Mock()

        self.presenter.handle_fit_generator_clicked()

        self.mock_model_simultaneous_fitting_mode.assert_called_once_with()
        self.mock_model_dataset_names.assert_called_once_with()
        self.presenter._open_fit_script_generator_interface.assert_called_once_with(self.dataset_names,
                                                                                    FittingMode.SEQUENTIAL,
                                                                                    fit_options)

    def test_that_handle_function_name_changed_by_user_will_set_the_automatic_update_property_to_false(self):
        self.presenter.handle_function_name_changed_by_user()

        self.mock_model_function_name_auto_update.assert_called_once_with(False)
        self.mock_view_function_name.assert_called_once_with()
        self.mock_model_function_name.assert_called_once_with(self.function_name)

    def test_that_handle_minimizer_changed_will_set_the_minimizer_in_the_model(self):
        self.presenter.handle_minimizer_changed()

        self.mock_view_minimizer.assert_called_with()
        self.mock_model_minimizer.assert_called_with(self.minimizer)

    def test_that_handle_evaluation_type_changed_will_set_the_evaluation_type_in_the_model(self):
        self.presenter.handle_evaluation_type_changed()

        self.mock_view_evaluation_type.assert_called_with()
        self.mock_model_evaluation_type.assert_called_with(self.evaluation_type)

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

    def test_that_handle_function_parameter_changed_will_update_the_fit_functions_and_notify_they_are_updated(self):
        function_index = ""
        parameter = "A0"
        parameter_value = 5.0
        full_parameter = f"{function_index}{parameter}"

        self.view.parameter_value = mock.Mock(return_value=parameter_value)
        self.model.update_parameter_value = mock.Mock()
        self.model.update_fit_functions_in_model_from_view = mock.Mock()

        self.presenter.handle_function_parameter_changed(function_index, parameter)

        self.view.parameter_value.assert_called_once_with(full_parameter)
        self.model.update_parameter_value.assert_called_once_with(full_parameter, parameter_value)

        self.model.update_plot_guess.assert_called_once_with()
        self.presenter.fit_parameter_changed_notifier.notify_subscribers.assert_called_once_with()

    def test_that_handle_start_x_updated_will_attempt_to_update_the_start_x_in_the_model(self):
        self.presenter.handle_start_x_updated()
        self.mock_model_current_start_x.assert_called_once_with(self.start_x)

    def test_that_handle_start_x_updated_will_reset_the_start_x_if_it_equals_the_end_x(self):
        self.mock_view_start_x = mock.PropertyMock(return_value=self.end_x)
        type(self.view).start_x = self.mock_view_start_x

        self.presenter.handle_start_x_updated()

        calls = [mock.call(), mock.call(), mock.call(), mock.call(), mock.call(0.0), mock.call()]
        self.mock_view_start_x.assert_has_calls(calls)

    def test_that_handle_start_x_updated_will_use_the_min_start_x_when_the_set_start_x_is_too_small(self):
        x_lower = 3.0
        self.model.x_limits_of_workspace = mock.Mock(return_value=(x_lower, self.end_x))

        self.presenter.handle_start_x_updated()

        calls = [mock.call(), mock.call(3.0), mock.call()]
        self.mock_view_start_x.assert_has_calls(calls)

    def test_that_handle_start_x_updated_will_use_the_max_start_x_when_the_set_start_x_is_too_large(self):
        x_upper = -0.1
        self.model.x_limits_of_workspace = mock.Mock(return_value=(self.start_x, x_upper))
        self.model.is_equal_to_n_decimals = mock.Mock(return_value=False)

        self.presenter.handle_start_x_updated()

        calls = [mock.call(), mock.call(), mock.call(15.0), mock.call()]
        self.mock_view_start_x.assert_has_calls(calls)

    def test_that_handle_end_x_updated_will_attempt_to_update_the_end_x_in_the_model(self):
        self.presenter.handle_end_x_updated()
        self.mock_model_current_end_x.assert_called_once_with(self.end_x)

    def test_that_handle_end_x_updated_will_reset_the_end_x_if_it_equals_the_end_x(self):
        self.mock_view_end_x = mock.PropertyMock(return_value=self.start_x)
        type(self.view).end_x = self.mock_view_end_x

        self.presenter.handle_end_x_updated()

        calls = [mock.call(), mock.call(), mock.call(), mock.call(), mock.call(15.0), mock.call()]
        self.mock_view_end_x.assert_has_calls(calls)

    def test_that_handle_use_rebin_changed_will_not_update_the_model_if_the_rebin_check_fails(self):
        self.presenter._check_rebin_options = mock.Mock(return_value=False)

        self.presenter.handle_use_rebin_changed()

        # The call count is 1 because they are initialized in the presenters constructor
        self.assertEqual(self.mock_view_fit_to_raw.call_count, 1)
        self.assertEqual(self.mock_model_fit_to_raw.call_count, 1)

    def test_that_handle_use_rebin_changed_will_update_the_model_if_the_rebin_check_is_successful(self):
        self.presenter._check_rebin_options = mock.Mock(return_value=True)

        self.presenter.handle_use_rebin_changed()

        # The call count is 2 because they are also initialized in the presenters constructor
        self.assertEqual(self.mock_view_fit_to_raw.call_count, 2)
        self.assertEqual(self.mock_model_fit_to_raw.call_count, 2)

    def test_that_clear_cached_fit_functions_will_clear_the_cache_in_the_model(self):
        self.model.number_of_undos = mock.Mock(return_value=0)
        self.presenter.clear_undo_data()

        self.view.set_number_of_undos.assert_called_once_with(0)
        self.model.clear_undo_data.assert_called_once_with()

    def test_that_reset_fit_status_and_chi_squared_information_will_reset_the_info_in_the_model(self):
        self.presenter.update_fit_statuses_and_chi_squared_in_view_from_model = mock.Mock()

        self.presenter.reset_fit_status_and_chi_squared_information()

        self.model.reset_fit_statuses_and_chi_squared.assert_called_once_with()
        self.presenter.update_fit_statuses_and_chi_squared_in_view_from_model.assert_called_once_with()

    def test_that_reset_start_xs_and_end_xs_will_reset_the_xs_in_the_model_and_view(self):
        self.presenter.reset_start_xs_and_end_xs()

        self.model.reset_start_xs_and_end_xs.assert_called_once_with()
        self.mock_model_current_start_x.assert_called_once_with()
        self.mock_model_current_end_x.assert_called_once_with()
        self.mock_view_start_x.assert_called_once_with(self.start_x)
        self.mock_view_end_x.assert_called_once_with(self.end_x)

    def test_that_set_current_dataset_index_will_set_the_index_in_the_model_and_view(self):
        index = 1

        self.presenter.set_current_dataset_index(index)

        self.mock_model_current_dataset_index.assert_called_once_with(index)
        self.view.set_current_dataset_index.assert_called_once_with(index)

    def test_that_automatically_update_function_name_will_update_the_function_name_in_the_model(self):
        self.presenter.automatically_update_function_name()

        self.model.automatically_update_function_name.assert_called_once_with()
        self.mock_model_function_name.assert_called_once_with()
        self.mock_view_function_name.assert_called_once_with(self.function_name)

    def test_that_update_dataset_names_in_view_and_model_will_update_the_datasets_in_the_model_and_view(self):
        self.presenter.update_dataset_names_in_view_and_model()

        self.model.get_workspace_names_to_display_from_context.assert_called_once_with()
        self.view.set_datasets_in_function_browser.assert_called_once_with(self.dataset_names)

    def test_that_update_fit_function_in_view_from_model_will_update_the_function_and_index_in_the_view(self):
        self.presenter.update_fit_function_in_view_from_model()

        self.view.update_fit_function.assert_called_once_with(self.fit_function)
        self.view.set_current_dataset_index.assert_called_once_with(self.current_dataset_index)

    def test_that_update_fit_functions_in_model_from_view_will_update_the_single_fit_functions_and_notify(self):
        self.presenter.update_single_fit_functions_in_model = mock.Mock()

        self.presenter.update_fit_functions_in_model_from_view()

        self.presenter.update_single_fit_functions_in_model.assert_called_once_with()
        self.presenter.fit_function_changed_notifier.notify_subscribers.assert_called_once_with()

    def test_that_update_single_fit_functions_in_model_will_update_the_fit_functions_in_the_model(self):
        self.presenter._get_single_fit_functions_from_view = mock.Mock(return_value=self.single_fit_functions)

        self.presenter.update_single_fit_functions_in_model()

        self.presenter._get_single_fit_functions_from_view.assert_called_once_with()
        self.mock_model_single_fit_functions.assert_called_once_with(self.single_fit_functions)

    def test_that_update_fit_statuses_and_chi_squared_in_view_from_model_will_update_the_local_and_global_statuses_in_the_view(self):
        self.presenter.update_fit_statuses_and_chi_squared_in_view_from_model()

        self.mock_model_current_fit_status.assert_called_once_with()
        self.mock_model_current_chi_squared.assert_called_once_with()
        self.mock_model_fit_statuses.assert_called_once_with()
        self.mock_model_current_dataset_index.assert_called_once_with()

        self.view.update_local_fit_status_and_chi_squared.assert_called_once_with(self.fit_status, self.chi_squared)
        self.view.update_global_fit_status.assert_called_once_with([self.fit_status] * len(self.dataset_names),
                                                                   self.current_dataset_index)

    def test_that_update_start_and_end_x_in_view_from_model_will_update_the_start_and_end_x_in_the_view(self):
        self.presenter.update_start_and_end_x_in_view_from_model()

        self.mock_model_current_start_x.assert_called_once_with()
        self.mock_view_start_x.assert_called_once_with(self.start_x)
        self.mock_model_current_end_x.assert_called_once_with()
        self.mock_view_end_x.assert_called_once_with(self.end_x)

    def _setup_mock_view(self):
        self.view = mock.Mock(spec=BasicFittingView)
        self.view = add_mock_methods_to_basic_fitting_view(self.view)

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

    def _setup_mock_model(self):
        self.model = mock.Mock(spec=BasicFittingModel)
        self.model = add_mock_methods_to_basic_fitting_model(self.model, self.dataset_names, self.current_dataset_index,
                                                             self.fit_function, self.start_x, self.end_x,
                                                             self.fit_status, self.chi_squared)

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

    def _setup_presenter(self):
        self.presenter = BasicFittingPresenter(self.view, self.model)
        self.presenter = add_mock_methods_to_basic_fitting_presenter(self.presenter)

        # Mock the fit result property
        self.presenter.fitting_calculation_model = mock.Mock(spec=ThreadModel)
        self.mock_presenter_get_fit_results = mock.PropertyMock(return_value=(self.fit_function, self.fit_status,
                                                                              self.chi_squared))
        type(self.presenter.fitting_calculation_model).result = self.mock_presenter_get_fit_results

        # Mock unimplemented methods and notifiers
        self.presenter.handle_fitting_finished = mock.Mock()
        self.presenter.update_and_reset_all_data = mock.Mock()
        self.presenter.disable_editing_notifier.notify_subscribers = mock.Mock()
        self.presenter.enable_editing_notifier.notify_subscribers = mock.Mock()
        self.presenter.selected_fit_results_changed.notify_subscribers = mock.Mock()
        self.presenter.fit_function_changed_notifier.notify_subscribers = mock.Mock()
        self.presenter.fit_parameter_changed_notifier.notify_subscribers = mock.Mock()


if __name__ == '__main__':
    unittest.main()
