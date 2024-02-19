# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import FrameworkManager, FunctionFactory

from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_model import TFAsymmetryFittingModel
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_presenter import (
    TFAsymmetryFittingPresenter,
)
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_view import TFAsymmetryFittingView


class TFAsymmetryFittingPresenterTest(unittest.TestCase):
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
        self.tf_asymmetry_mode = True
        self.normalisation = 3.0
        self.normalisation_error = 0.3

        self._setup_mock_view()
        self._setup_mock_model()
        self._setup_presenter()

        self.mock_view_minimizer.assert_called_once_with()
        self.mock_view_evaluation_type.assert_called_once_with()
        self.mock_view_fit_to_raw.assert_called_once_with()
        self.mock_view_simultaneous_fit_by.assert_called_once_with()
        self.mock_view_simultaneous_fit_by_specifier.assert_called_once_with()
        self.mock_view_global_parameters.assert_called_once_with()
        self.mock_view_tf_asymmetry_mode.assert_called_once_with()
        self.mock_model_minimizer.assert_called_once_with(self.minimizer)
        self.mock_model_evaluation_type.assert_called_once_with(self.evaluation_type)
        self.mock_model_fit_to_raw.assert_called_once_with(self.fit_to_raw)
        self.mock_model_simultaneous_fit_by.assert_called_once_with(self.simultaneous_fit_by)
        self.mock_model_simultaneous_fit_by_specifier.assert_called_once_with(self.simultaneous_fit_by_specifier)
        self.mock_model_global_parameters.assert_called_once_with(self.global_parameters)
        self.mock_model_tf_asymmetry_mode.assert_called_once_with(self.tf_asymmetry_mode)

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
        self.assertEqual(self.view.set_slot_for_fitting_type_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_normalisation_changed.call_count, 1)

    def tearDown(self):
        self.presenter = None
        self.model = None
        self.view = None

    def test_that_handle_instrument_changed_will_turn_tf_asymmetry_mode_off(self):
        self.presenter.handle_instrument_changed()

        self.mock_view_tf_asymmetry_mode.assert_called_with(False)
        self.mock_model_tf_asymmetry_mode.assert_called_with(False)
        self.assertEqual(self.mock_view_tf_asymmetry_mode.call_count, 2)
        self.assertEqual(self.mock_model_tf_asymmetry_mode.call_count, 2)

    def test_that_handle_pulse_type_changed_will_not_turn_tf_asymmetry_mode_off_if_it_contains_does_not_contain_DoublePulseEnabled(self):
        updated_variables = {"FirstVariable": True, "OtherVariable": False}

        self.presenter.handle_pulse_type_changed(updated_variables)

        # The call count is one because they were initialized in the constructor
        self.assertEqual(self.mock_view_tf_asymmetry_mode.call_count, 1)
        self.assertEqual(self.mock_model_tf_asymmetry_mode.call_count, 1)

    def test_that_handle_ads_clear_or_remove_workspace_event_will_attempt_to_reset_all_the_data_and_enable_gui(self):
        self.mock_model_dataset_names = mock.PropertyMock(return_value=["Name"])
        type(self.model).dataset_names = self.mock_model_dataset_names
        self.presenter.update_and_reset_all_data = mock.Mock()

        self.presenter.handle_ads_clear_or_remove_workspace_event("Name")

        self.presenter.update_and_reset_all_data.assert_called_with()
        self.assertEqual(self.mock_view_tf_asymmetry_mode.call_count, 1)
        self.assertEqual(self.mock_model_tf_asymmetry_mode.call_count, 1)

    def test_that_handle_ads_clear_or_remove_workspace_event_will_attempt_to_reset_all_the_data_and_enable_gui_with_no_datasets(self):
        self.mock_model_dataset_names = mock.PropertyMock(return_value=["Name"])
        type(self.model).dataset_names = self.mock_model_dataset_names
        self.mock_model_number_of_datasets = mock.PropertyMock(return_value=0)
        type(self.model).number_of_datasets = self.mock_model_number_of_datasets
        self.presenter.update_and_reset_all_data = mock.Mock()

        self.presenter.handle_ads_clear_or_remove_workspace_event("Name")

        self.presenter.update_and_reset_all_data.assert_called_with()
        self.mock_view_tf_asymmetry_mode.assert_called_with(False)
        self.mock_model_tf_asymmetry_mode.assert_called_with(False)
        self.assertEqual(self.mock_view_tf_asymmetry_mode.call_count, 2)
        self.assertEqual(self.mock_model_tf_asymmetry_mode.call_count, 2)

    def test_that_handle_new_data_loaded_will_attempt_to_reset_all_the_data_and_enable_the_gui(self):
        self.presenter.clear_undo_data = mock.Mock()
        self.presenter.update_and_reset_all_data = mock.Mock()

        self.presenter.handle_new_data_loaded()

        self.presenter.update_and_reset_all_data.assert_called_with()
        self.mock_view_plot_guess.assert_called_once_with(False)
        self.mock_model_plot_guess.assert_called_once_with(False)
        self.presenter.clear_undo_data.assert_called_with()
        self.assertEqual(self.mock_view_tf_asymmetry_mode.call_count, 1)
        self.assertEqual(self.mock_model_tf_asymmetry_mode.call_count, 1)

    def test_that_handle_new_data_loaded_will_disable_the_tab_if_no_data_is_loaded(self):
        self.mock_model_number_of_datasets = mock.PropertyMock(return_value=0)
        type(self.model).number_of_datasets = self.mock_model_number_of_datasets
        self.presenter.clear_undo_data = mock.Mock()
        self.presenter.update_and_reset_all_data = mock.Mock()

        self.presenter.handle_new_data_loaded()

        self.presenter.update_and_reset_all_data.assert_called_with()
        self.mock_view_plot_guess.assert_called_once_with(False)
        self.mock_model_plot_guess.assert_called_once_with(False)
        self.presenter.clear_undo_data.assert_called_with()
        self.mock_view_tf_asymmetry_mode.assert_called_with(False)
        self.mock_model_tf_asymmetry_mode.assert_called_with(False)
        self.assertEqual(self.mock_view_tf_asymmetry_mode.call_count, 2)
        self.assertEqual(self.mock_model_tf_asymmetry_mode.call_count, 2)

    def test_that_handle_function_structure_changed_will_attempt_to_update_the_tf_asymmetry_functions(self):
        self.presenter.update_tf_asymmetry_functions_in_model_and_view = mock.Mock()

        self.presenter.handle_function_structure_changed()

        self.presenter.update_tf_asymmetry_functions_in_model_and_view.assert_called_once_with()

    def test_that_handle_dataset_name_changed_will_attempt_to_update_the_normalisation_displayed_in_the_view(self):
        self.presenter.handle_dataset_name_changed()

        self.model.current_normalisation.assert_called_with()
        self.model.current_normalisation_error.assert_called_with()
        self.view.set_normalisation.assert_called_with(self.normalisation, self.normalisation_error)

    def test_that_handle_tf_asymmetry_mode_changed_will_not_set_the_tf_asymmetry_mode_if_the_workspaces_do_not_comply(self):
        self.presenter._check_tf_asymmetry_compliance = mock.Mock(return_value=False)

        self.presenter.handle_tf_asymmetry_mode_changed(self.tf_asymmetry_mode)

        # The call count is one because they were initialized in the constructor
        self.assertEqual(self.mock_view_tf_asymmetry_mode.call_count, 1)
        self.assertEqual(self.mock_model_tf_asymmetry_mode.call_count, 1)

        self.presenter._check_tf_asymmetry_compliance.assert_called_once_with(self.tf_asymmetry_mode)

    def test_that_handle_tf_asymmetry_mode_changed_will_set_the_tf_asymmetry_mode_if_the_workspaces_do_comply(self):
        self.presenter._check_tf_asymmetry_compliance = mock.Mock(return_value=True)
        self.presenter.update_tf_asymmetry_functions_in_model_and_view = mock.Mock()
        self.presenter.reset_start_xs_and_end_xs = mock.Mock()
        self.presenter.reset_fit_status_and_chi_squared_information = mock.Mock()
        self.presenter.clear_undo_data = mock.Mock()
        self.presenter.automatically_update_function_name = mock.Mock()

        self.presenter.handle_tf_asymmetry_mode_changed(self.tf_asymmetry_mode)

        # The call count is one because they were initialized in the constructor
        self.assertEqual(self.mock_view_tf_asymmetry_mode.call_count, 2)
        self.assertEqual(self.mock_model_tf_asymmetry_mode.call_count, 2)

        self.presenter._check_tf_asymmetry_compliance.assert_called_once_with(self.tf_asymmetry_mode)
        self.presenter.update_tf_asymmetry_functions_in_model_and_view.assert_called_once_with()

        self.presenter.reset_fit_status_and_chi_squared_information.assert_called_once_with()
        self.presenter.clear_undo_data.assert_called_once_with()
        self.presenter.automatically_update_function_name.assert_called_once_with()

        self.model.update_plot_guess(self.plot_guess)

    def test_that_handle_normalisation_changed_sets_the_normalisation_in_the_model_and_updates_the_guess(self):
        self.presenter.handle_normalisation_changed()

        self.mock_view_normalisation.assert_called_with()
        self.model.set_current_normalisation.assert_called_once_with(self.normalisation)

        self.model.update_plot_guess(self.plot_guess)

    def test_that_update_and_reset_all_data_will_attempt_to_update_the_tf_asymmetry_functions(self):
        self.presenter.update_tf_asymmetry_functions_in_model_and_view = mock.Mock()

        self.presenter.update_and_reset_all_data()

        self.presenter.update_tf_asymmetry_functions_in_model_and_view.assert_called_once_with()

    def test_that_update_tf_asymmetry_functions_in_model_and_view_shows_a_warning_if_it_fails_to_recalculate_the_functions(self):
        self.model.recalculate_tf_asymmetry_functions = mock.Mock(return_value=False)

        self.presenter.update_tf_asymmetry_functions_in_model_and_view()

        self.model.recalculate_tf_asymmetry_functions.assert_called_once_with()
        self.view.warning_popup.assert_called_once_with("Failed to convert fit function to a TF Asymmetry function.")

        self.model.current_normalisation.assert_called_with()
        self.view.set_normalisation.assert_called_with(self.normalisation, self.normalisation_error)

    def test_that_update_tf_asymmetry_functions_in_model_and_view_will_set_the_normalisation_in_the_view(self):
        self.model.recalculate_tf_asymmetry_functions = mock.Mock(return_value=True)

        self.presenter.update_tf_asymmetry_functions_in_model_and_view()

        self.model.recalculate_tf_asymmetry_functions.assert_called_once_with()

        self.model.current_normalisation.assert_called_with()
        self.view.set_normalisation.assert_called_with(self.normalisation, self.normalisation_error)

    def test_that_update_fit_function_in_model_will_update_the_simultaneous_fit_functions_when_in_simultaneous_mode(self):
        self.model.update_tf_asymmetry_simultaneous_fit_function = mock.Mock()
        self.mock_model_simultaneous_fitting_mode = mock.PropertyMock(return_value=True)
        type(self.model).simultaneous_fitting_mode = self.mock_model_simultaneous_fitting_mode

        self.presenter.update_fit_function_in_model(self.fit_function)

        self.model.update_tf_asymmetry_simultaneous_fit_function.assert_called_once_with(self.fit_function)

    def test_that_update_fit_function_in_model_will_update_the_current_fit_function_when_in_single_mode(self):
        self.model.update_tf_asymmetry_single_fit_function = mock.Mock()
        self.mock_model_simultaneous_fitting_mode = mock.PropertyMock(return_value=False)
        type(self.model).simultaneous_fitting_mode = self.mock_model_simultaneous_fitting_mode

        self.presenter.update_fit_function_in_model(self.fit_function)

        self.model.update_tf_asymmetry_single_fit_function.assert_called_once_with(self.model.current_dataset_index, self.fit_function)

    def test_that_handle_sequential_fit_finished_will_update_the_fit_functions_and_statuses_in_the_view_and_model(self):
        self.presenter.update_fit_function_in_view_from_model = mock.Mock()
        self.presenter.update_fit_statuses_and_chi_squared_in_view_from_model = mock.Mock()

        self.presenter.handle_sequential_fit_finished()

        self.presenter.update_fit_function_in_view_from_model.assert_called_once_with()
        self.presenter.update_fit_statuses_and_chi_squared_in_view_from_model.assert_called_once_with()

    def _setup_mock_view(self):
        self.view = mock.Mock(spec=TFAsymmetryFittingView)

        # Mock the properties of the view
        self.mock_view_current_dataset_index = mock.PropertyMock(return_value=self.current_dataset_index)
        type(self.view).current_dataset_index = self.mock_view_current_dataset_index
        self.mock_view_current_dataset_name = mock.PropertyMock(return_value=self.dataset_names[self.current_dataset_index])
        type(self.view).current_dataset_name = self.mock_view_current_dataset_name
        self.mock_view_simultaneous_fit_by = mock.PropertyMock(return_value=self.simultaneous_fit_by)
        type(self.view).simultaneous_fit_by = self.mock_view_simultaneous_fit_by
        self.mock_view_simultaneous_fit_by_specifier = mock.PropertyMock(return_value=self.simultaneous_fit_by_specifier)
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
        self.mock_view_tf_asymmetry_mode = mock.PropertyMock(return_value=self.tf_asymmetry_mode)
        type(self.view).tf_asymmetry_mode = self.mock_view_tf_asymmetry_mode
        self.mock_view_normalisation = mock.PropertyMock(return_value=self.normalisation)
        type(self.view).normalisation = self.mock_view_normalisation

        # Mock the methods of the view
        self.view.set_slot_for_fit_generator_clicked = mock.Mock()
        self.view.set_slot_for_fit_button_clicked = mock.Mock()
        self.view.set_slot_for_undo_fit_clicked = mock.Mock()
        self.view.set_slot_for_plot_guess_changed = mock.Mock()
        self.view.set_slot_for_fit_name_changed = mock.Mock()
        self.view.set_slot_for_function_structure_changed = mock.Mock()
        self.view.set_slot_for_function_parameter_changed = mock.Mock()
        self.view.set_slot_for_start_x_updated = mock.Mock()
        self.view.set_slot_for_end_x_updated = mock.Mock()
        self.view.set_slot_for_minimizer_changed = mock.Mock()
        self.view.set_slot_for_evaluation_type_changed = mock.Mock()
        self.view.set_slot_for_use_raw_changed = mock.Mock()
        self.view.set_slot_for_dataset_changed = mock.Mock()
        self.view.set_slot_for_fitting_mode_changed = mock.Mock()
        self.view.set_slot_for_simultaneous_fit_by_changed = mock.Mock()
        self.view.set_slot_for_simultaneous_fit_by_specifier_changed = mock.Mock()
        self.view.set_slot_for_fitting_type_changed = mock.Mock()
        self.view.set_slot_for_normalisation_changed = mock.Mock()

        self.view.set_datasets_in_function_browser = mock.Mock()
        self.view.set_current_dataset_index = mock.Mock()
        self.view.update_local_fit_status_and_chi_squared = mock.Mock()
        self.view.update_global_fit_status = mock.Mock()
        self.view.update_fit_function = mock.Mock()
        self.view.enable_undo_fit = mock.Mock()
        self.view.number_of_datasets = mock.Mock(return_value=len(self.dataset_names))
        self.view.warning_popup = mock.Mock()
        self.view.get_global_parameters = mock.Mock(return_value=[])
        self.view.switch_to_simultaneous = mock.Mock()
        self.view.switch_to_single = mock.Mock()
        self.view.set_normalisation = mock.Mock()

    def _setup_mock_model(self):
        self.model = mock.Mock(spec=TFAsymmetryFittingModel)

        # Mock the properties of the model
        self.mock_model_current_dataset_index = mock.PropertyMock(return_value=self.current_dataset_index)
        type(self.model).current_dataset_index = self.mock_model_current_dataset_index
        self.mock_model_dataset_names = mock.PropertyMock(return_value=self.dataset_names)
        type(self.model).dataset_names = self.mock_model_dataset_names
        self.mock_model_current_dataset_name = mock.PropertyMock(return_value=self.dataset_names[self.current_dataset_index])
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
        self.mock_model_simultaneous_fit_by_specifier = mock.PropertyMock(return_value=self.simultaneous_fit_by_specifier)
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
        self.mock_model_tf_asymmetry_mode = mock.PropertyMock(return_value=self.tf_asymmetry_mode)
        type(self.model).tf_asymmetry_mode = self.mock_model_tf_asymmetry_mode

        # Mock the context
        self.model.context = mock.Mock()

        # Mock the methods of the model
        self.model.clear_single_fit_functions = mock.Mock()
        self.model.get_single_fit_function_for = mock.Mock(return_value=self.fit_function)
        self.model.cache_the_current_fit_functions = mock.Mock()
        self.model.clear_undo_data = mock.Mock()
        self.model.automatically_update_function_name = mock.Mock()
        self.model.save_current_fit_function_to_undo_data = mock.Mock()
        self.model.update_plot_guess = mock.Mock()
        self.model.remove_all_fits_from_context = mock.Mock()
        self.model.reset_current_dataset_index = mock.Mock()
        self.model.reset_start_xs_and_end_xs = mock.Mock()
        self.model.reset_fit_statuses_and_chi_squared = mock.Mock()
        self.model.reset_fit_functions = mock.Mock()
        self.model.x_limits_of_workspace = mock.Mock(return_value=(self.start_x, self.end_x))
        self.model.retrieve_first_good_data_from_run = mock.Mock(return_value=self.start_x)
        self.model.get_active_fit_function = mock.Mock(return_value=self.fit_function)
        self.model.get_active_workspace_names = mock.Mock(return_value=[self.dataset_names[self.current_dataset_index]])
        self.model.get_active_fit_results = mock.Mock(return_value=[])
        self.model.get_workspace_names_to_display_from_context = mock.Mock(return_value=self.dataset_names)
        self.model.perform_fit = mock.Mock(return_value=(self.fit_function, self.fit_status, self.chi_squared))
        self.model.current_normalisation = mock.Mock(return_value=self.normalisation)
        self.model.set_current_normalisation = mock.Mock()
        self.model.current_normalisation_error = mock.Mock(return_value=self.normalisation_error)

    def _setup_presenter(self):
        self.presenter = TFAsymmetryFittingPresenter(self.view, self.model)

        # Mock unimplemented methods and notifiers
        self.presenter.disable_editing_notifier.notify_subscribers = mock.Mock()
        self.presenter.enable_editing_notifier.notify_subscribers = mock.Mock()
        self.presenter.selected_fit_results_changed.notify_subscribers = mock.Mock()
        self.presenter.fit_function_changed_notifier.notify_subscribers = mock.Mock()
        self.presenter.fit_parameter_changed_notifier.notify_subscribers = mock.Mock()
        self.presenter.fitting_mode_changed_notifier.notify_subscribers = mock.Mock()


if __name__ == "__main__":
    unittest.main()
