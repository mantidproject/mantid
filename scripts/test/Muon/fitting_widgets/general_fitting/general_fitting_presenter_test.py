# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import FrameworkManager, FunctionFactory

from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model import GeneralFittingModel
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_presenter import GeneralFittingPresenter
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_view import GeneralFittingView
from Muon.GUI.Common.test_helpers.fitting_mock_setup import (add_mock_methods_to_basic_fitting_model,
                                                             add_mock_methods_to_general_fitting_view)


class GeneralFittingPresenterTest(unittest.TestCase):

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
        self.simultaneous_fitting_mode = True
        self.simultaneous_fit_by = "Group/Pair"
        self.simultaneous_fit_by_specifier = "fwd"
        self.global_parameters = ["A0"]
        self.fit_function = FunctionFactory.createFunction("FlatBackground")
        self.single_fit_functions = [self.fit_function.clone(), self.fit_function.clone()]

        self._setup_mock_view()
        self._setup_mock_model()
        self._setup_presenter()

        self.mock_view_minimizer.assert_called_once_with()
        self.mock_view_evaluation_type.assert_called_once_with()
        self.mock_view_fit_to_raw.assert_called_once_with()
        self.mock_view_simultaneous_fit_by.assert_called_once_with()
        self.mock_view_simultaneous_fit_by_specifier.assert_called_once_with()
        self.mock_view_global_parameters.assert_called_once_with()
        self.mock_model_minimizer.assert_called_once_with(self.minimizer)
        self.mock_model_evaluation_type.assert_called_once_with(self.evaluation_type)
        self.mock_model_fit_to_raw.assert_called_once_with(self.fit_to_raw)
        self.mock_model_simultaneous_fit_by.assert_called_once_with(self.simultaneous_fit_by)
        self.mock_model_simultaneous_fit_by_specifier.assert_called_once_with(self.simultaneous_fit_by_specifier)
        self.mock_model_global_parameters.assert_called_once_with(self.global_parameters)

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
        self.assertEqual(self.view.set_slot_for_dataset_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_fitting_mode_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_simultaneous_fit_by_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_simultaneous_fit_by_specifier_changed.call_count, 1)

    def tearDown(self):
        self.presenter = None
        self.model = None
        self.view = None

    def test_that_handle_instrument_changed_will_update_and_reset_the_data(self):
        self.presenter.update_and_reset_all_data = mock.Mock()
        self.presenter.clear_undo_data = mock.Mock()
        self.presenter.model.remove_all_fits_from_context = mock.Mock()

        self.presenter.handle_instrument_changed()

        self.presenter.update_and_reset_all_data.assert_called_with()
        self.presenter.clear_undo_data.assert_called_once_with()
        self.presenter.model.remove_all_fits_from_context.assert_called_once_with()

    def test_that_handle_pulse_type_changed_will_update_and_reset_the_data_if_it_contains_DoublePulseEnabled(self):
        updated_variables = {"DoublePulseEnabled": True, "OtherVariable": False}
        self.presenter.update_and_reset_all_data = mock.Mock()

        self.presenter.handle_pulse_type_changed(updated_variables)

        self.presenter.update_and_reset_all_data.assert_called_with()

    def test_that_handle_selected_group_pair_changed_will_set_the_list_of_fit_by_specifiers(self):
        self.model.get_simultaneous_fit_by_specifiers_to_display_from_context = mock.Mock(
            return_value=[self.simultaneous_fit_by_specifier])

        self.presenter.handle_selected_group_pair_changed()

        self.model.get_simultaneous_fit_by_specifiers_to_display_from_context.assert_called_once_with()
        self.view.setup_fit_by_specifier.assert_called_with([self.simultaneous_fit_by_specifier])

    def test_that_handle_fitting_finished_will_update_the_fit_results_in_the_model_and_view_and_calls_the_notifiers(self):
        self.presenter.update_fit_statuses_and_chi_squared_in_model = mock.Mock()
        self.presenter.update_fit_function_in_model = mock.Mock()
        self.presenter.update_fit_statuses_and_chi_squared_in_view_from_model = mock.Mock()
        self.presenter.update_fit_function_in_view_from_model = mock.Mock()

        self.presenter.handle_fitting_finished(self.fit_function, self.fit_status, self.chi_squared)

        self.presenter.update_fit_statuses_and_chi_squared_in_model.assert_called_once_with(self.fit_status,
                                                                                            self.chi_squared)
        self.presenter.update_fit_function_in_model.assert_called_once_with(self.fit_function)
        self.presenter.update_fit_statuses_and_chi_squared_in_view_from_model.assert_called_once_with()
        self.presenter.update_fit_function_in_view_from_model.assert_called_once_with()
        self.presenter.selected_fit_results_changed.notify_subscribers.assert_called_once_with([])
        self.presenter.fit_parameter_changed_notifier.notify_subscribers.assert_called_once_with()

    def test_that_handle_fitting_mode_changed_will_update_the_functions_and_reset_the_gui_data(self):
        self.presenter.switch_fitting_mode_in_view = mock.Mock()
        self.presenter.update_fit_functions_in_model_from_view = mock.Mock()
        self.presenter.update_dataset_names_in_view_and_model = mock.Mock()
        self.presenter.reset_start_xs_and_end_xs = mock.Mock()
        self.presenter.reset_fit_status_and_chi_squared_information = mock.Mock()
        self.presenter.clear_undo_data = mock.Mock()

        self.presenter.handle_fitting_mode_changed()

        self.mock_view_simultaneous_fitting_mode.assert_called_once_with()
        self.mock_model_simultaneous_fitting_mode.assert_called_once_with(self.simultaneous_fitting_mode)

        self.presenter.switch_fitting_mode_in_view.assert_called_once_with()
        self.presenter.update_fit_functions_in_model_from_view.assert_called_once_with()
        self.presenter.update_dataset_names_in_view_and_model.assert_called_once_with()
        self.presenter.reset_fit_status_and_chi_squared_information.assert_called_once_with()
        self.presenter.clear_undo_data.assert_called_once_with()
        self.presenter.fitting_mode_changed_notifier.notify_subscribers.assert_called_once_with()
        self.presenter.fit_function_changed_notifier.notify_subscribers.assert_called_once_with()

    def test_that_handle_simultaneous_fit_by_changed_will_update_the_model(self):
        self.presenter.update_simultaneous_fit_by_specifiers_in_view = mock.Mock()

        self.presenter.handle_simultaneous_fit_by_changed()

        self.mock_view_simultaneous_fit_by.assert_called_with()
        self.mock_model_simultaneous_fit_by.assert_called_with(self.simultaneous_fit_by)
        self.presenter.update_simultaneous_fit_by_specifiers_in_view.assert_called_once_with()

    def test_that_handle_simultaneous_fit_by_specifier_changed_will_update_the_model(self):
        self.presenter.update_dataset_names_in_view_and_model = mock.Mock()
        self.presenter.reset_fit_status_and_chi_squared_information = mock.Mock()
        self.presenter.clear_undo_data = mock.Mock()

        self.presenter.handle_simultaneous_fit_by_specifier_changed()

        self.mock_view_simultaneous_fit_by_specifier.assert_called_with()
        self.mock_model_simultaneous_fit_by_specifier.assert_called_with(self.simultaneous_fit_by_specifier)

        self.presenter.update_dataset_names_in_view_and_model.assert_called_once_with()
        self.presenter.reset_fit_status_and_chi_squared_information.assert_called_once_with()
        self.presenter.clear_undo_data.assert_called_once_with()
        self.presenter.simultaneous_fit_by_specifier_changed.notify_subscribers.assert_called_once_with()

    def test_that_set_selected_dataset_will_set_the_dataset_name_in_the_view(self):
        dataset_name = "Name2"

        self.presenter.set_selected_dataset(dataset_name)

        self.mock_view_current_dataset_name.assert_called_once_with(dataset_name)

    def test_that_switch_fitting_mode_in_view_will_switch_to_simultaneous_mode_when_appropriate(self):
        self.mock_model_simultaneous_fitting_mode = mock.PropertyMock(return_value=True)
        type(self.model).simultaneous_fitting_mode = self.mock_model_simultaneous_fitting_mode
        self.view.switch_to_simultaneous = mock.Mock()

        self.presenter.switch_fitting_mode_in_view()

        self.view.switch_to_simultaneous.assert_called_once_with()

    def test_that_switch_fitting_mode_in_view_will_switch_to_single_mode_when_appropriate(self):
        self.mock_model_simultaneous_fitting_mode = mock.PropertyMock(return_value=False)
        type(self.model).simultaneous_fitting_mode = self.mock_model_simultaneous_fitting_mode
        self.view.switch_to_single = mock.Mock()

        self.presenter.switch_fitting_mode_in_view()

        self.view.switch_to_single.assert_called_once_with()

    def test_that_update_and_reset_all_data_will_attempt_to_update_the_simultaneous_fit_by_specifiers(self):
        self.presenter.update_simultaneous_fit_by_specifiers_in_view = mock.Mock()

        self.presenter.update_and_reset_all_data()

        self.presenter.update_simultaneous_fit_by_specifiers_in_view.assert_called_once_with()

    def test_that_update_fit_statuses_and_chi_squared_in_model_will_update_all_fit_info_when_in_simultaneous_mode(self):
        self.mock_model_simultaneous_fitting_mode = mock.PropertyMock(return_value=True)
        type(self.model).simultaneous_fitting_mode = self.mock_model_simultaneous_fitting_mode

        self.presenter.update_fit_statuses_and_chi_squared_in_model(self.fit_status, self.chi_squared)

        self.mock_model_fit_statuses.assert_called_once_with([self.fit_status] * len(self.dataset_names))
        self.mock_model_chi_squared.assert_called_once_with([self.chi_squared] * len(self.dataset_names))

    def test_that_update_fit_statuses_and_chi_squared_in_model_will_update_the_current_fit_status_when_in_single_mode(self):
        self.mock_model_simultaneous_fitting_mode = mock.PropertyMock(return_value=False)
        type(self.model).simultaneous_fitting_mode = self.mock_model_simultaneous_fitting_mode

        self.presenter.update_fit_statuses_and_chi_squared_in_model(self.fit_status, self.chi_squared)

        self.mock_model_current_fit_status.assert_called_once_with(self.fit_status)
        self.mock_model_current_chi_squared.assert_called_once_with(self.chi_squared)

    def test_that_update_fit_function_in_model_will_update_the_simultaneous_fit_function_when_in_simultaneous_mode(self):
        self.mock_model_simultaneous_fitting_mode = mock.PropertyMock(return_value=True)
        type(self.model).simultaneous_fitting_mode = self.mock_model_simultaneous_fitting_mode

        self.presenter.update_fit_function_in_model(self.fit_function)

        self.mock_model_simultaneous_fit_function.assert_called_once_with(self.fit_function)

    def test_that_update_fit_function_in_model_will_update_the_current_fit_function_when_in_single_mode(self):
        self.mock_model_simultaneous_fitting_mode = mock.PropertyMock(return_value=False)
        type(self.model).simultaneous_fitting_mode = self.mock_model_simultaneous_fitting_mode

        self.presenter.update_fit_function_in_model(self.fit_function)

        self.mock_model_current_single_fit_function.assert_called_once_with(self.fit_function)

    def test_that_update_simultaneous_fit_by_specifiers_in_view_will_change_the_fit_specifiers_in_the_view(self):
        self.model.get_simultaneous_fit_by_specifiers_to_display_from_context = mock.Mock(
            return_value=[self.simultaneous_fit_by_specifier])
        self.view.setup_fit_by_specifier = mock.Mock()

        self.presenter.update_simultaneous_fit_by_specifiers_in_view()

        self.model.get_simultaneous_fit_by_specifiers_to_display_from_context.assert_called_once_with()
        self.view.setup_fit_by_specifier.assert_called_with([self.simultaneous_fit_by_specifier])

    def test_that_update_dataset_names_in_view_and_model_will_update_the_dataset_names_in_the_view(self):
        self.view.update_dataset_name_combo_box = mock.Mock()

        self.presenter.update_dataset_names_in_view_and_model()

        self.mock_model_dataset_names.assert_called_with()
        self.view.update_dataset_name_combo_box.assert_called_once_with(self.dataset_names)
        self.mock_view_current_dataset_index.assert_called_once_with()
        self.mock_model_current_dataset_index.assert_called_once_with(self.current_dataset_index)

    def test_that_update_fit_functions_in_model_from_view_will_update_the_global_parameters_in_the_model(self):
        self.presenter.update_fit_functions_in_model_from_view()

        self.mock_view_global_parameters.assert_called_with()
        self.mock_model_global_parameters.assert_called_with(self.global_parameters)

    def test_that_update_fit_functions_in_model_from_view_will_update_the_simultaneous_function_if_in_simultaneous_mode(self):
        self.mock_model_simultaneous_fitting_mode = mock.PropertyMock(return_value=True)
        type(self.model).simultaneous_fitting_mode = self.mock_model_simultaneous_fitting_mode
        self.model.clear_single_fit_functions = mock.Mock()
        self.presenter.update_simultaneous_fit_function_in_model = mock.Mock()

        self.presenter.update_fit_functions_in_model_from_view()

        self.model.clear_single_fit_functions.assert_called_once_with()
        self.presenter.update_simultaneous_fit_function_in_model.assert_called_once_with()

    def test_that_update_fit_functions_in_model_from_view_will_update_the_single_function_if_in_single_mode(self):
        self.mock_model_simultaneous_fitting_mode = mock.PropertyMock(return_value=False)
        type(self.model).simultaneous_fitting_mode = self.mock_model_simultaneous_fitting_mode
        self.model.clear_simultaneous_fit_function = mock.Mock()
        self.presenter.update_single_fit_functions_in_model = mock.Mock()

        self.presenter.update_fit_functions_in_model_from_view()

        self.model.clear_simultaneous_fit_function.assert_called_once_with()
        self.presenter.update_single_fit_functions_in_model.assert_called_once_with()

    def test_that_update_simultaneous_fit_function_in_model_will_set_the_simultaneous_function_in_the_model(self):
        self.presenter.update_simultaneous_fit_function_in_model()

        self.mock_view_fit_object.assert_called_once_with()
        self.mock_model_simultaneous_fit_function.assert_called_once_with(self.fit_function)

    def _setup_mock_view(self):
        self.view = mock.Mock(spec=GeneralFittingView)
        self.view = add_mock_methods_to_general_fitting_view(self.view)

        # Mock the properties of the view
        self.mock_view_current_dataset_index = mock.PropertyMock(return_value=self.current_dataset_index)
        type(self.view).current_dataset_index = self.mock_view_current_dataset_index
        self.mock_view_current_dataset_name = mock.PropertyMock(return_value=
                                                                self.dataset_names[self.current_dataset_index])
        type(self.view).current_dataset_name = self.mock_view_current_dataset_name
        self.mock_view_simultaneous_fit_by = mock.PropertyMock(return_value=self.simultaneous_fit_by)
        type(self.view).simultaneous_fit_by = self.mock_view_simultaneous_fit_by
        self.mock_view_simultaneous_fit_by_specifier = mock.PropertyMock(return_value=
                                                                         self.simultaneous_fit_by_specifier)
        type(self.view).simultaneous_fit_by_specifier = self.mock_view_simultaneous_fit_by_specifier
        self.mock_view_global_parameters = mock.PropertyMock(return_value=self.global_parameters)
        type(self.view).global_parameters = self.mock_view_global_parameters
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
        self.mock_view_simultaneous_fitting_mode = mock.PropertyMock(return_value=self.simultaneous_fitting_mode)
        type(self.view).simultaneous_fitting_mode = self.mock_view_simultaneous_fitting_mode

    def _setup_mock_model(self):
        self.model = mock.Mock(spec=GeneralFittingModel)
        self.model = add_mock_methods_to_basic_fitting_model(self.model, self.dataset_names, self.current_dataset_index,
                                                             self.fit_function, self.start_x, self.end_x,
                                                             self.fit_status, self.chi_squared)
        # Mock the context
        self.model.context = mock.Mock()

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
        self.mock_model_simultaneous_fit_by = mock.PropertyMock(return_value=self.simultaneous_fit_by)
        type(self.model).simultaneous_fit_by = self.mock_model_simultaneous_fit_by
        self.mock_model_simultaneous_fit_by_specifier = mock.PropertyMock(return_value=
                                                                          self.simultaneous_fit_by_specifier)
        type(self.model).simultaneous_fit_by_specifier = self.mock_model_simultaneous_fit_by_specifier
        self.mock_model_global_parameters = mock.PropertyMock(return_value=self.global_parameters)
        type(self.model).global_parameters = self.mock_model_global_parameters
        self.mock_model_single_fit_functions = mock.PropertyMock(return_value=self.single_fit_functions)
        type(self.model).single_fit_functions = self.mock_model_single_fit_functions
        self.mock_model_current_single_fit_function = mock.PropertyMock(return_value=self.fit_function)
        type(self.model).current_single_fit_function = self.mock_model_current_single_fit_function
        self.mock_model_single_fit_functions_cache = mock.PropertyMock(return_value=self.fit_function)
        type(self.model).single_fit_functions_cache = self.mock_model_single_fit_functions_cache
        self.mock_model_simultaneous_fit_function = mock.PropertyMock(return_value=self.fit_function)
        type(self.model).simultaneous_fit_function = self.mock_model_simultaneous_fit_function
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
        self.mock_model_simultaneous_fitting_mode = mock.PropertyMock(return_value=self.simultaneous_fitting_mode)
        type(self.model).simultaneous_fitting_mode = self.mock_model_simultaneous_fitting_mode
        self.mock_model_global_parameters = mock.PropertyMock(return_value=[])
        type(self.model).global_parameters = self.mock_model_global_parameters
        self.mock_model_do_rebin = mock.PropertyMock(return_value=False)
        type(self.model).do_rebin = self.mock_model_do_rebin

    def _setup_presenter(self):
        self.presenter = GeneralFittingPresenter(self.view, self.model)

        # Mock unimplemented methods and notifiers
        self.presenter.disable_editing_notifier.notify_subscribers = mock.Mock()
        self.presenter.enable_editing_notifier.notify_subscribers = mock.Mock()
        self.presenter.selected_fit_results_changed.notify_subscribers = mock.Mock()
        self.presenter.fit_function_changed_notifier.notify_subscribers = mock.Mock()
        self.presenter.fit_parameter_changed_notifier.notify_subscribers = mock.Mock()
        self.presenter.fitting_mode_changed_notifier.notify_subscribers = mock.Mock()
        self.presenter.simultaneous_fit_by_specifier_changed.notify_subscribers = mock.Mock()


if __name__ == '__main__':
    unittest.main()
