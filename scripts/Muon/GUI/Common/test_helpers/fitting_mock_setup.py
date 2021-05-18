# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from unittest import mock


def add_mock_methods_to_basic_fitting_view(view):
    # Mock the methods of the view
    view.set_slot_for_fit_generator_clicked = mock.Mock()
    view.set_slot_for_fit_button_clicked = mock.Mock()
    view.set_slot_for_undo_fit_clicked = mock.Mock()
    view.set_slot_for_plot_guess_changed = mock.Mock()
    view.set_slot_for_fit_name_changed = mock.Mock()
    view.set_slot_for_function_structure_changed = mock.Mock()
    view.set_slot_for_function_parameter_changed = mock.Mock()
    view.set_slot_for_start_x_updated = mock.Mock()
    view.set_slot_for_end_x_updated = mock.Mock()
    view.set_slot_for_minimizer_changed = mock.Mock()
    view.set_slot_for_evaluation_type_changed = mock.Mock()
    view.set_slot_for_use_raw_changed = mock.Mock()

    view.set_datasets_in_function_browser = mock.Mock()
    view.set_current_dataset_index = mock.Mock()
    view.update_local_fit_status_and_chi_squared = mock.Mock()
    view.update_global_fit_status = mock.Mock()
    view.update_fit_function = mock.Mock()
    view.enable_undo_fit = mock.Mock()
    view.number_of_datasets = mock.Mock(return_value=2)
    view.warning_popup = mock.Mock()
    view.get_global_parameters = mock.Mock(return_value=[])
    view.switch_to_simultaneous = mock.Mock()
    view.switch_to_single = mock.Mock()
    return view


def add_mock_methods_to_basic_fitting_model(model, dataset_names, current_dataset_index, fit_function, start_x, end_x,
                                            fit_status, chi_squared):
    # Mock the methods of the model
    model.clear_single_fit_functions = mock.Mock()
    model.get_single_fit_function_for = mock.Mock(return_value=fit_function)
    model.cache_the_current_fit_functions = mock.Mock()
    model.clear_cached_fit_functions = mock.Mock()
    model.automatically_update_function_name = mock.Mock()
    model.use_cached_function = mock.Mock()
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
    return model


def add_mock_methods_to_basic_fitting_presenter(presenter):
    # Mock unimplemented methods and notifiers
    presenter.handle_fitting_finished = mock.Mock()
    presenter.update_and_reset_all_data = mock.Mock()
    presenter.disable_editing_notifier.notify_subscribers = mock.Mock()
    presenter.enable_editing_notifier.notify_subscribers = mock.Mock()
    presenter.disable_fitting_notifier.notify_subscribers = mock.Mock()
    presenter.selected_fit_results_changed.notify_subscribers = mock.Mock()
    presenter.fit_function_changed_notifier.notify_subscribers = mock.Mock()
    presenter.fit_parameter_changed_notifier.notify_subscribers = mock.Mock()
    return presenter


def add_mock_methods_to_general_fitting_view(view):
    view = add_mock_methods_to_basic_fitting_view(view)

    view.set_slot_for_dataset_changed = mock.Mock()
    view.set_slot_for_fitting_mode_changed = mock.Mock()
    view.set_slot_for_simultaneous_fit_by_changed = mock.Mock()
    view.set_slot_for_simultaneous_fit_by_specifier_changed = mock.Mock()
    return view
