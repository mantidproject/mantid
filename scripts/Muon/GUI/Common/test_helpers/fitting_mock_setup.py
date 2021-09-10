# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from unittest import mock

from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import BasicFittingModel
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_presenter import BasicFittingPresenter
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_view import BasicFittingView
from Muon.GUI.Common.thread_model import ThreadModel


def add_mock_methods_to_basic_fitting_view(view):
    # Mock the methods of the view
    view.set_slot_for_fit_generator_clicked = mock.Mock()
    view.set_slot_for_fit_button_clicked = mock.Mock()
    view.set_slot_for_undo_fit_clicked = mock.Mock()
    view.set_slot_for_plot_guess_changed = mock.Mock()
    view.set_slot_for_fit_name_changed = mock.Mock()
    view.set_slot_for_covariance_matrix_clicked = mock.Mock()
    view.set_slot_for_function_structure_changed = mock.Mock()
    view.set_slot_for_function_parameter_changed = mock.Mock()
    view.set_slot_for_start_x_updated = mock.Mock()
    view.set_slot_for_end_x_updated = mock.Mock()
    view.set_slot_for_exclude_range_state_changed = mock.Mock()
    view.set_slot_for_exclude_start_x_updated = mock.Mock()
    view.set_slot_for_exclude_end_x_updated = mock.Mock()
    view.set_slot_for_minimizer_changed = mock.Mock()
    view.set_slot_for_evaluation_type_changed = mock.Mock()
    view.set_slot_for_use_raw_changed = mock.Mock()

    view.set_datasets_in_function_browser = mock.Mock()
    view.set_current_dataset_index = mock.Mock()
    view.update_local_fit_status_and_chi_squared = mock.Mock()
    view.update_global_fit_status = mock.Mock()
    view.update_fit_function = mock.Mock()
    view.set_number_of_undos = mock.Mock()
    view.number_of_datasets = mock.Mock(return_value=2)
    view.warning_popup = mock.Mock()
    view.get_global_parameters = mock.Mock(return_value=[])
    view.switch_to_simultaneous = mock.Mock()
    view.switch_to_single = mock.Mock()
    view.disable_view = mock.Mock()
    view.set_exclude_start_and_end_x_visible = mock.Mock()
    view.set_covariance_button_enabled = mock.Mock()
    view.show_normalised_covariance_matrix = mock.Mock()
    return view


def add_mock_methods_to_basic_fitting_model(model, dataset_names, current_dataset_index, fit_function, start_x, end_x,
                                            fit_status, chi_squared):
    # Mock the methods of the model
    model.clear_single_fit_functions = mock.Mock()
    model.get_single_fit_function_for = mock.Mock(return_value=fit_function)
    model.save_current_fit_function_to_undo_data = mock.Mock()
    model.clear_undo_data = mock.Mock()
    model.automatically_update_function_name = mock.Mock()
    model.undo_previous_fit = mock.Mock()
    model.update_plot_guess = mock.Mock()
    model.remove_all_fits_from_context = mock.Mock()
    model.reset_current_dataset_index = mock.Mock()
    model.reset_start_xs_and_end_xs = mock.Mock()
    model.reset_fit_statuses_and_chi_squared = mock.Mock()
    model.reset_fit_functions = mock.Mock()
    model.x_limits_of_workspace = mock.Mock(return_value=(start_x, end_x))
    model.retrieve_first_good_data_from_run = mock.Mock(return_value=start_x)
    model.get_active_fit_function = mock.Mock(return_value=fit_function)
    model.get_active_workspace_names = mock.Mock(return_value=[dataset_names[current_dataset_index]])
    model.get_active_fit_results = mock.Mock(return_value=[])
    model.get_workspace_names_to_display_from_context = mock.Mock(return_value=dataset_names)
    model.perform_fit = mock.Mock(return_value=(fit_function, fit_status, chi_squared))
    model.number_of_undos = mock.Mock(return_value=1)
    model.has_normalised_covariance_matrix = mock.Mock(return_value=True)
    return model


def add_mock_methods_to_basic_fitting_presenter(presenter):
    # Mock unimplemented methods and notifiers
    presenter.handle_fitting_finished = mock.Mock()
    presenter.update_and_reset_all_data = mock.Mock()
    presenter.disable_editing_notifier.notify_subscribers = mock.Mock()
    presenter.enable_editing_notifier.notify_subscribers = mock.Mock()
    presenter.selected_fit_results_changed.notify_subscribers = mock.Mock()
    presenter.fit_function_changed_notifier.notify_subscribers = mock.Mock()
    presenter.fit_parameter_changed_notifier.notify_subscribers = mock.Mock()
    presenter.update_exclude_start_and_end_x_in_view_and_model = mock.Mock()
    return presenter


def add_mock_methods_to_general_fitting_view(view):
    view = add_mock_methods_to_basic_fitting_view(view)

    view.set_slot_for_dataset_changed = mock.Mock()
    view.set_slot_for_fitting_mode_changed = mock.Mock()
    view.set_slot_for_simultaneous_fit_by_changed = mock.Mock()
    view.set_slot_for_simultaneous_fit_by_specifier_changed = mock.Mock()
    return view


def add_mock_methods_to_model_fitting_view(view):
    view = add_mock_methods_to_basic_fitting_view(view)

    view.set_slot_for_results_table_changed = mock.Mock()
    view.set_slot_for_selected_x_changed = mock.Mock()
    view.set_slot_for_selected_y_changed = mock.Mock()

    view.add_results_table_name = mock.Mock()
    view.update_result_table_names = mock.Mock()
    view.update_x_and_y_parameters = mock.Mock()
    view.update_fit_function = mock.Mock()
    view.x_parameter = mock.Mock(return_value="A0")
    view.y_parameter = mock.Mock(return_value="A1")
    view.enable_view = mock.Mock()
    view.set_selected_x_parameter = mock.Mock()
    view.set_selected_y_parameter = mock.Mock()

    return view


def add_mock_methods_to_model_fitting_model(model, dataset_names, current_dataset_index, fit_function, start_x, end_x,
                                            fit_status, chi_squared, param_combination_name, param_group_name,
                                            results_table_names, x_parameters, y_parameters):
    model = add_mock_methods_to_basic_fitting_model(model, dataset_names, current_dataset_index, fit_function, start_x,
                                                    end_x, fit_status, chi_squared)

    model.parameter_combination_workspace_name = mock.Mock(return_value=param_combination_name)
    model.parameter_combination_group_name = mock.Mock(return_value=param_group_name)
    model.get_workspace_names_to_display_from_context = mock.Mock(return_value=results_table_names)
    model.create_x_and_y_parameter_combination_workspaces = mock.Mock(return_value=(x_parameters, y_parameters))
    model.get_first_x_parameter_not = mock.Mock(return_value="A0")
    model.get_first_y_parameter_not = mock.Mock(return_value="A1")
    model.x_parameters = mock.Mock(return_value=x_parameters)
    model.y_parameters = mock.Mock(return_value=y_parameters)

    return model


def add_mock_methods_to_model_fitting_presenter(presenter):
    presenter = add_mock_methods_to_basic_fitting_presenter(presenter)
    return presenter


class MockBasicFitting:

    def _setup_mock_view(self):
        self.view = mock.Mock(spec=BasicFittingView)
        self.view = add_mock_methods_to_basic_fitting_view(self.view)

        # Mock the properties of the view
        self.mock_view_current_dataset_index = mock.PropertyMock(return_value=self.current_dataset_index)
        type(self.view).current_dataset_index = self.mock_view_current_dataset_index
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
        self.mock_view_exclude_range = mock.PropertyMock(return_value=True)
        type(self.view).exclude_range = self.mock_view_exclude_range
        self.mock_view_exclude_start_x = mock.PropertyMock(return_value=self.start_x)
        type(self.view).exclude_start_x = self.mock_view_exclude_start_x
        self.mock_view_exclude_end_x = mock.PropertyMock(return_value=self.end_x)
        type(self.view).exclude_end_x = self.mock_view_exclude_end_x
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
        self.mock_model_exclude_range = mock.PropertyMock(return_value=True)
        type(self.model).exclude_range = self.mock_model_exclude_range
        self.mock_model_current_exclude_start_x = mock.PropertyMock(return_value=self.start_x)
        type(self.model).current_exclude_start_x = self.mock_model_current_exclude_start_x
        self.mock_model_current_exclude_end_x = mock.PropertyMock(return_value=self.end_x)
        type(self.model).current_exclude_end_x = self.mock_model_current_exclude_end_x
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
